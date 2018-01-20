#ifndef __CUBE_HTTP_REQUEST_H__
#define __CUBE_HTTP_REQUEST_H__

#include <map>
#include <string>
#include <vector>

#include "http_util.h"

namespace cube {

namespace http {

class HTTPRequest {
    public:
        enum ParseState {
            PARSE_REQUEST_LINE,
            PARSE_HEADERS,
            PARSE_BODY,
            PARSE_DONE,
        };

        typedef std::map<std::string, std::string> Headers;

        HTTPRequest();
        ~HTTPRequest();

        bool HasParsedDone() const { return m_parse_state == PARSE_DONE; }

        // getter
        const std::string &URL() const { return m_url; }
        const std::string &Method() const { return m_method; }
        const std::string &Proto() const { return m_proto; }
        const std::string &Header(const std::string &key) const {
            auto it = m_headers.find(key);
            if (it != m_headers.end()) return it->second;
            return HTTP_HEADER_NONE;
        }
        const std::string &ContentType() const { return Header("Content-Type"); }
        size_t ContentLength() const {
            auto it = m_headers.find("Content-Length");
            size_t content_length = 0;
            if (it != m_headers.end()) {
                int tmp = stoi(it->second);
                if (tmp > 0) content_length = tmp;
            }
            return content_length;
        }
        const std::string &Body() const { return m_body; }
        bool KeepAlive() const;

        // setter
        void SetURL(const std::string &url);
        void SetMethod(const std::string &method) { m_method = method; }
        void SetProto(const std::string &proto) { m_proto = proto; }
        void SetHeader(const std::string &key, const std::string &value) { m_headers[key] = value; }
        void SetContentType(const std::string &value) { SetHeader("Content-Type", value); }
        void SetContentLength(size_t len) { SetHeader("Content-Length", std::to_string((unsigned long long)len)); }
        void SetKeepAlive(bool on);

        void Write(const std::string &str);
        void Write(const char *data, size_t len);

        std::string ToString() const;

        ParseState State() const { return m_parse_state; }
        void SetState(ParseState state) { m_parse_state = state; }

        void Reset();

    private:

        // request line
        std::string m_url;      // "/"
        std::string m_method;   // "GET", "POST", etc.
        std::string m_proto;    // "HTTP/1.1"

        // Headers contains the request header fields either received
        // by the server or to be sent by the client.
        Headers m_headers;

        // body
        std::string m_body;

        ParseState m_parse_state;
};

}

}

#endif
