#ifndef SIREN_CHANNEL_H_
#define SIREN_CHANNEL_H_

#include <sys/epoll.h>

#include <stdlib.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <functional>

#include "sutils.h"

namespace BlackSiren {

enum {
    SIREN_CHANNEL_OK = 0,
    SIREN_CHANNEL_ERROR_FD,
    SIREN_CHANNEL_MAGIC_ERROR,
    SIREN_CHANNEL_INVALID_LEN,
    SIREN_CHANNEL_NOT_PREPARE,
    SIREN_CHANNEL_ERROR,
};




struct Message {
    Message () {
        magic[0] = 'a';
        magic[1] = 'a';
        magic[2] = 'b';
        magic[3] = 'b';
        data = nullptr;
    }

    Message(int msg_) : msg(msg_), len(0){
        magic[0] = 'a';
        magic[1] = 'a';
        magic[2] = 'b';
        magic[3] = 'b';
        data = nullptr;
    }
    
    char magic[4];
    char padding[4];
    int msg;
	int len;
    char *data;
}; 

Message* allocateMessage(int msg, int len);
struct InterstedResponse {
    Message message;
    std::function<void(int)> callback;
};

class SirenSocketChannel;
class SirenSocketReader {
public:
    SirenSocketReader(SirenSocketChannel *channel_) :
        channel(channel_) { }
    ~SirenSocketReader();
    
    void prepareOnReadSideProcess();
    int pollMessage(Message **msg);
private:
    bool isPrepareOnReadSide = false;
    int socket;
    int epollFD;
    SirenSocketChannel *channel;
};

class SirenSocketWriter {
public:
    SirenSocketWriter(SirenSocketChannel *channel_) :
        channel(channel_) {}
    ~SirenSocketWriter();

    void prepareOnWriteSideProcess();
    int writeMessage(Message *message);
private:
    std::mutex writeGuard;
    bool isPrepareOnWriteSide = false;
    int socket;
    SirenSocketChannel *channel;
};

class SirenSocketChannel {
public:
    SirenSocketChannel(int rmem_ = 0, int wmem_ = 0);
    ~SirenSocketChannel();

    bool open();
    
    friend class SirenSocketReader;
    friend class SirenSocketWriter; 
private:
    int sockets[2];
    int rmem;
    int wmem;
};



}

#endif
