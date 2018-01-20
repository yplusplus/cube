#ifndef __CUBE_HTTP_UTIL_H__
#define __CUBE_HTTP_UTIL_H__

#include <string>

namespace cube {

namespace http {
    
enum {
    // 1xx
    HTTPStatus_Continue                         = 100,
    HTTPStatus_Switching_Protocols              = 101,

    // 2xx
    HTTPStatus_OK                               = 200,
    HTTPStatus_Created                          = 201,
    HTTPStatus_Accepted                         = 202,
    HTTPStatus_Non_Authoritative_Information    = 203,
    HTTPStatus_No_Content                       = 204,
    HTTPStatus_Reset_Content                    = 205,
    HTTPStatus_Partial_Content                  = 206,
    HTTPStatus_Multi_Status                     = 207,

    // 3xx
    HTTPStatus_Multiple_Choices                 = 300,
    HTTPStatus_Moved_Permanently                = 301,
    HTTPStatus_Found                            = 302,
    HTTPStatus_See_Other                        = 303,
    HTTPStatus_Not_Modified                     = 304,
    HTTPStatus_Use_Proxy                        = 305,
    HTTPStatus_Switch_Proxy                     = 306,
    HTTPStatus_Temporary_Redirect               = 307,

    // 4xx
    HTTPStatus_Bad_Request                      = 400,
    HTTPStatus_Unauthorized                     = 401,
    HTTPStatus_Payment_Required                 = 402,
    HTTPStatus_Forbidden                        = 403,
    HTTPStatus_Not_Found                        = 404,
    HTTPStatus_Method_Not_Allowed               = 405,
    HTTPStatus_Not_Acceptable                   = 406,
    HTTPStatus_Proxy_Authentication_Required    = 407,
    HTTPStatus_Request_Timeout                  = 408,
    HTTPStatus_Conflict                         = 409,
    HTTPStatus_Gone                             = 410,
    HTTPStatus_Length_Required                  = 411,
    HTTPStatus_Precondition_Failed              = 412,
    HTTPStatus_Request_Entity_Too_Large         = 413,
    HTTPStatus_Request_URI_Too_Long             = 414,
    HTTPStatus_Unsupported_Media_Type           = 415,
    HTTPStatus_Request_Range_Not_Satisfiable    = 416,
    HTTPStatus_Expectation_Failed               = 417,
    HTTPStatus_Unprocessable_Entity             = 422,
    HTTPStatus_Locked                           = 423,
    HTTPStatus_Failed_Dependency                = 424,
    HTTPStatus_Unordered_Collection             = 425,
    HTTPStatus_Upgrade_Required                 = 426,
    HTTPStatus_Retry_With                       = 449,
    HTTPStatus_Unavailable_For_Legal_Reasons    = 451,

    // 5xx
    HTTPStatus_Internal_Server_Error            = 500,
    HTTPStatus_Not_Implemented                  = 501,
    HTTPStatus_Bad_Gateway                      = 502,
    HTTPStatus_Service_Unavailable              = 503,
    HTTPStatus_Gateway_Timeout                  = 504,
    HTTPStatus_HTTP_Version_Not_Supported       = 505,
    HTTPStatus_Variant_Also_Negotiates          = 506,
    HTTPStatus_Insufficient_Storage             = 507,
    HTTPStatus_Bandwidth_Limit_Exceeded         = 509,
    HTTPStatus_Not_Extended                     = 510,
};

const std::string HTTP_HEADER_NONE("__Cube_Header_None__");
const std::string HTTP_VERSION_1_0("HTTP/1.0");
const std::string HTTP_VERSION_1_1("HTTP/1.1");

class HTTPUtil {
    public:
        static const std::string &GetHTTPStatusReason(int status);

    private:
};

}

}
#endif
