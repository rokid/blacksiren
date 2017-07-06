#include <thread>
#include <iostream>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <sys/eventfd.h>
#include <sys/stat.h>


#include "isiren.h"
#include "sutils.h"
#include "siren_channel.h"


static void setnonblocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if(opts < 0) {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts = opts | O_NONBLOCK;
    if(fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(sock,SETFL,opts)");
        exit(1);
    }
}

namespace BlackSiren {

Message* allocateMessage(int msg, int len) {
    char *pBuffer = new char [sizeof(Message) + len];
    if (pBuffer == nullptr) {
        return nullptr;
    }
    memset (pBuffer, 0, sizeof(Message) + len);
    Message *pMessage = (Message *)pBuffer;
    pMessage->magic[0] = 'a';
    pMessage->magic[1] = 'a';
    pMessage->magic[2] = 'b';
    pMessage->magic[3] = 'b';

    pMessage->msg = msg;
    pMessage->len = len;
    if (len == 0) {
        pMessage->data = nullptr;
    } else {
        pMessage->data = pBuffer + sizeof(Message);
    }

    return pMessage;
}

Message* allocateMessageFromVTWord(std::vector<siren_vt_word> &vt_words) {
    if (vt_words.empty()) {
        return nullptr;
    }
    int total_len = 0;

    std::vector<int> packed_lens;
    for (siren_vt_word vt_word : vt_words) {
        //we have 3 string member
        int vt_word_str_num = vt_word.vt_word.length();
        int vt_phone_str_num = vt_word.vt_phone.length();
        int vt_nnet_path_str_num = 0;
        if (!vt_word.alg_config.nnet_path.empty()) {
            vt_nnet_path_str_num = vt_word.alg_config.nnet_path.length();
        }

        //vt_type(int)
        //vt_block_avg_score/vt_block_min_score/vt_classify_shield(float)
        //vt_left_sil_det/vt_right_sil_det/vt_remote_check_with_aec
        ///vt_remote_check_without_aec/vt_local_classify_check(bool)
        //string size(int)
        //string content
        //including three '\0'
        int other_num = sizeof(UnpackedVTConfig) + vt_word_str_num + vt_phone_str_num + vt_nnet_path_str_num + 3;
        int round_up_len = other_num + (8 - other_num % 8);
        //siren_printf(SIREN_INFO, "round from %d to %d", other_num, round_up_len);
        packed_lens.push_back(round_up_len);
        total_len += round_up_len;
    }
    total_len += 4 * sizeof(int);
    //siren_printf(SIREN_INFO, "allocate unpackedVTConfig with len %d", total_len);

    char *pBuffer = new char [sizeof(Message) + total_len];
    if (pBuffer == nullptr) {
        return nullptr;
    }

    memset (pBuffer, 0, sizeof(Message) + total_len);
    Message *pMessage = (Message *)pBuffer;
    pMessage->magic[0] = 'a';
    pMessage->magic[1] = 'a';
    pMessage->magic[2] = 'b';
    pMessage->magic[3] = 'b';

    pMessage->msg = SIREN_REQUEST_MSG_SYNC_VT_WORD_LIST;
    pMessage->len = total_len;
    pMessage->data = pBuffer + sizeof(Message);
    char *p = pMessage->data;
    int num = vt_words.size();
    memcpy(p, (char *)&num, sizeof(int));
    p += sizeof(int) * 4;
    char *k = p;
    int j = 0;

    for (siren_vt_word vt_word : vt_words) {
        UnpackedVTConfig *unpackedVTConfig = (UnpackedVTConfig *)k;
        p = k;
        unpackedVTConfig->total_len = packed_lens[j];
        k += packed_lens[j];
        j++;

        unpackedVTConfig->vt_block_avg_score = vt_word.alg_config.vt_block_avg_score;
        unpackedVTConfig->vt_block_min_score = vt_word.alg_config.vt_block_min_score;
        unpackedVTConfig->vt_classify_shield = vt_word.alg_config.vt_classify_shield;

        unpackedVTConfig->vt_left_sil_det = vt_word.alg_config.vt_left_sil_det;
        unpackedVTConfig->vt_right_sil_det = vt_word.alg_config.vt_right_sil_det;
        unpackedVTConfig->vt_remote_check_with_aec = vt_word.alg_config.vt_remote_check_with_aec;
        unpackedVTConfig->vt_remote_check_without_aec = vt_word.alg_config.vt_remote_check_without_aec;
        unpackedVTConfig->vt_local_classify_check = vt_word.alg_config.vt_local_classify_check;

        unpackedVTConfig->vt_type = vt_word.vt_type;
        int vt_word_tmp_len = vt_word.vt_word.length();
        int vt_phone_tmp_len = vt_word.vt_phone.length();
        int nnet_path_tmp_len =  vt_word.alg_config.nnet_path.empty() ? 0 : vt_word.alg_config.nnet_path.length();

        unpackedVTConfig->vt_word_size = vt_word_tmp_len;
        unpackedVTConfig->vt_phone_size = vt_phone_tmp_len;
        unpackedVTConfig->vt_nnet_path_size = nnet_path_tmp_len;

        memcpy(p, unpackedVTConfig, sizeof(UnpackedVTConfig));
        p += sizeof(UnpackedVTConfig);

        memcpy(p, vt_word.vt_word.c_str(), vt_word_tmp_len);
        p += vt_word_tmp_len;
        *p = '\0';
        p++;

        memcpy(p, vt_word.vt_phone.c_str(), vt_phone_tmp_len);
        p += vt_phone_tmp_len;
        *p = '\0';
        p++;

        if (!vt_word.alg_config.nnet_path.empty()) {
            memcpy(p, vt_word.alg_config.nnet_path.c_str(), nnet_path_tmp_len);
            p += nnet_path_tmp_len;
        }
        *p = '\0';
        p++;
    }

    return pMessage;
}

int getVTWordFromMessage(Message *message, std::vector<siren_vt_word> &vt_words) {
    if (message == nullptr) {
        siren_printf(SIREN_ERROR, "message is nullptr");
        return -1;
    }

    if (message->data == nullptr) {
        siren_printf(SIREN_ERROR, "message data is nullptr");
        return -2;
    }

    if (message->msg != SIREN_REQUEST_MSG_SYNC_VT_WORD_LIST) {
        siren_printf(SIREN_ERROR, "message msg is not sync vt word");
        return -3;
    }

    int total_len = message->len;
    char *p = message->data;
    int num = -1;

    memcpy((char *)&num, p, sizeof(int));
    p += sizeof(int) * 4;
    char *k = p;
    //siren_printf(SIREN_INFO, "vt_words contains %d", num);

    if (total_len < (int)sizeof(UnpackedVTConfig) * num) {
        siren_printf(SIREN_ERROR, "len is %d cannot have so many configs", total_len);
        return -6;
    }
    
    for (int i = 0; i < num; i++) {
        siren_vt_word vt;
        UnpackedVTConfig *unpackedVTConfig = (UnpackedVTConfig *)k;
        int total_len = unpackedVTConfig->total_len;
        p = k;
        k += total_len;
        
        vt.vt_type = unpackedVTConfig->vt_type;
        vt.use_default_config = false;
        vt.alg_config.vt_classify_shield = unpackedVTConfig->vt_classify_shield;
        vt.alg_config.vt_block_avg_score = unpackedVTConfig->vt_block_avg_score;
        vt.alg_config.vt_block_min_score = unpackedVTConfig->vt_block_min_score;
        vt.alg_config.vt_left_sil_det = unpackedVTConfig->vt_left_sil_det;
        vt.alg_config.vt_right_sil_det = unpackedVTConfig->vt_right_sil_det;
        vt.alg_config.vt_local_classify_check = unpackedVTConfig->vt_local_classify_check;
        vt.alg_config.vt_remote_check_with_aec = unpackedVTConfig->vt_remote_check_with_aec;
        vt.alg_config.vt_remote_check_without_aec = unpackedVTConfig->vt_remote_check_without_aec;

        int vt_word_size = unpackedVTConfig->vt_word_size;
        int vt_phone_size = unpackedVTConfig->vt_phone_size;
        int vt_nnet_size = unpackedVTConfig->vt_nnet_path_size;

        char *content;
        p += sizeof(UnpackedVTConfig);
        content = p;

        //siren_printf(SIREN_INFO, "vt word size ->> %d", vt_word_size);
        //siren_printf(SIREN_INFO, "vt phone size ->> %d", vt_phone_size);

        if (*(content + vt_word_size) != '\0') {
            siren_printf(SIREN_ERROR, "vt word end with %d need 0", *(content + vt_word_size));
            return -4;
        }
        vt.vt_word = content;

        p += vt_word_size + 1;
        content = p;
        if (*(content + vt_phone_size) != '\0') {
            siren_printf(SIREN_ERROR, "vt phone end with %d need 0", *(content + vt_phone_size));
            return -7;
        }
        vt.vt_phone = content;

        if (vt_nnet_size == 0) {
            vt.alg_config.nnet_path = "";
        } else {
            p += vt_phone_size + 1;
            content = p;
            if (*(content + vt_nnet_size) != '\0') {
                siren_printf(SIREN_ERROR, "vt nnet path end with %d need 0", *(content + vt_phone_size));
                return -5;
            }
            vt.alg_config.nnet_path = content;
        }

        //add vt config
        vt_words.push_back(vt);        
    }

    return 0;
}

void copyMessage(Message **to, Message *from) {
    if (to == nullptr || from == nullptr) {
        return;
    }

    int len = from->len + sizeof(Message);
    char *p = new char[len];
    memcpy (p, (char *)from, len);
    Message *t = (Message *)p;
    t->data = (char *)p + sizeof(Message);
    *to = (Message *)p;
}

SirenSocketReader::~SirenSocketReader() {
    if (isPrepareOnReadSide) {
        close(epollFD);
    }

    close(socket);
}

void SirenSocketReader::prepareOnReadSideProcess() {
    //close write
    close (channel->sockets[0]);
    epollFD = epoll_create(2);
    SIREN_ASSERT(epollFD >= 0);
    if (epollFD < 0) {
        siren_printf(SIREN_ERROR, "epoll_create failed since %s", strerror(errno));
        SIREN_ASSERT(false);
        return;
    }

    setnonblocking(channel->sockets[1]);
    struct epoll_event eventItemReader;
    memset(&eventItemReader, 0, sizeof(struct epoll_event));
    eventItemReader.events = EPOLLIN;
    eventItemReader.data.fd = channel->sockets[1];
    int result = epoll_ctl(epollFD, EPOLL_CTL_ADD, channel->sockets[1], &eventItemReader);
    if (result != 0) {
        siren_printf(SIREN_ERROR, "epoll_ctl failed since %s", strerror(errno));
        SIREN_ASSERT(false);
        return;
    }

    setnonblocking(channel->sockets[1]);
    isPrepareOnReadSide = true;
}

static bool checkMagic(const char *buff) {
    const char *magic = "aabb";
    bool checkfail = false;
    for (int i = 0; i < 4; i++) {
        if (magic[i] != buff[i]) {
            checkfail = true;
        }
    }

    return !checkfail;
}

static void dumpMessage(Message &temp) {
    siren_printf(SIREN_INFO, "dump with message: magic %c%c%c%c, len %d msg %d data %p",
                 temp.magic[0], temp.magic[1], temp.magic[2], temp.magic[3], temp.len, temp.msg, temp.data);
}

int SirenSocketReader::pollMessage(Message **msg) {
    if (!isPrepareOnReadSide) {
        siren_printf(SIREN_ERROR, "not prepare on read side");
        return SIREN_CHANNEL_NOT_PREPARE;
    }

    //pollOnce'
    for (;;) {
        struct epoll_event item;
        memset(&item, 0, sizeof(struct epoll_event));
        int nfds = epoll_wait(epollFD, &item, 1, -1);
        if (nfds < 0) {
            siren_printf(SIREN_ERROR, "poll failed with %s ", strerror(errno));
            return nfds;
        }

        if (item.data.fd != channel->sockets[1]) {
            siren_printf(SIREN_WARNING, "read fd not this reader side nfds = %d, prev = %d", item.data.fd, channel->sockets[1]);
            continue;
        }
#ifdef CONFIG_DEBUG_CHANNEL
        siren_printf(SIREN_INFO, "read message");
#endif
        Message temp;
        Message *rmsg = nullptr;
        read(channel->sockets[1], (char *)&temp, sizeof(Message));
        if (!checkMagic(temp.magic)) {
            siren_printf(SIREN_ERROR, "check magic failed!!");
            dumpMessage(temp);
            return SIREN_CHANNEL_MAGIC_ERROR;
        }
#ifdef CONFIG_DEBUG_CHANNEL
        siren_printf(SIREN_INFO, "read msg data len %d", temp.len);
#endif
        rmsg = allocateMessage(temp.msg, temp.len);
        if (temp.len != 0) {
            int readlen = temp.len;
            char *offset = rmsg->data;
            for (;;) {
                int t = read(channel->sockets[1], offset, readlen);
                if (t < 0) {
                    if (errno == EAGAIN) {
#ifdef CONFIG_DEBUG_CHANNEL
                        continue;
#endif
                    } else {
                        siren_printf(SIREN_ERROR, "read error %s", strerror(errno));
                    }
                } else {
                    if (t != readlen) {
                        readlen = readlen - t;
                        offset += t;
#ifdef CONFIG_DEBUG_CHANNEL
                        siren_printf(SIREN_INFO, "need read %d have read %d", readlen, t);
#endif
                    } else {
                        break;
                    }
                }
            }
        }

        *msg = rmsg;

        return SIREN_CHANNEL_OK;
    }
}

SirenSocketWriter::~SirenSocketWriter() {
    //close reader
    close (channel->sockets[1]);
}

void SirenSocketWriter::prepareOnWriteSideProcess() {
    close (channel->sockets[1]);
    isPrepareOnWriteSide = true;
}

int SirenSocketWriter::writeMessage(Message *msg) {
    if (!isPrepareOnWriteSide) {
        siren_printf(SIREN_ERROR, "not prepare on write side");
        return SIREN_CHANNEL_NOT_PREPARE;
    }

    if (msg == nullptr)  {
        siren_printf(SIREN_ERROR, "invalid msg");
        return SIREN_CHANNEL_ERROR;
    }

    std::lock_guard<decltype(writeGuard)> l_(writeGuard);
    //siren_printf(SIREN_INFO, "send message %d with len %d", msg->msg, msg->len);
    int t = write (channel->sockets[0], msg, sizeof(Message) + msg->len);
    if (t <= 0) {
        siren_printf(SIREN_ERROR, "write failed with %s", strerror(errno));
    }

    if (t != (int)sizeof(Message) + msg->len) {
        siren_printf(SIREN_ERROR, "write %d, but expect %d",
                     t, msg->len);
    }
    return SIREN_CHANNEL_OK;
}

SirenSocketChannel::SirenSocketChannel(int rmem_, int wmem_) {
    rmem = rmem_;
    wmem = wmem_;
}

SirenSocketChannel::~SirenSocketChannel() {
}

bool SirenSocketChannel::open() {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        siren_printf(SIREN_ERROR, "make socketpair failed");
        return false;
    }

    if (wmem != 0 && rmem != 0) {
        setsockopt(sockets[0], SOL_SOCKET, SO_SNDBUF, &wmem, sizeof(wmem));
        setsockopt(sockets[1], SOL_SOCKET, SO_SNDBUF, &wmem, sizeof(wmem));
        setsockopt(sockets[0], SOL_SOCKET, SO_RCVBUF, &rmem, sizeof(rmem));
        setsockopt(sockets[1], SOL_SOCKET, SO_RCVBUF, &rmem, sizeof(rmem));
    }

    int real_rmem = 0;
    int real_wmem = 0;
    socklen_t len = sizeof(int);

    getsockopt(sockets[0], SOL_SOCKET, SO_SNDBUF, &real_wmem, &len);
    siren_printf(SIREN_INFO, "get sockets[0] wmem to %d", real_wmem);
    getsockopt(sockets[1], SOL_SOCKET, SO_SNDBUF, &real_wmem, &len);
    siren_printf(SIREN_INFO, "get sockets[1] wmem to %d", real_wmem);
    getsockopt(sockets[0], SOL_SOCKET, SO_RCVBUF, &real_rmem, &len);
    siren_printf(SIREN_INFO, "get sockets[0] rmem to %d", real_rmem);
    getsockopt(sockets[1], SOL_SOCKET, SO_RCVBUF, &real_rmem, &len);
    siren_printf(SIREN_INFO, "get sockets[1] rmem to %d", real_rmem);
    //use 0 as writer 1 as reader

    return true;
}

}
