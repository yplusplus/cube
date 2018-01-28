#include <sys/uio.h>
#include <algorithm>

#include "buffer.h"

namespace cube {

const char *Buffer::CRLF = "\r\n";

void Buffer::Append(const std::string &str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const char *data, size_t len) {
    if (WritableBytes() < len)
        MakeSpace(len);
    std::copy(data, data + len, BeginWrite());
    write_index += len;
}

const char *Buffer::FindCRLF() {
    return Find(CRLF, CRLF + 2);
}

const char *Buffer::Find(const char *begin, const char *end) {
    return FindFrom(BeginRead(), begin, end);
}

const char *Buffer::Find(const std::string &delimiter) {
    assert(!delimiter.empty());
    return Find(delimiter.data(), delimiter.data() + delimiter.length());
}

const char *Buffer::FindFrom(const char *from, const char *begin, const char *end) {
    assert(from >= BeginRead());
    assert(from <= BeginWrite());
    const char *found = std::search(from, (const char *)BeginWrite(), begin, end);
    if (found == BeginWrite())
        return NULL;
    return found;
}

ssize_t Buffer::ReadFromFd(int fd) {
    struct iovec iov[2];
    char buf[32 * 1024];
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = WritableBytes();
    iov[1].iov_base = buf;
    iov[1].iov_len = sizeof(buf);
    ssize_t nread = readv(fd, iov, 2);
    if (nread >= 0) {
        if ((size_t)nread <= WritableBytes()) {
            write_index += nread;
        } else {
            const size_t len = nread - WritableBytes();
            write_index += WritableBytes();
            Append(buf, len);
        }
    } else {
        // error
    }
    return nread;
}

void Buffer::Swap(Buffer &rhs) {
    // 交换其中的buffer数组和读写下标
    std::swap(buffer, rhs.buffer);
    std::swap(read_index, rhs.read_index);
    std::swap(write_index, rhs.write_index);
}

}

