#ifndef __CUBE_STRING_UTIL_H__
#define __CUBE_STRING_UTIL_H__

#include <string>
#include <vector>

namespace cube {

namespace strings {

// make sure dest is null-terminated
char *safe_strncpy(char *dest, const char *src, size_t n);

void InternalAppend(std::string& dst, const char* fmt, va_list ap);
std::string FormatString(const char* fmt, ...);
std::string& FormatString(std::string &dst, const char *fmt, ...);
void FormatAppend(std::string &dst, const char *fmt, ...);

std::string &LeftTrim(std::string &str, const std::string &charlist = " \t\r\n");
std::string &RightTrim(std::string &str, const std::string &charlist = " \t\r\n");
std::string &Trim(std::string &str, const std::string &charlist = " \t\r\n");

void Split(const std::string &str, const std::string &splitor, std::vector<std::string> &split_strs, int split_num = -1, bool remove_empty_str = false);
void Split(const std::string &str, const std::vector<std::string> &splitors, std::vector<std::string> &split_strs, int split_num = -1, bool remove_empty_str = false);

bool BeginWith(const std::string &str, const std::string &prefix);

}

}

#endif
