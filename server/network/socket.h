#ifndef SOCKET_H
#define SOCKET_H

class Socket
{
public:
    Socket(int fd);
    ~Socket();
    int getfd();
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
protected:
    int m_fd;
};

#endif // SOCKET_H
