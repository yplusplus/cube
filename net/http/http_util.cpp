#include "http_util.h"

#include <map>

namespace cube {

namespace http {

class StatusReasons {
    public:
        StatusReasons() {
            m_status_reasons[100] = "Continue";
            m_status_reasons[101] = "Switching Protocols";

            m_status_reasons[200] = "OK";
            m_status_reasons[201] = "Created";
            m_status_reasons[202] = "Accepted";
            m_status_reasons[203] = "Non-Authoritative Information";
            m_status_reasons[204] = "No Content";
            m_status_reasons[205] = "Reset Content";
            m_status_reasons[206] = "Partial Content";
            m_status_reasons[207] = "Multi Status";

            m_status_reasons[300] = "Multiple Choices";
            m_status_reasons[301] = "Moved Permanently";
            m_status_reasons[302] = "Found";
            m_status_reasons[303] = "See Other";
            m_status_reasons[304] = "Not Modified";
            m_status_reasons[305] = "Use Proxy";
            m_status_reasons[306] = "Switch Proxy";
            m_status_reasons[307] = "Temporary Redirect";

            m_status_reasons[400] = "Bad Request";
            m_status_reasons[401] = "Unauthorized";
            m_status_reasons[402] = "Payment Required";
            m_status_reasons[403] = "Forbidden";
            m_status_reasons[404] = "Not Found";
            m_status_reasons[405] = "Method Not Allowed";
            m_status_reasons[406] = "Not Acceptable";
            m_status_reasons[407] = "Proxy Authentication Required";
            m_status_reasons[408] = "Request Time-out";
            m_status_reasons[409] = "Conflict";
            m_status_reasons[410] = "Gone";
            m_status_reasons[411] = "Length Required";
            m_status_reasons[412] = "Precondition Failed";
            m_status_reasons[413] = "Request Entity Too Large";
            m_status_reasons[414] = "Request-URI Too Large";
            m_status_reasons[415] = "Unsupported Media Type";
            m_status_reasons[416] = "Requested range not satisfiable";
            m_status_reasons[417] = "Expectation Failed";
            m_status_reasons[422] = "Unprocessable Entity";
            m_status_reasons[423] = "Locked";
            m_status_reasons[424] = "Failed Dependency";
            m_status_reasons[425] = "Unordered Collection";
            m_status_reasons[426] = "Upgrade Required";
            m_status_reasons[449] = "Retry With";
            m_status_reasons[451] = "Unavailable For Legal Reasons";

            m_status_reasons[500] = "Internal Server Error";
            m_status_reasons[501] = "Not Implemented";
            m_status_reasons[502] = "Bad Gateway";
            m_status_reasons[503] = "Service Unavailable";
            m_status_reasons[504] = "Gateway Time-out";
            m_status_reasons[505] = "HTTP Version not supported";            
            m_status_reasons[506] = "Variant Also Negotiates";
            m_status_reasons[507] = "Insufficient Storage";
            m_status_reasons[509] = "Bandwidth Limit Exceeded";
            m_status_reasons[510] = "Not Extended";
        }

        const std::string &GetReason(int status) const {
            auto it = m_status_reasons.find(status);
            if (it == m_status_reasons.end()) {
                return m_unknown;
            }
            return it->second;
        }

    private:
        static const std::string m_unknown;
        std::map<int, std::string> m_status_reasons;
};

const std::string StatusReasons::m_unknown("Unknown Status");

static const StatusReasons status_reason;

const std::string &HTTPUtil::GetHTTPStatusReason(int status) {
    return status_reason.GetReason(status);
}

}

}
