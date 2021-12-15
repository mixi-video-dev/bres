#include "bres+client_ext.h"
#include <netinet/tcp.h>

#ifndef SOL_TCP
#define SOL_TCP     (IPPROTO_TCP)
#endif
#ifndef evutil_socket_t
#define evutil_socket_t int
#endif

#ifndef __LIBEVENT_V2__
static struct event*
event_new(
    void* base,
    evutil_socket_t sock,
    short flags,
    void (*callback)(evutil_socket_t, short, void *),
    void * user
){
    auto ev = new struct event();
    if (ev) {
        ::event_set(
            ev,
            sock,
            flags,
            callback,
            user
        );
    }
    LOG("event_new(%p : %d)", ev, sock);
    return(ev);
}
#endif


static bres::events_ptr s_pevents = NULL;
//
static void
onRecv(
    evutil_socket_t sock,
    short what,
    void *arg
)
{
    auto cli = (bres::client_ptr)arg;
    std::vector<char> bf(128);
    ssize_t recvlen;
    //
    if (what & EV_READ && cli) {
        if ((recvlen = recv(sock, bf.data(), bf.size(), 0)) <= 0) {
            LOG("recv(%d) payload", recvlen);
            goto err_return;
        }
        if (recvlen > 0) {
            cli->req_.append(bf.data(), recvlen);
            auto len = cli->req_.length();
            if (len > 2 /* CRLF */) {
                if (cli->req_.at(len - 1) == '\n' ||
                    cli->req_.at(len - 2) == '\r')
                {
                    // command separated by crlf
                    if (nlohmann::json::accept(cli->req_)) {
                        auto json = nlohmann::json::parse(cli->req_);
                        if (json.find("type") != json.end()) {
                            auto type = json["type"].get<std::string>();
                            if (!type.compare("recieve/channel/add")) {
                                LOG("add channel : %s ,%s", type.c_str(), cli->req_.c_str());
                                bres::LOCK lock(s_pevents->mtx_);
                                s_pevents->events_.push({
                                    bres::EVENT_TYPE_SUBSCRIBE_ADD_CHANNEL,
                                    cli->req_
                                });
                            } else if (!type.compare("recieve/channel/delete")) {
                                LOG("delete channel : %s ,%s", type.c_str(), cli->req_.c_str());
                                bres::LOCK lock(s_pevents->mtx_);
                                s_pevents->events_.push({
                                    bres::EVENT_TYPE_SUBSCRIBE_DELETE_CHANNEL,
                                    cli->req_
                                });
                            } else if (!type.compare("recieve/channel/property")) {
                                LOG("property channel : %s ,%s", type.c_str(), cli->req_.c_str());
                                bres::LOCK lock(s_pevents->mtx_);
                                s_pevents->events_.push({
                                    bres::EVENT_TYPE_SUBSCRIBE_PROPERTY_CHANNEL,
                                    cli->req_
                                });
                            } else if (!type.compare("resource/static-image/add")) {
                                LOG("add static-image : %s ,%s", type.c_str(), cli->req_.c_str());
                                bres::LOCK lock(s_pevents->mtx_);
                                s_pevents->events_.push({
                                    bres::EVENT_TYPE_RESOURCE_ADD_STATIC_IMAGE,
                                    cli->req_
                                });
                            } else if (!type.compare("resource/static-image/delete")) {
                                LOG("delete static-image : %s ,%s", type.c_str(), cli->req_.c_str());
                                bres::LOCK lock(s_pevents->mtx_);
                                s_pevents->events_.push({
                                    bres::EVENT_TYPE_RESOURCE_DELETE_STATIC_IMAGE,
                                    cli->req_
                                });
                            }
                        }
                    }
                    cli->req_.clear();
                }
            }
        }
        return;
    }
err_return:
    shutdown(sock, SHUT_RDWR);
    close(sock);
    event_del(cli->cli_);
    delete cli;
}
//
static void onAccept(
    evutil_socket_t sock,
    short what,
    void *arg
)
{
    auto base = (struct event_base *)arg;
    struct sockaddr_in addr;
    int one = 1;
    socklen_t addrlen = sizeof(addr);
    //
    auto client = accept(sock, (struct sockaddr*)&addr, &addrlen);

    if (client>0){
        auto flags = fcntl(client,F_GETFL,0);
        if (flags < 0){
            throw std::runtime_error("fcntl(F_GETFL)..");
        }
        fcntl(client,F_SETFL,flags^O_NONBLOCK);
        if (setsockopt(client, SOL_TCP, TCP_NODELAY, &one, sizeof(one)) < 0){
            throw std::runtime_error("nagle off..");
        }
        one = (1024*32);
        if (setsockopt(client, SOL_SOCKET, SO_RCVBUF, &one, sizeof(one)) < 0){
            throw std::runtime_error("recieve buffer size..");
        }
        one = (1024*32);
        if (setsockopt(client, SOL_SOCKET, SO_SNDBUF, &one, sizeof(one)) < 0){
            throw std::runtime_error("send buffer size..");
        }
        auto pcli = new bres::client_t();
        pcli->socket_ = client;
        memcpy(&pcli->addr_, &addr, addrlen);
        pcli->addrlen_ = addrlen;
        pcli->base_ = base;
        auto clievent = event_new(base, client, EV_READ|EV_PERSIST, onRecv, pcli);
        if (clievent){
            pcli->cli_ = clievent;
            if (!event_add(clievent, NULL)){
                char rmt[32]={0};
                inet_ntop(AF_INET, &addr.sin_addr.s_addr, rmt, sizeof(rmt));
                // tcp client is connection
                LOG("tcp connected from. (%s:%u) cli(%p) ev(%p)", rmt, addr.sin_port, pcli, clievent);
            }
        }
    }
}
//
void
bres::telnet_service(
    void* arg
)
{
    s_pevents = (bres::events_ptr)arg;
    struct sockaddr_in addr;
    int addrlen,on = 1;
    bzero(&addr, sizeof(addr));
    addrlen = sizeof(addr);            
    auto accept_sock = -1;
#ifdef __LIBEVENT_V2__
    auto base = event_base_new();
#else
    ::event_init();
#endif
    if ((accept_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        throw std::runtime_error("socket..");
    }
    on = 1;
    if (setsockopt(accept_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
        throw std::runtime_error("SO_REUSEADDR..");
    }
    addr.sin_family    = AF_INET;
    addr.sin_port      = htons(50002);
    inet_pton(AF_INET, "127.0.0.1" ,&addr.sin_addr.s_addr);
    //
    if (bind(accept_sock, (struct sockaddr*)&addr, sizeof(addr))) {
        throw std::runtime_error("bind..");
    }
    auto flags = fcntl(accept_sock, F_GETFL, 0);
    if (flags < 0){
        throw std::runtime_error("fcntl(F_GETFL)..");
    }
    fcntl(accept_sock, F_SETFL, flags|O_NONBLOCK);
    on = 1;
    if (setsockopt(accept_sock, SOL_TCP, TCP_NODELAY, &on, sizeof(on)) < 0){
        throw std::runtime_error("nagle off..");
    }
    on = (1024*32);
    if (setsockopt(accept_sock, SOL_SOCKET, SO_RCVBUF, &on, sizeof(on)) < 0){
        throw std::runtime_error("recieve buffer size..");
    }
    on = (1024*32);
    if (setsockopt(accept_sock, SOL_SOCKET, SO_SNDBUF, &on, sizeof(on)) < 0){
        throw std::runtime_error("send buffer size..");
    }
    if (listen(accept_sock, 1024) < 0){
        throw std::runtime_error("listen..");
    }

#ifdef __LIBEVENT_V2__
    auto ev = event_new(
        base,
        accept_sock,
        EV_READ|EV_PERSIST,
        onAccept,
        base
    );
    // main event
    if (event_add(ev, NULL)) {
        throw std::runtime_error("event_add..");
    }
    while(!bres::quit) {
        auto r = event_base_loop(base, EVLOOP_ONCE);
        if (r == 0){
            // success.
        }else if (r < 0){
            LOG("failed.event_base_loop(%p:%d)", base, r);
        }else{
            usleep(10);
        }
    }
#else
    auto ev = event_new(
        NULL,
        accept_sock,
        EV_READ|EV_PERSIST,
        onAccept,
        NULL
    );
    // main event
    if (event_add(ev, NULL)) {
        throw std::runtime_error("event_add..");
    }
    ::event_dispatch();
#endif
}
