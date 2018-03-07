// socket_pool.cc (2014-04-23)
// Li Xinjie (xjason.li@gmail.com)

#include "socket_pool.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <fcntl.h>
#include <assert.h>
#include <sstream>
#include <log4cplus/helpers/loglog.h>
#include <string.h>
#include <errno.h>

namespace qlog {

using namespace log4cplus::helpers;

Peer::Peer(const std::string& host, int port, SocketPool* pool)
    : host_(host), port_(port), pool_(pool)
{
    memset(&addr_, 0, sizeof(addr_));
}

const SocketOptions& Peer::options() const {
    return pool_->options();
}

Socket::Socket(const Peer* peer)
    : peer_(peer),
      socket_(-1)
{
}

Socket::~Socket() {
    Close();
}

bool Socket::Open() {
    assert(socket_ == -1);

    int r = 0;
    int flags = 0;

    socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == -1) {
        std::stringstream errmsg;
        errmsg << "socket(): " << strerror(errno);
        getLogLog().error(errmsg.str());
        goto failed;
    }

    flags = ::fcntl(socket_, F_GETFL, 0);
    if (::fcntl(socket_, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::stringstream errmsg;
        errmsg << "fcntl(): " << strerror(errno);
        getLogLog().error(errmsg.str());
        goto failed;
    }

    if (peer_->options().keep_alive) {
        int one = 1;
        if (::setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, &one,
                    sizeof(one)) < 0) {
            std::stringstream errmsg;
            errmsg << "setsockopt(SOL_SOCKET, SO_KEEPALIVE): "
                << strerror(errno);
            getLogLog().error(errmsg.str());
            goto failed;
        }
    }
    if (peer_->options().cloexec) {
        if (::fcntl(socket_, F_SETFD, FD_CLOEXEC) < 0) {
            std::stringstream errmsg;
            errmsg << "fcntl(FD_CLOEXEC): " << strerror(errno);
            getLogLog().error(errmsg.str());
            goto failed;
        }
    }
    if (peer_->options().snd_timeout > 0) {
        struct timeval tv;
        tv.tv_sec = peer_->options().snd_timeout / 1000;
        tv.tv_usec = (peer_->options().snd_timeout % 1000) * 1000;
        if (::setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &tv,
                    sizeof(tv)) < 0) {
            std::stringstream errmsg;
            errmsg << "setsockopt(SOL_SOCKET, SO_SNDTIMEO): "
                << strerror(errno);
            getLogLog().error(errmsg.str());
            goto failed;
        }
    }
    if (peer_->options().snd_buffer > 0) {
        int size = peer_->options().snd_buffer;
        if (::setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &size,
                    sizeof(size)) < 0) {
            std::stringstream errmsg;
            errmsg << "setsockopt(SOL_SOCKET, SO_SNDBUF): "
                << strerror(errno);
            getLogLog().error(errmsg.str());
            goto failed;
        }
    }
    if (peer_->options().rcv_timeout > 0) {
        struct timeval tv;
        tv.tv_sec = peer_->options().rcv_timeout / 1000;
        tv.tv_usec = (peer_->options().rcv_timeout % 1000) * 1000;
        if (::setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv,
                    sizeof(tv)) < 0) {
            std::stringstream errmsg;
            errmsg << "setsockopt(SOL_SOCKET, SO_RCVTIMEO): "
                << strerror(errno);
            getLogLog().error(errmsg.str());
            goto failed;
        }
    }
    if (peer_->options().rcv_buffer > 0) {
        int size = peer_->options().rcv_buffer;
        if (::setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &size,
                    sizeof(size)) < 0) {
            std::stringstream errmsg;
            errmsg << "setsockopt(SOL_SOCKET, SO_RCVBUF): "
                << strerror(errno);
            getLogLog().error(errmsg.str());
            goto failed;
        }
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr = peer_->addr();
    addr.sin_port = htons(peer_->port());

retry:
    r = ::connect(socket_, (struct sockaddr*)&addr, sizeof(addr));
    if (r < 0) {
        if (errno == EINTR) {
            goto retry;
        } else if (errno == EINPROGRESS) {
        } else {
            std::stringstream errmsg;
            errmsg << "connect(" << peer_->host() << ", "
                << peer_->port() << "): "
                << strerror(errno);
            getLogLog().error(errmsg.str());
            goto failed;
        }
    }
    return true;

failed:
    if (socket_ != -1) {
        Close();
    }
    return false;
}

void Socket::Close() {
    if (socket_ >= 0) {
        ::close(socket_);
        socket_ = -1;
    }
}

bool Socket::Check() {
    if (socket_ >= 0) {
        char buf;
        ssize_t n = ::recv(socket_, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
        if ((n < 0 && errno == EAGAIN) || n > 0) {
            return true;
        }
    }
    return false;
}

SocketOptions::SocketOptions()
    : snd_timeout(-1),
      snd_buffer(-1),
      rcv_timeout(-1),
      rcv_buffer(-1),
      keep_alive(false),
      cloexec(false),
      nonblock(false),
      connect_delay(5000),
      connect_timeout(20),
      poll_interval(20),
      count(8),
      check(false)
{
}

bool Peer::Resolve() {
    struct addrinfo hint = {};
    struct addrinfo* res = NULL;
    hint.ai_family = AF_INET;
    int r = ::getaddrinfo(host_.c_str(), "", &hint, &res);
    if (r != 0) {
        return false;
    }
    addr_ = reinterpret_cast<struct sockaddr_in*>(res->ai_addr)->sin_addr;
    freeaddrinfo(res);
    return true;
}

SocketPool::SocketPool(const SocketOptions& options)
    : options_(options),
      using_sockets_(0)
{
    pthread_cond_init(&cond_, NULL);
}

SocketPool::~SocketPool() {
    pthread_cond_destroy(&cond_);
    std::vector<Socket*>::iterator sit = sockets_.begin();
    for (; sit != sockets_.end(); ++sit) {
        delete (*sit);
    }
    std::vector<Peer*>::iterator pit = peers_.begin();
    for (; pit != peers_.end(); ++pit) {
        delete (*pit);
    }
}

bool SocketPool::AddPeer(const std::string& host, int port) {
    Peer* peer = new Peer(host, port, this);
    if (!peer->Resolve()) {
        delete peer;
        return false;
    }
    peers_.push_back(peer);
    Time now = Time::gettimeofday();
    for (int i = 0; i < options_.count; ++i) {
        Socket* socket = new Socket(peer);
        sockets_.push_back(socket);
        if (socket->Open()) {
            TimedSocket polling_socket(socket, now +
                    Time(options_.connect_timeout / 1000,
                        (options_.connect_timeout % 1000) * 1000));
            polling_sockets_.push_back(polling_socket);
        } else {
            TimedSocket closed_socket(socket, now +
                    Time(options_.connect_delay / 1000,
                        (options_.connect_delay % 1000) * 1000));
            closed_sockets_.push_back(closed_socket);
        }
    }
    return true;
}

Socket* SocketPool::Get(pthread_mutex_t* mtx,
        const log4cplus::helpers::Time& now) {
    if (!closed_sockets_.empty()) {
        log4cplus::helpers::Time limit = now;
        // if (polling_sockets_.empty() && ready_sockets_.empty()
        //         && using_sockets_ == 0) {
        //     if (limit < closed_sockets_.front().time()) {
        //         limit = closed_sockets_.front().time();
        //     }
        // }
        std::list<TimedSocket>::iterator it = closed_sockets_.begin();
        for (; it != closed_sockets_.end();) {
            TimedSocket& timed_socket = *it++;
            if (limit >= timed_socket.time()) {
                Socket* socket = timed_socket.socket();
                closed_sockets_.pop_front();
                if (socket->Open()) {
                    TimedSocket polling_socket(socket, now +
                            Time(options_.connect_timeout / 1000,
                                (options_.connect_timeout % 1000) * 1000));
                    polling_sockets_.push_back(polling_socket);
                } else {
                    TimedSocket closed_socket(socket, now +
                            Time(options_.connect_delay / 1000,
                                (options_.connect_delay % 1000) * 1000));
                    closed_sockets_.push_back(closed_socket);
                }
            } else {
                break;
            }
        }
    }

    if (!polling_sockets_.empty()) {
        if (ready_sockets_.empty()) {
            Poll(now, options_.connect_timeout);
        } else if (now >= poll_time_) {
            Poll(now, 0);
            poll_time_ += Time(options_.poll_interval / 1000,
                    (options_.poll_interval % 1000) * 1000);
        }
    }

    while (ready_sockets_.empty() && using_sockets_) {
        pthread_cond_wait(&cond_, mtx);
    }

    if (!options_.check) {
        if (!ready_sockets_.empty()) {
            Socket* socket = ready_sockets_.front();
            ready_sockets_.pop_front();
            using_sockets_++;
            return socket;
        }
    } else {
        while (!ready_sockets_.empty()) {
            Socket* socket = ready_sockets_.front();
            ready_sockets_.pop_front();
            if (!socket->Check()) {
                socket->Close();
                TimedSocket closed_socket(socket, now +
                        Time(options_.connect_delay / 1000,
                            (options_.connect_delay % 1000) * 1000));
                closed_sockets_.push_back(closed_socket);
            } else {
                using_sockets_ ++;
                return socket;
            }
        }
    }
    return NULL;
}

void SocketPool::Release(Socket* socket) {
    if (socket->socket() >= 0) {
        ready_sockets_.push_back(socket);
    } else {
        TimedSocket closed_socket(socket, Time::gettimeofday() + 
                Time(options_.connect_delay / 1000,
                    (options_.connect_delay % 1000) * 1000));
        closed_sockets_.push_back(closed_socket);
    }
    using_sockets_ --;
    pthread_cond_broadcast(&cond_);
}

void SocketPool::Poll(log4cplus::helpers::Time now, int wait) {
    assert(!polling_sockets_.empty());

    Time start = now;

recheck:
    pfds_.clear();
    ppos_.clear();
    std::list<TimedSocket>::iterator it = polling_sockets_.begin();
    for (; it != polling_sockets_.end(); ++it) {
        TimedSocket& timed_socket = *it;

        struct ::pollfd pfd = {};
        pfd.fd = timed_socket.socket()->socket();
        pfd.events = POLLIN | POLLOUT | POLLPRI;
        pfds_.push_back(pfd);
        ppos_.push_back(it);
    }

retry:
    int r = ::poll(&pfds_[0], pfds_.size(), wait);
    if (r < 0) {
        if (errno == EINTR) {
            goto retry;
        } else {
            std::stringstream errmsg;
            errmsg << "poll: " << strerror(errno);
            getLogLog().error(errmsg.str());
            return;
        }
    } else if (r == 0) {
        // poll timed out
        return;
    }

    if (wait) {
        now = Time::gettimeofday();
    }

    int ready = 0;
    for (size_t idx = 0; idx < pfds_.size(); ++idx) {
        TimedSocket& timed_socket = *ppos_[idx];
        Socket* socket = timed_socket.socket();
        if (pfds_[idx].revents == 0) {
            if (now > timed_socket.time()) {
                // connect time out
                polling_sockets_.erase(ppos_[idx]);
                socket->Close();
                TimedSocket closed_socket(socket, now +
                        Time(options_.connect_delay / 1000,
                            (options_.connect_delay % 1000) * 1000));
                closed_sockets_.push_back(closed_socket);
            }
        } else if (pfds_[idx].revents & POLLOUT) {
            std::stringstream msg;
            msg << "connect " << socket->peer()->host()
                << ":" << socket->peer()->port() << " ok";
            getLogLog().debug(msg.str());

            polling_sockets_.erase(ppos_[idx]);

            if (options_.nonblock == false) {
                int flags = ::fcntl(socket->socket(), F_GETFL, 0);
                if (::fcntl(socket->socket(), F_SETFL, flags & ~O_NONBLOCK) < 0){
                    std::stringstream errmsg;
                    errmsg << "fcntl: " << strerror(errno);
                    getLogLog().error(errmsg.str());

                    socket->Close();
                    TimedSocket closed_socket(socket, now +
                            Time(options_.connect_delay / 1000,
                                (options_.connect_delay % 1000) * 1000));
                    closed_sockets_.push_back(closed_socket);
                    continue;
                }
            }
            ready_sockets_.push_back(socket);
            ready++;

        } else if (pfds_[idx].revents & (POLLIN | POLLPRI)) {
            std::stringstream errmsg;
            errmsg << "connect " << socket->peer()->host()
                << ":" << socket->peer()->port()
                << " failed";
            getLogLog().error(errmsg.str());

            polling_sockets_.erase(ppos_[idx]);

            int err = 0;
            socklen_t len = sizeof(err);
            int r = getsockopt(socket->socket(), SOL_SOCKET, SO_ERROR, &err,
                    &len);
            if (r == 0) {
                errmsg << ": " << strerror(err);
            }
            getLogLog().error(errmsg.str());
            socket->Close();

            TimedSocket closed_socket(socket, now +
                    Time(options_.connect_delay / 1000,
                        (options_.connect_delay % 1000) * 1000));
            closed_sockets_.push_back(closed_socket);
        }
    }

    if (ready) {
        pthread_cond_broadcast(&cond_);
    } else {
        if (wait) {
            int64_t start_ms = start.sec() * 1000 + start.usec() / 1000;
            int64_t now_ms = now.sec() * 1000 + now.usec() / 1000;
            int used_ms = now_ms - start_ms;
            if (used_ms <= 0) {
                used_ms = 1;
            }
            if (used_ms < wait && !polling_sockets_.empty()) {
                start = now;
                wait -= used_ms;
                goto recheck;
            }
        }
    }
}

} // namespace qlog
