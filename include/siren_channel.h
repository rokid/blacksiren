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
    SIREN_CHANNEL_ERROR,
};



struct Message {
    int len;
    int msg;
	void release() {
		if (len > 0) {
			free ((char *)this);
		} else if (len == 0) {
			delete this;
		}
	}
};


struct InterstedResponse {
    Message message;
    std::function<void(int)> callback;
};

struct Request {
    struct Message message;
    char *data;
    void release() {
        message.release();
    }
};

class SirenSocketChannel;
class SirenSocketReader {
public:
    SirenSocketReader(SirenSocketChannel *channel_) :
        channel(channel_) { }
    ~SirenSocketReader();
    
    void prepareOnReadSideProcess();
    int pollMessage(Message **msg, char **data);
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
    int writeMessage(Message *message, char *data);
private:
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
    std::mutex mtx;
};



}

#endif
