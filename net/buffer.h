#ifndef __CUBE_BUFFER_H__
#define __CUBE_BUFFER_H__

#include <algorithm>
#include <vector>
#include <string>
#include <assert.h>

namespace cube {

class Buffer {
    public:
        static const size_t INITIAL_SIZE = 1024;        // 1K

        Buffer() : buffer(INITIAL_SIZE), read_index(0), write_index(0) { }

        void Append(const std::string &str);
        void Append(const char *data, size_t len);

        const char *Peek() { return BeginRead(); }

        const char *FindCRLF();
        const char *Find(const char *begin, const char *end);
        const char *Find(const std::string &delimiter);
        const char *FindFrom(const char *from, const char *begin, const char *end);

        void Retrieve(size_t n) {
            assert(ReadableBytes() >= n);
            read_index += n;
        }

        void RetrieveUntil(const char *end) {
            assert(end >= BeginRead());
            assert(end <= BeginWrite());
            Retrieve(end - BeginRead());
        }

        std::string RetrieveAllToString() {
            std::string str(Peek(), ReadableBytes());
            RetrieveAll();
            return str;
        }

        void RetrieveAll() {
            read_index = 0;
            write_index = 0;
        }

        size_t ReadableBytes() const { return write_index - read_index; }
        size_t WritableBytes() const { return Cap() - write_index; }
        size_t Cap() const { return buffer.size(); }

        ssize_t ReadFromFd(int fd);

    private:
        static const char *CRLF;

        char *Begin() { return &*buffer.begin(); }

        char *BeginRead() { return Begin() + read_index; }

        char *BeginWrite() { return Begin() + write_index; }

        // MakeSpace(n) to guarantee space for another n bytes.
        // After MakeSpace(n), at least n bytes can be written to the
        // buffer without another allocation.
        void MakeSpace(size_t n) {
            if (buffer.size() - ReadableBytes() >= n) {
                size_t readable = ReadableBytes();
                std::copy(BeginRead(), BeginWrite(), Begin()); 
                read_index = 0;
                write_index = read_index + readable;
            } else {
                // TODO
                buffer.resize(write_index + n);
            }
        }

    private:
        // TODO use char * instead of std::vector
        std::vector<char> buffer;
        size_t read_index;
        size_t write_index;
};

}

#endif

