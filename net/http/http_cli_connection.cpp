#include "base/log.h"
#include "base/string_util.h"
#include "net/event_loop.h"
#include "http_cli_connection.h"
#include "http_request.h"

using namespace std::placeholders;

namespace cube {

namespace http {

HTTPClientConnection::HTTPClientConnection(EventLoop *event_loop, TcpConnectionPtr conn)
    : m_event_loop(event_loop),
    m_conn(conn),
    m_requesting(false) {

    m_conn->SetConnectCallback(std::bind(&HTTPClientConnection::OnConnect, this, _1, _2));
    m_conn->SetDisconnectCallback(std::bind(&HTTPClientConnection::OnDisconnect, this, _1));
}

HTTPClientConnection::~HTTPClientConnection() {
    LOG_DEBUG("~HTTPClientConnection");
}

bool HTTPClientConnection::SendRequest(const HTTPRequest &request, const ResponseCallback &response_callback) {
    assert(!m_response_callback);

    if (!m_conn->Write(request.ToString())) {
        return false;
    }

    m_requesting = true;
    m_response_callback = response_callback;
    m_conn->ReadUntil("\r\n\r\n", std::bind(&HTTPClientConnection::OnHeaders, this, _1, _2));
    return true;
}

void HTTPClientConnection::OnConnect(TcpConnectionPtr conn, int status) {
    // connect failed
    if (status != CUBE_OK) {
    }
}

void HTTPClientConnection::OnDisconnect(TcpConnectionPtr conn) {
    // run response callback with NULL response
    if (m_response_callback) {
        //m_event_loop->Post(std::bind(response_callback, shared_from_this(), (const HTTPResponse *)NULL));
        ResponseCallback response_callback = std::move(m_response_callback);
        m_response_callback = NULL;
        m_requesting = false;
        response_callback(shared_from_this(), (const HTTPResponse *)NULL);
    }

    if (m_disconnect_callback)
        m_disconnect_callback(shared_from_this());
}

void HTTPClientConnection::OnHeaders(TcpConnectionPtr conn, Buffer *buffer) {
    bool succ = ParseHeaders(buffer);
    if (!succ) {
        LOG_ERROR("conn[%lu] parse headers falied", conn->Id());
        conn->Close();
        return;
    }
    size_t body_len = m_response.ContentLength();
    if (body_len > 0) {
        conn->ReadBytes(body_len, std::bind(&HTTPClientConnection::OnBody, this, _1, _2));
        return;
    }
    HandleResponse();
}

bool HTTPClientConnection::ParseHeaders(Buffer *buffer) {
    m_response.Reset();

    const char *eoh = buffer->Find("\r\n\r\n");
    if (eoh == NULL) return false;

    const std::string header_str(buffer->Peek(), eoh - buffer->Peek());
    std::vector<std::string> lines;
    ::cube::strings::Split(header_str, "\r\n", lines);
    if (lines.empty()) return false;

    // status line
    // Proto StatusCode StatusMessage
    std::vector<std::string> fields;
    ::cube::strings::Split(lines[0], " ", fields, 2);
    if (fields.size() != 3) {
        return false;
    }
    m_response.SetProto(fields[0]);
    m_response.SetStatusCode(std::stoi(fields[1]));
    m_response.SetStatusMessage(fields[2]);

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
        m_response.SetHeader(key, value);
    }
    buffer->RetrieveUntil(eoh + 4);
    return true;
}

void HTTPClientConnection::OnBody(TcpConnectionPtr conn, Buffer *buffer) {
    // TODO Parse body according to ContentType
    size_t body_len = m_response.ContentLength();
    if (buffer->ReadableBytes() < body_len) {
        conn->Close();
        return;
    }
    m_response.Write(buffer->Peek(), body_len);
    buffer->Retrieve(body_len);
    HandleResponse();
}

void HTTPClientConnection::HandleResponse() {
    assert(m_response_callback);

    m_requesting = false;
    ResponseCallback response_callback = std::move(m_response_callback);
    m_response_callback = NULL;
    response_callback(shared_from_this(), &m_response);
}

}

}
