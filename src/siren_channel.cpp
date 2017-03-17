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

Message* Message::allocateMessage(int msg, int len) {
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

        siren_printf(SIREN_INFO, "read message");
        Message temp;
        Message *rmsg = nullptr;

        read(channel->sockets[1], &temp, sizeof(Message));
        if (!checkMagic(temp.magic)) {
            siren_printf(SIREN_ERROR, "check magic failed!!");
            return SIREN_CHANNEL_MAGIC_ERROR;
        }
        siren_printf(SIREN_INFO, "read msg data len %d", temp.len);
        rmsg = Message::allocateMessage(temp.msg, temp.len);
        if (temp.len != 0) {
            read(channel->sockets[1], rmsg->data, rmsg->len); 
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

    siren_printf(SIREN_INFO, "send message %d with len %d", msg->msg, msg->len);
    write (channel->sockets[0], msg, sizeof(Message) + msg->len);
    
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
