#ifndef __CUBE_TCP_CONNECTION_H__
#define __CUBE_TCP_CONNECTION_H__

#include <memory>

#include "callbacks.h"
#include "inet_addr.h"
#include "buffer.h"

namespace cube {

class EventLoop;
class Eventor;
class Socket;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
    public:
        enum ConnState {
            ConnState_Connecting,
            ConnState_Connected,
            ConnState_Disconnected,
        };

        TcpConnection(EventLoop *event_loop, int sockfd, const InetAddr &local_addr, const InetAddr &peer_addr);
        ~TcpConnection();

        // callback settor
        void SetConnectCallback(const ConnectCallback &cb) { m_connect_callback = cb; }
        void SetDisconnectCallback(const DisconnectCallback &cb) { m_disconnect_callback = cb; }
        //void SetWriteCompleteCallback(const WriteCompleteCallback &cb) { m_write_complete_callback = cb; }

        uint64_t Id() const { return m_conn_id; }
        const InetAddr &LocalAddr() const { return m_local_addr; }
        const InetAddr &PeerAddr() const { return m_peer_addr; }

        void Initialize();

        void ReadAny(const ReadCallback &cb);
        void ReadBytes(size_t read_bytes, const ReadCallback &cb);
        void ReadUntil(const std::string &delimiter, const ReadCallback &cb);

        bool Write(const std::string &str);
        bool Write(const char *data, size_t len);
        bool Write(const std::string &str, const WriteCompleteCallback &cb);
        bool Write(const char *data, size_t len, const WriteCompleteCallback &cb);

        void Close();   // close the connection
        bool Closed() const { return m_state == ConnState_Disconnected; }

        void EnableReading();
        void DisableReading();

    private:

        void OnRead();

        void HandleEvents(int revents);
        int HandleConnect();
        void HandleRead();
        void HandleWrite();
        void HandleError();
        void HandleClose();

    private:

        static uint64_t m_next_conn_id; // atomic

        EventLoop *m_event_loop;

        const uint64_t m_conn_id;

        std::unique_ptr<Socket> m_sock;
        std::unique_ptr<Eventor> m_eventor;

        InetAddr m_local_addr;
        InetAddr m_peer_addr;

        ConnState m_state;

        Buffer m_input_buffer;
        Buffer m_output_buffer;

        std::string m_read_delimiter;
        int m_read_bytes;

        ConnectCallback m_connect_callback;
        DisconnectCallback m_disconnect_callback;
        ReadCallback m_read_callback;
        WriteCompleteCallback m_write_complete_callback;
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::weak_ptr<TcpConnection> TcpConnectionWPtr;

}

#endif
