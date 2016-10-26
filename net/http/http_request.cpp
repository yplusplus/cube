#include "http_request.h"

namespace cube {

namespace http {

HTTPRequest::HTTPRequest() :
    m_parse_state(PARSE_REQUEST_LINE) {
}

HTTPRequest::~HTTPRequest() {
}

bool HTTPRequest::KeepAlive() const {
    const std::string value = Header("Connection");
    if (value == "Keep-Alive" || value == "keep-alive"
            || (value == HTTP_HEADER_NONE && m_proto == "HTTP/1.1"))
        return true;
    return false;
}

void HTTPRequest::SetKeepAlive(bool on) {
    if (on) {
        SetHeader("Connection", "Keep-Alive");
    } else {
        SetHeader("Connection", "close");
    }
}

void HTTPRequest::Write(const std::string &str) {
    Write(str.data(), str.length());
}

void HTTPRequest::Write(const char *data, size_t len) {
    m_body.append(data, len);
}

std::string HTTPRequest::ToString() const {
    std::string crlf("\r\n");
    std::string str;
    char buf[256];

    // request line
    str = m_method + " " + m_url + " " + m_proto + crlf;

    // set content length if has body
    if (m_body.length() > 0) {
        //SetContentLength(m_body.length());
        snprintf(buf, sizeof(buf), "Content-Length: %lu\r\n", m_body.length());
        str += buf;
    }

    for (auto it = m_headers.begin(); it != m_headers.end(); it++) {
        if (it->second.empty()) continue;
        str += it->first + ": " + it->second[0];
        for (size_t i = 1; i < it->second.size(); i++) {
            str += "; " + it->second[i];
        }
        str += crlf;
    }
    str += crlf;

    // set body if has
    if (m_body.length() > 0) {
        str += m_body;
    }

    return str;
}

void HTTPRequest::Reset() {
    m_url.clear();
    m_method.clear();
    m_proto.clear();
    m_headers.clear();
    m_body.clear();
    m_parse_state = PARSE_REQUEST_LINE;
}

}

}
