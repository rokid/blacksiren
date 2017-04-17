
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "siren_net.h"
#include "sutils.h"

namespace BlackSiren {

siren_net_result SirenUDPAgent::prepareRecv() {
    struct sockaddr_in addrto;
    bzero(&addrto, sizeof(struct sockaddr_in));
    addrto.sin_family = AF_INET;
    addrto.sin_addr.s_addr = htonl(INADDR_ANY);
    addrto.sin_port = htons(CONFIG_MONITOR_UDP_PORT);

    recvSocket = -1;
    if ((recvSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        siren_printf(SIREN_ERROR, "create socket failed since %s", strerror(errno));
        return SIREN_NET_FAILED;
    }

    const int opt = 1;
    int nb = 0;
    nb = setsockopt(recvSocket, SOL_SOCKET, SO_BROADCAST, (char *)opt, sizeof(opt));
    if (nb < 0) {
        siren_printf(SIREN_ERROR, "set socket broadcast failed");
        return SIREN_NET_FAILED;
    }
    
    if (bind(recvSocket, (struct sockaddr *)&addrto, sizeof(struct sockaddr_in)) == -1) {
        siren_printf(SIREN_ERROR, "failed to bind socket to local addr");
        return SIREN_NET_FAILED;
    }

    return SIREN_NET_OK;
}

siren_net_result SirenUDPAgent::prepareSend() {
    sendSocket = -1;
    if ((sendSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        siren_printf(SIREN_ERROR, "failed to create send socket since %s", strerror(errno));
        return SIREN_NET_FAILED;
    }

    const int opt = 1;
    int nb = 0;
    nb = setsockopt(sendSocket, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)); 
    if (nb == -1) {
        siren_printf(SIREN_ERROR, "failed to set send sock to broadcast");
        return SIREN_NET_FAILED;
    }

    return SIREN_NET_OK;
}

siren_net_result SirenUDPAgent::pollMessage(UDPMessage &msg) {
    struct sockaddr_in from;
    bzero(&from, sizeof(struct sockaddr_in));
    from.sin_family = AF_INET;
    from.sin_addr.s_addr = htonl(INADDR_ANY);
    from.sin_port = htons(CONFIG_MONITOR_UDP_PORT);
    
    int len = sizeof(sockaddr_in);
    int ret = recvfrom(recvSocket, &msg, sizeof(UDPMessage), 0, 
            (struct sockaddr *)&from, (socklen_t *)&len);
    if (ret <= 0) {
        siren_printf(SIREN_ERROR, "recv from broadcast return <= 0 since %s", strerror(errno));
        return SIREN_NET_FAILED;
    }

    return SIREN_NET_OK;
}

siren_net_result SirenUDPAgent::sendMessage(UDPMessage &msg) {
    return SIREN_NET_OK;
} 

}
