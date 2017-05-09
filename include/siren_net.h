#ifndef SIREN_NET_H_
#define SIREN_NET_H_

#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "siren_config.h"

namespace BlackSiren {
typedef int siren_net_result;

enum {
    SIREN_NET_OK,
    SIREN_NET_FAILED
};

struct UDPMessage {
    char magic[4];
    int len;
    char data[32];
};

class SirenUDPAgent {
public:
    void setupConfig(SirenConfig *config) {
        this->config = config;
    }
    
    siren_net_result prepareRecv();
    siren_net_result prepareSend();
    siren_net_result pollMessage(UDPMessage &msg);    
    siren_net_result sendMessage(UDPMessage &msg);
private:
    SirenConfig *config;

    int sendSocket;
    int recvSocket;
    struct sockaddr_in from;
    struct sockaddr_in addrto;
};
}
#endif
