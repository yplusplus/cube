# cube

## 概述
cube是用C++开发一个网络库，基于Reactor编程模型开发，由事件驱动，使用同步非阻塞IO。

支持TCP，UDP编程，开发者可以基于cube开发任意协议的网络应用。

此外，基于cube的底层接口，实现了HTTP协议，可用于开发HTTP服务端或客户端应用；基于hiredis封装了redis客户端。


## 模块分析
* `EventLoop`

    封装了事件轮询的逻辑，执行定时任务，驱动整个框架的运行

* `Poller`

    对linux epoll的封装。用于添加、修改、删除文件句柄的监听事件

* `Eventor`

    对事件的封装。用户向Eventor中设置需要监听的文件句柄，事件及事件唤醒时的回调函数。将Eventor注册到EventLoop后，由EventLoop进行驱动

* `TcpConnection` & `Buffer` & `Socket`

    Socket是对socket句柄的封装，可用于修改socket属性，以及bind，listen，accept

    TcpConnection是对TCP连接的封装。主要定义了连接的状态；进行连接上的读写；维护连接上的状态变化；允许用户为读写和状态变化注册回调函数；执行用户注册的回调函数

    Buffer是对TcpConnection读写缓冲区的封装

    TcpConnection中包含了Socket和Buffer


* `Timer` & `TimerQueue`

    Timer是对定时任务的封装，定时任务分为两类：一类是到达超时时间后，执行一次然后从任务队列中删除；一类是循环执行，到达超时时间后执行任务，执行后再添加回任务队列，在一定的时间间隔后再次执行

    TimerQueue是任务队列的封装。可以添加、删除定时任务。该队列是个优先队列，按定时任务的超时时间从早到晚排序，每次从队列中取出一个定时任务时，是当前整个队列中最先超时。


## 接口
###cube的运行流程：

`注册监听事件` ---> `监听等待` ---> `等待超时or事件发生` ---> `判断是否执行定时任务` ---> `处理事件：执行回调函数` ---> `修改or删除事件` ---> `监听等待` ......

用户使用cube开发网络应用，需要关注TCP和UDP层面的编程接口和回调函数：

### 回调函数原型

* **typedef std::function\<void(TcpConnectionPtr)\> NewConnectionCallback;**

    用于TCP服务器编程。当accept成功返回后，TcpServer将构造一个新的TcpConnection，用户在可以在连接建立后执行自己的业务逻辑

* **typedef std::function\<void(TcpConnectionPtr)\> DisconnectCallback;**

    可用于TCP服务器和客户端编程。关闭一个TcpConnection，会先注销TcpConnection上的读写回调函数，然后从事件轮询中删除该连接相关的事件，最后将执行该函数。

    因此用户的业务逻辑中不能再处理读写事件（读写回调函数已被注销）。

    最后，在TcpConnection被析构时才会触发关闭socket句柄

* **typedef std::function\<void(TcpConnectionPtr, Buffer \*)\> ReadCallback;**

    可用于TCP服务器和客户端编程。当可读事件发生时，将调用该函数，用户可以从读缓冲区中读取数据并进行处理

* **typedef std::function\<void(TcpConnectionPtr)\> WriteCompleteCallback;**

    可用于TCP服务器和客户端编程。当可写事件发生时，TcpConnection内部将写缓冲区的数据写入socket句柄。写操作结果后，若写缓冲区为空，表示一轮写操作完成，将执行该函数。

### 编程接口
**EventLoop：**封装了事件轮询

```cpp
class EventLoop {
    public:
        // 定时任务中的业务逻辑，执行定时任务实际就是在执行Task
        typedef std::function<void()> Task;

        EventLoop();
        ~EventLoop();

        // 返回指向EventLoop自身的指针，该指针被__thread修饰过，是一个线程局部变量
        static EventLoop *Current();

        // 添加定时任务，接口内部会将Task包装成一个Timer
        void Post(const Task &task);

        // 启动轮询，是一个死循环。只能在轮询线程中调用该函数
        void Loop();

        // 启动一次轮询，用户可以使用LoopOnce来控制轮询次数。只能在轮询线程中调用该函数
        int LoopOnce(int poll_timeout_ms);

        // 停止事件轮询，设置运行运行标记为false，然后通过eventfd唤醒。只能在非轮询线程中调用该函数
        void Stop();

        // 该函数直接透传到Poller的同名函数，在Poller中添加或修改一个Eventor
        // 当Poller中没有注册该fd，则向Poller中添加Eventor
        // 当Poller中已注册该fd，则更新
        // 只能在轮询线程中调用
        void UpdateEvents(Eventor *e);

        // 该函数直接透传到Poller的同名函数，在Poller中删除Eventor
        // 调用该函数时要确保fd已在Poller中注册，否则会被assert
        // 只能在轮询线程中调用
        void RemoveEvents(Eventor *e);



        // 以下接口用于操作定时任务

        // 设置一个任务和超时时间，超过超时时间后，执行该任务
        TimerId RunAt(const Task &task, int64_t expiration_ms);

        // 设置一个从当前时间开始，延后执行的任务
        TimerId RunAfter(const Task &task, int64_t delay_ms);

          // 设置循环执行的任务，从当前时间开始，每间隔interval_ms执行一次
        TimerId RunPeriodic(const Task &task, int64_t interval_ms);

        // 设置循环执行的任务，从expiration_ms开始，每间隔interval_ms执行一次
        TimerId RunPeriodic(const Task &task, int64_t expiration_ms, int64_t interval_ms);

        // 通过定时任务ID从定时任务队列中删除一个任务
        void CancelTimer(TimerId time_id);

        // 通过线程ID判断当前是否处于为轮询线程
        bool IsLoopThread() const { return m_thread_id == std::this_thread::get_id(); }

        // assert是否处于轮询线程中
        void AssertInLoopThread() const;

        // 通过linux eventfd唤醒轮询线程
        void WakeUp();
};
```

**TcpServer：**封装了服务器编程的基本操作和流程，基于cube开发服务器应用时可以包含TcpServer

```cpp
class TcpServer {
    public:
        // TcpServer由EventLoop驱动起来，另外需要提供一个服务器监听地址
        TcpServer(EventLoop *event_loop, const InetAddr &server_addr);

        // 析构TcpServer
        ~TcpServer();

        // 用户通过该接口注册NewConnectionCallback
        void SetNewConnectionCallback(const NewConnectionCallback &cb);

        // 启动TcpServer
        bool Start();

        // 停止TcpServer
        void Stop();

        // 获取服务器监听地址
        const InetAddr &ServerAddr() const { return m_server_addr; }

        // 获取服务器出错信息
        const std::string &ErrMsg() const { return m_err_msg; }
};
```

**TcpConnection：**封装了TCP连接的相关状态和操作，用户用过操作TcpConenction来读取和接受数据并完成自己的业务逻辑

```cpp
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
    public:
        TcpConnection(EventLoop *event_loop, int sockfd, const InetAddr &local_addr, const InetAddr &peer_addr);
        ~TcpConnection();

        // 用于设置ConnectCallback，DisconnectCallback，WriteCompleteCallback
        void SetConnectCallback(const ConnectCallback &cb);
        void SetDisconnectCallback(const DisconnectCallback &cb);
        //void SetWriteCompleteCallback(const WriteCompleteCallback &cb);

        // 返回连接ID
        uint64_t Id() const;

        // 返回本端地址
        const InetAddr &LocalAddr() const { return m_local_addr; }

        // 返回远端地址
        const InetAddr &PeerAddr() const { return m_peer_addr; }

        // 初始化，会注册读时间，若是服务器应用返回的新客户端连接，将执行ConnectCallback
        void Initialize();

        // run callback when the read data's length >= read_bytes
        void ReadBytes(size_t read_bytes, const ReadCallback &cb);

        // run callback when the read data contains delimiter
        void ReadUntil(const std::string &delimiter, const ReadCallback &cb);
        // run callback when the read data's length >= 1
        void ReadAny(const ReadCallback &cb);

        // 向发送缓存区写入数据
        bool Write(const std::string &str);
        bool Write(const char *data, size_t len);
        // 下昂
        bool Write(const std::string &str, const WriteCompleteCallback &cb);
        bool Write(const char *data, size_t len, const WriteCompleteCallback &cb);

        // 关闭连接
        void Close();   // close the connection
        // 延后delay_ms时间后关闭连接
        void CloseAfter(int64_t delay_ms);
        // 判断连接状态是否为已关闭
        bool Closed() const;

        // 监听连接上的读事件
        void EnableReading();
        // 注销监听读事件
        void DisableReading();

        // 获取连接最后的活跃时间。读，写，错误，挂断4种事件的发送都会触发TcpConnection的处理流程
        time_t LastActiveTime() const { return m_last_active_time; }

        // 返回连接的错误信息
        const std::string &ErrMsg() const { return m_err_msg; }
};
```
