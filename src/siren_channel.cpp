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

int SirenSocketReader::pollMessage(Message **msg, char **data) {
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

        //add lock
        channel->mtx.lock();

        siren_printf(SIREN_INFO, "read message");
        char magic[4];
        read(channel->sockets[1], magic, 4);

        if (!checkMagic(magic)) {
            siren_printf(SIREN_ERROR, "check magic failed!!");
            channel->mtx.unlock();
            return SIREN_CHANNEL_MAGIC_ERROR;
        }

        Message msg_;
        read(channel->sockets[1], (char *)&msg_, sizeof(struct Message));
        siren_printf(SIREN_INFO, "read msg data len %d", msg_.len);
        char *pdata = nullptr;
        Message *rmsg_ = nullptr;

        if (msg_.len == 0) {
            rmsg_ = new Message;
            rmsg_->len = msg_.len;
            rmsg_->msg = msg_.msg;
            *msg = rmsg_;
            *data = nullptr;
        } else if (msg_.len > 0) {
            pdata = (char *)malloc(sizeof(struct Message) + msg_.len);
            read(channel->sockets[1], pdata + sizeof(struct Message), msg_.len);
            rmsg_ = (struct Message *)pdata;
            rmsg_->len = msg_.len;
            rmsg_->msg = msg_.msg;
            *msg = rmsg_;
            *data = (char *)(pdata + sizeof(struct Message));
        } else {
            channel->mtx.unlock();
            *msg = nullptr;
            *data = nullptr;
            return SIREN_CHANNEL_INVALID_LEN;
        }

        channel->mtx.unlock();
        return SIREN_CHANNEL_OK;
    }
}

SirenSocketWriter::~SirenSocketWriter() {
    //close reader
    close (channel->sockets[1]);
}

void SirenSocketWriter::prepareOnWriteSideProcess() {
    close (channel->sockets[1]);
}

int SirenSocketWriter::writeMessage(Message *msg, char *data) {
    if (msg == nullptr || msg->len < 0) {
        siren_printf(SIREN_ERROR, "invalid msg");
        return SIREN_CHANNEL_ERROR;
    }

    if (data == nullptr && msg->len != 0) {
        siren_printf(SIREN_ERROR, "zero len with null data");
        return SIREN_CHANNEL_ERROR;
    }

    siren_printf(SIREN_INFO, "send message %d with len %d", msg->msg, msg->len);

    channel->mtx.lock();
    const char *magic = "aabb";
    write (channel->sockets[0], magic, 4);

    write (channel->sockets[0], (char *)msg, sizeof(struct Message));
    if (msg->len != 0) {
        write (channel->sockets[0], data, msg->len);
    }

    channel->mtx.unlock();
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
