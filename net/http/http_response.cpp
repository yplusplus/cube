#include "http_response.h"

#include "http_util.h"

namespace cube {
    
namespace http {

HTTPResponse::HTTPResponse() 
    : m_parse_state(PARSE_STATUS_LINE) {
}

HTTPResponse::~HTTPResponse() {
}

bool HTTPResponse::KeepAlive() const {
    std::string value = Header("Connection");
    return value == "Keep-Alive" || value == "keep-alive";
}

void HTTPResponse::SetKeepAlive(bool on) {
    if (on) {
        SetHeader("Connection", "Keep-Alive");
    } else {
        SetHeader("Connection", "close");
    }
}

void HTTPResponse::Write(const std::string &str) {
    Write(str.data(), str.length());
}

void HTTPResponse::Write(const char *data, size_t len) {
    m_body.append(data, len);
}

std::string HTTPResponse::ToString() const {
    const std::string crlf("\r\n");
    std::string str;
    str.reserve(1024);
    char buf[256];
    // status line
    str += m_proto + " " + std::to_string((long long)m_status_code) + " " + m_status_message + crlf;

    // set content_length if has body
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

void HTTPResponse::Reset() {
    m_proto = "HTTP/1.1";
    SetStatusCode(HTTPStatus_OK);
    m_headers.clear();
    m_body.clear();
    m_parse_state = PARSE_STATUS_LINE;
}

}

}
