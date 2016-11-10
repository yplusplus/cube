#include "base/log.h"
#include "base/string_util.h"

#include "net/event_loop.h"

#include "http_server.h"
#include "http_connection.h"

using namespace std::placeholders;

#define HTTP_REQUEST_MAX_BODY_SIZE (1024 * 1024) // 1M

namespace cube {

namespace http {

HTTPConnection::HTTPConnection(EventLoop *event_loop, HTTPServer *server, const TcpConnectionPtr &conn, const RequestCallback &request_callback)
    : m_event_loop(event_loop),
    m_server(server),
    m_conn(conn),
    m_request_callback(request_callback) {

    conn->SetConnectCallback(std::bind(&HTTPConnection::OnConnect, this, _1, _2));
    conn->SetDisconnectCallback(std::bind(&HTTPConnection::OnDisconnect, this, _1));
    conn->ReadUntil("\r\n\r\n", std::bind(&HTTPConnection::OnHeaders, this, _1, _2));
}

HTTPConnection::~HTTPConnection() {
    LOG_DEBUG("~HTTPConnection");
}

bool HTTPConnection::SendResponse(const HTTPResponse &response) {
    bool keep_alive = m_server->KeepAlive() && response.KeepAlive();
    return m_conn->Write(response.ToString(),
            std::bind(&HTTPConnection::OnWriteComplete, this, _1, keep_alive));
}

void HTTPConnection::OnConnect(TcpConnectionPtr conn, int status) {
    // do nothing
}

void HTTPConnection::OnDisconnect(TcpConnectionPtr conn) {
    m_server->RemoveConnection(shared_from_this());
}

void HTTPConnection::OnHeaders(TcpConnectionPtr conn, Buffer *buffer) {
    m_request.Reset();

    bool succ = ParseHeaders(buffer);
    if (!succ) {
        LOG_ERROR("conn[%lu] parse headers falied", conn->Id());
        conn->Close();
        return;
    }
    size_t body_len = m_request.ContentLength();
    if (body_len > 0) {
        if (body_len > HTTP_REQUEST_MAX_BODY_SIZE) {
            conn->Close();
        } else {
            conn->ReadAny(std::bind(&HTTPConnection::OnBody, this, _1, _2));
            return;
        }
    }
    HandleRequest();
}

bool HTTPConnection::ParseHeaders(Buffer *buffer) {
    const std::string debug_str(buffer->Peek(), buffer->ReadableBytes());
    const char *eoh = buffer->Find("\r\n\r\n");
    if (eoh == NULL) {
        LOG_ERROR("can not find CRLFCRLF");
        return false;
    }

    const std::string header_str(buffer->Peek(), eoh - buffer->Peek());
    std::vector<std::string> lines;
    ::cube::strings::Split(header_str, "\r\n", lines);
    if (lines.empty()) {
        LOG_ERROR("no request line");
        return false;
    }

    // request line
    // Method URL Proto
    std::vector<std::string> fields;
    ::cube::strings::Split(lines[0], " ", fields);
    if (fields.size() != 3) {
        LOG_ERROR("request line format invalid, %s", lines[0].c_str());
        return false;
    }
    m_request.SetMethod(fields[0]);
    m_request.SetURL(fields[1]);
    m_request.SetProto(fields[2]);
    
    // headers
    for (size_t i = 1; i < lines.size(); i++) {
        const std::string line = lines[i];
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        cube::strings::Trim(key);
        cube::strings::Trim(value);
        m_request.SetHeader(key, value);
    }
    buffer->RetrieveUntil(eoh + 4);
    return true;
}

void HTTPConnection::OnBody(TcpConnectionPtr conn, Buffer *buffer) {
    // TODO Parse body according to ContentType
    size_t body_len = m_request.ContentLength();
    assert(body_len > m_request.Body().length());
    size_t need = body_len - m_request.Body().length();
    if (buffer->ReadableBytes() >= need) {
        m_request.Write(buffer->Peek(), need);
        buffer->Retrieve(need);
        HandleRequest();
    } else {
        m_request.Write(buffer->Peek(), buffer->ReadableBytes());
        buffer->RetrieveAll();
        conn->ReadAny(std::bind(&HTTPConnection::OnBody, this, _1, _2));
    }
}

void HTTPConnection::HandleRequest() {
    if (Closed())
        return;

    m_request_callback(shared_from_this(), m_request);
}

void HTTPConnection::OnWriteComplete(TcpConnectionPtr conn, bool keep_alive) {
    //LOG_DEBUG("conn[%lu] keepalive[%d]", conn->Id(), keep_alive);
    if (!keep_alive) {
        conn->Close();
        return;
    }

    // continue to read request
    conn->ReadUntil("\r\n\r\n", std::bind(&HTTPConnection::OnHeaders, this, _1, _2));
}

void HTTPConnection::Close() {
    if (!Closed())
        m_conn->Close();
}

bool HTTPConnection::Closed() const {
    return m_conn->Closed();
}

}

}
