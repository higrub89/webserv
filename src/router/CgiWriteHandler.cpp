#include "CgiWriteHandler.hpp"

#include "EpollManager.hpp"

CgiWriteHandler::CgiWriteHandler(int stdin_fd, EpollManager* epoll_manager,
                                 const std::vector<char>& body)
  : AEventHandler(stdin_fd),
    epollManager_(epoll_manager),
    bodyBuffer_(body),
    bytesWritten_(0) {
  epollManager_->addHandler(this, EPOLLOUT | EPOLLRDHUP);
}
