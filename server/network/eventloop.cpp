#include "eventloop.h"
#include <cstring>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

const int EpollSize=1000;
const int EventsSize=1024;

EventLoop::EventLoop()
    : m_epollfd(epoll_create1(EPOLL_CLOEXEC)),
    _events(EventsSize)
{
    if (m_epollfd < 0) {
        perror("EventLoop epoll_create1");
        exit(1);
    }
}

EventLoop::~EventLoop() {
    close(m_epollfd);
}

void EventLoop::run() {
    while (true) {
        int num = epoll_wait(m_epollfd, _events.data(), _events.size(), -1);
        if (num < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (int i = 0; i < num; ++i) {
            Channel* ch = static_cast<Channel*>(_events[i].data.ptr);
            ch->handleEvent(_events[i].events);
        }
    }
}

void EventLoop::addChannel(Channel* ch) {
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = ch->getEvents();
    ev.data.ptr = ch;

    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, ch->getfd(), &ev) < 0) {
        std::cerr << "EventLoop::addChannel epoll_ctl ADD failed fd=" << ch->getfd() << std::endl;
    } else {
        _channels[ch->getfd()] = ch;
    }
}

void EventLoop::updateChannel(Channel* ch) {
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = ch->getEvents();
    ev.data.ptr = ch;
    if (epoll_ctl(m_epollfd, EPOLL_CTL_MOD, ch->getfd(), &ev) < 0) {
        std::cerr << "EventLoop::updateChannel epoll_ctl MOD failed fd=" << ch->getfd() << std::endl;
    }
}

void EventLoop::removeChannel(Channel* ch) {
    if (_channels.find(ch->getfd()) != _channels.end()) {
        epoll_ctl(m_epollfd, EPOLL_CTL_DEL, ch->getfd(), nullptr);
        _channels.erase(ch->getfd());
    }
}