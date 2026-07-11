#include "EpollManager.hpp"

#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

extern volatile sig_atomic_t g_running;

// ─── Constructor / Destructor ───────────────────────────────────────────────

EpollManager::EpollManager() : epollFd_(-1), running_(false) {
  std::memset(events_, 0, sizeof(events_));
}

EpollManager::~EpollManager() {
  // Copiar el mapa para iterar de forma segura (delete puede modificar handlers_)
  std::map<int, AEventHandler*> copy = handlers_;
  handlers_.clear();
  for (std::map<int, AEventHandler*>::iterator it = copy.begin();
       it != copy.end(); ++it) {
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, it->first, NULL);
    delete it->second;
  }
  if (epollFd_ >= 0)
    close(epollFd_);
}

// ─── Inicialización ─────────────────────────────────────────────────────────

void EpollManager::init() {
  epollFd_ = epoll_create(1);
  if (epollFd_ < 0)
    throw std::runtime_error(
        std::string("epoll_create failed: ") + strerror(errno));
}

// ─── Bucle central de eventos ───────────────────────────────────────────────

void EpollManager::run() {
  running_ = true;

  while (running_ && g_running) {
    int nready =
        epoll_wait(epollFd_, events_, MAX_EVENTS, EPOLL_TIMEOUT_MS);

    if (nready < 0) {
      if (errno == EINTR)
        continue;
      std::cerr << "[ERROR] epoll_wait: " << strerror(errno) << std::endl;
      break;
    }

    if (nready == 0) {
      cleanupTimeouts();
      continue;
    }

    for (int i = 0; i < nready; ++i) {
      int fd = events_[i].data.fd;
      uint32_t revents = events_[i].events;

      // Guard: handler pudo ser eliminado durante este ciclo de dispatch
      if (handlers_.count(fd) == 0)
        continue;

      AEventHandler* handler = handlers_[fd];

      if (revents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        handler->onDisconnect();
      else if (revents & EPOLLIN)
        handler->onReadReady();
      else if (revents & EPOLLOUT)
        handler->onWriteReady();
    }

    cleanupTimeouts();
  }
}

void EpollManager::stop() {
  running_ = false;
}

// ─── Gestión de handlers ────────────────────────────────────────────────────

void EpollManager::addHandler(AEventHandler* handler, uint32_t events) {
  struct epoll_event ev;
  std::memset(&ev, 0, sizeof(ev));
  ev.events = events;
  ev.data.fd = handler->getFd();

  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, handler->getFd(), &ev) < 0) {
    std::ostringstream oss;
    oss << "epoll_ctl ADD failed on fd=" << handler->getFd() << ": "
        << strerror(errno);
    throw std::runtime_error(oss.str());
  }
  handlers_[handler->getFd()] = handler;
}

void EpollManager::updateHandlerEvents(AEventHandler* handler,
                                       uint32_t events) {
  struct epoll_event ev;
  std::memset(&ev, 0, sizeof(ev));
  ev.events = events;
  ev.data.fd = handler->getFd();
  epoll_ctl(epollFd_, EPOLL_CTL_MOD, handler->getFd(), &ev);
}

void EpollManager::removeHandler(AEventHandler* handler) {
  int fd = handler->getFd();
  epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL);
  handlers_.erase(fd);
  delete handler;
}

// ─── Limpieza de conexiones expiradas ───────────────────────────────────────

void EpollManager::cleanupTimeouts() {
  time_t now = std::time(NULL);
  std::vector<AEventHandler*> expired;

  for (std::map<int, AEventHandler*>::iterator it = handlers_.begin();
       it != handlers_.end(); ++it) {
    if (it->second->isTimedOut(now))
      expired.push_back(it->second);
  }

  for (size_t i = 0; i < expired.size(); ++i)
    removeHandler(expired[i]);
}
