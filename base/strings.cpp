#include <stdarg.h>
#include <stdlib.h>
#include <cassert>
#include <cstring>

#include "strings.h"

namespace cube {

namespace strings {

char *safe_strncpy(char *dest, const char *src, size_t n) {
    assert(n > 0);
    dest = strncpy(dest, src, n);
    // make sure dest is null-terminated
    dest[n - 1] = '\0';
    return dest;
}

void InternalAppend(std::string& dst, const char* fmt, va_list ap)
{
    // Use 1k bytes for the first try, should be enough for most cases
    static __thread char space[1024];

    int size = sizeof(space);
    char* p = space;
    int result = 0;

    va_list backup_ap;
    va_copy(backup_ap, ap);

    do
    {
        result = vsnprintf(p, size, fmt, ap);
        va_end(ap);

        if ((result >= 0) && result < size)
        {
            // Fit the buffer exactly
            break;
        }

        if (result < 0)
        {
            // Double the size of buffer
            size *= 2;
        }
        else
        {
            // Need result+1 exactly
            size = result + 1;
        }

        p = (char*)(p == space ? malloc(size) : realloc(p, size));
        if (!p)
            throw std::bad_alloc();
        va_copy(ap, backup_ap);
    } while (true);

    dst.append(p, result);

    if (p != space)
        free(p);

    // make coverity happy
    va_end(backup_ap);
}

std::string FormatString(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    std::string result;
    InternalAppend(result, fmt, ap);
    va_end(ap);
    return result;
}

std::string& FormatString(std::string& dst, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dst.clear();
    InternalAppend(dst, fmt, ap);
    va_end(ap);
    return dst;
}       

void FormatAppend(std::string& dst, const char* fmt, ...)
{   
    va_list ap;
    va_start(ap, fmt);
    InternalAppend(dst, fmt, ap);
    va_end(ap);
}  

std::string &LeftTrim(std::string &str, const std::string &charlist /* = " \t\r\n" */) {
    int num = 0;
    bool remove_char[256] = {0};
    for (int i = 0; i < (int)charlist.length(); i++)
        remove_char[(unsigned char)charlist[i]] = true;
    for (int i = 0; i < (int)str.length(); i++) {
        if (!remove_char[(unsigned char)str[i]]) break;
        num++;
    }   
    return str = num < (int)str.length() ? str.substr(num) : ""; 
}

std::string &RightTrim(std::string &str, const std::string &charlist /* = " \t\r\n" */) {
    int num = 0;
    bool remove_char[256] = {0};
    for (int i = 0; i < (int)charlist.length(); i++) 
        remove_char[(unsigned char)charlist[i]] = true;
    for (int i = str.length() - 1; i >= 0; i--) {
        if (!remove_char[(unsigned char)str[i]]) break;
        num++;
    }   
    return str = num < (int)str.length() ? str.substr(0, str.length() - num) : ""; 
}

std::string &Trim(std::string &str, const std::string &charlist /* = " \t\r\n" */) {
    return LeftTrim(RightTrim(str, charlist), charlist);
}

void Split(const std::string &str, const std::string &splitor, std::vector<std::string> &split_strs, int split_num, bool remove_empty_str) {
        Split(str, std::vector<std::string>(1, splitor), split_strs, split_num, remove_empty_str); 
}   

void Split(const std::string &str, const std::vector<std::string> &splitors, std::vector<std::string> &split_strs, int split_num, bool remove_empty_str) {
    split_strs.clear();
    int num = 0;
    size_t pos = 0;
    while ((split_num == -1 || num < split_num) && pos < str.length()) {
        size_t find_pos = 0;
        int which = -1; 
        for (int i = 0; i < (int)splitors.size(); i++) {
            size_t fpos = str.find(splitors[i], pos);
            if (fpos != std::string::npos && (which == -1 || fpos < find_pos)) {
                which = i;
                find_pos = fpos;
            }
        }
        if (which == -1) break;
        std::string s = str.substr(pos, find_pos - pos);
        if (s.length() > 0 || !remove_empty_str) {
            split_strs.push_back(s);
        }
        pos = find_pos + splitors[which].length();
        num++;
    }

    if (pos < str.length()) {
        std::string s = str.substr(pos);
        if (s.length() > 0 || !remove_empty_str) {
            split_strs.push_back(s);
        }
    }
}

}

}

