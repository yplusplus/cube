#ifndef __CUBE_REDIS_COMMAND_H__
#define __CUBE_REDIS_COMMAND_H__

namespace cube {

namespace redis {

class RedisConnection;

class RedisCommand {

public:

    RedisCommand &operator<<(const std::string &str) {
        m_args.push_back(str);
        return *this;
    }
    RedisCommand &operator<<(const char *str) {
        m_args.push_back(str);
        return *this;
    }
    RedisCommand &operator<<(int val) {
        m_args.push_back(std::to_string(val));
        return *this;
    }
    RedisCommand &operator<<(unsigned val) {
        m_args.push_back(std::to_string(val));
        return *this;
    } RedisCommand &operator<<(long long val) {
        m_args.push_back(std::to_string(val));
        return *this;
    }
    RedisCommand &operator<<(unsigned long long val) {
        m_args.push_back(std::to_string(val));
        return *this;
    }
    RedisCommand &operator<<(double val) {
        m_args.push_back(std::to_string(val));
        return *this;
    }

    friend class RedisClient;

private:
    // command arguments
    std::vector<std::string> m_args;
};

class RedisCommands {

public:
    RedisCommands &Add(const RedisCommand &cmd) {
        m_cmds.push_back(cmd);
        return *this;
    }

    friend class RedisClient;

private:
    // commands
    std::vector<RedisCommand> m_cmds;
};

}

}

#endif
