#include "string_util.h"

namespace cube {

namespace strings {

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

