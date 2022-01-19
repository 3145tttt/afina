#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <sys/epoll.h>

//HW
#include "afina/Storage.h"
#include "afina/execute/Command.h"
#include "afina/logging/Service.h"
#include "protocol/Parser.h"
#include <deque>

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<spdlog::logger> logger, std::shared_ptr<Afina::Storage> storage) : _socket(s) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        _logger = logger;
    }

    bool isAlive = true;

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;

    std::shared_ptr <spdlog::logger> _logger;
    std::shared_ptr <Afina::Storage> pStorage;
    std::string argument_for_command;
    std::unique_ptr <Execute::Command> command_to_execute;
    std::size_t arg_remains = 0;
    Protocol::Parser parser;

    char client_buffer[4096] = {0};
    size_t _offset = 0;

    std::deque <std::string> out_queue;
    size_t head_offset = 0;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
