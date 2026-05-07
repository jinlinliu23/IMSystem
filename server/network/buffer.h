#ifndef BUFFER_H
#define BUFFER_H

#include <vector>

class Buffer {
public:
    void append(const char* data, size_t len) {
        _buf.insert(_buf.end(), data, data + len);
    }
    size_t readableBytes() const { return _buf.size(); }
    const char* peek() const { return _buf.data(); }
    void retrieve(size_t len) { _buf.erase(_buf.begin(), _buf.begin() + len); }
private:
    std::vector<char> _buf;
};

#endif // BUFFER_H
