#ifndef AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H

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
namespace MTnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<spdlog::logger> logger, std::shared_ptr<Afina::Storage> storage, size_t max = 10) : _socket(s) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        _logger = logger;
        max_size = max;
    }

    bool isAlive = true;

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class Worker;
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;

    std::shared_ptr <spdlog::logger> _logger;
    std::shared_ptr <Afina::Storage> pStorage;
    std::string argument_for_command;
    std::unique_ptr <Execute::Command> command_to_execute;
    std::size_t arg_remains = 0;
    Protocol::Parser parser;

    std::deque <std::string> out_queue;
    size_t max_size;
    size_t head_offset = 0;

    std::mutex queue_push_mutex;
};

} // namespace MTnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
