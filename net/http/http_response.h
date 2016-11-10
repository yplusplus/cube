#ifndef __CUBE_HTTP_RESPONSE_H__
#define __CUBE_HTTP_RESPONSE_H__

#include <map>
#include <string>
#include <vector>

#include "http_util.h"

namespace cube {
    
namespace http {

class HTTPResponse {
    public:
        enum ParseState {
            PARSE_STATUS_LINE,
            PARSE_HEADERS,
            PARSE_BODY,
            PARSE_DONE,
        };

        typedef std::map<std::string, std::vector<std::string> > Headers;

        HTTPResponse();
        ~HTTPResponse();

        bool HasParsedDone() const { return m_parse_state == PARSE_DONE; }

        // getter
        const std::string &Proto() const { return m_proto; }
        int StatusCode() const { return m_status_code; }
        const std::string &StatusMessage() const { return m_status_message; }
        const std::string &Header(const std::string &key) const {
            auto it = m_headers.find(key);
            if (it != m_headers.end()) return it->second[0];
            // TODO
            static const std::string None("__CUBE_NONE__");
            return None;
        }
        const std::string &ContentType() const { return Header("Content-Length"); }
        size_t ContentLength() const {
            auto it = m_headers.find("Content-Length");
            size_t content_length = 0;
            if (it != m_headers.end()) {
                int tmp = std::stoi(it->second[0]);
                if (tmp > 0) content_length = tmp;
            }
            return content_length;
        }
        const std::string &Body() const { return m_body; }
        bool KeepAlive() const;

        // setter
        void SetProto(const std::string &proto) { m_proto = proto; }
        void SetStatusCode(int status_code) { 
            m_status_code = status_code;
            m_status_message = HTTPUtil::GetHTTPStatusReason(m_status_code);
        }
        void SetStatusMessage(const std::string status_message) { m_status_message = status_message; }
        void SetHeader(const std::string &key, const std::string &value) {
            auto it = m_headers.find(key);
            if (it == m_headers.end()) {
                m_headers[key].push_back(value);
            } else {
                it->second.push_back(value);
            }
        }
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
        
        // status line
        std::string m_proto;            // "HTTP/1.0"
        int m_status_code;              // 200
        std::string m_status_message;   // "OK"

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
