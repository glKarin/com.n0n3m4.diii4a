#include "StdString.h"

static char mytolower(char ch) {
    if (ch >= 'A' && ch <= 'Z')
        return ch + ('a' - 'A');
    return ch;
}
static bool is_part_of(const std::string &text, const std::string &part, bool caseInsensitive, bool asSuffix) {
    int n = text.size(), k = part.size();
    if (k > n)
        return false;
    for (int i = 0; i < k; i++) {
        char partCh = part[i];
        char textCh = (asSuffix ? text[n - k + i] : text[i]);
        if (caseInsensitive) {
            partCh = mytolower(partCh);
            textCh = mytolower(textCh);
        }
        if (partCh != textCh)
            return false;
    }
    return true;
}


namespace stdext {
    void split(std::vector<std::string> &tokens, const std::string &text, const char *delimiters) {
        tokens.clear();
        size_t pos = 0;
        while (true) {
            size_t left = text.find_first_not_of(delimiters, pos);
            if (left == std::string::npos)
                break;
            size_t right = text.find_first_of(delimiters, left);
            if (right == std::string::npos)
                right = text.length();
            tokens.push_back(text.substr(left, right - left));
            pos = right;
        }
    }

    std::string join(const std::vector<std::string> &tokens, const char *separator) {
        if (tokens.empty())
            return std::string();

        std::string res = tokens[0];
        for (size_t i = 1; i < tokens.size(); i++) {
            res += separator;
            res += tokens[i];
        }
        return res;
    }

    void trim(std::string &text, const char *delimiters) {
        size_t beg = text.find_first_not_of(delimiters);
        if (beg == std::string::npos) {
            text.clear();
            return;
        }
        size_t end = text.find_last_not_of(delimiters) + 1;
        if (beg == 0 && end == text.size())
            return;
        text.resize(end);
        text.erase(0, beg);
    }
    std::string trim_copy(const std::string &text) {
        std::string res = text;
        trim(res);
        return res;
    }

    std::string to_lower_copy(const std::string &text) {
        std::string data = text;
        for (size_t i = 0; i < data.size(); i++)
            data[i] = mytolower(data[i]);
        return data;
    }

    bool ends_with(const std::string &text, const std::string &suffix) {
        return is_part_of(text, suffix, false, true);
    }
    bool iends_with(const std::string &text, const std::string &suffix) {
        return is_part_of(text, suffix, true, true);
    }
    bool starts_with(const std::string &text, const std::string &prefix) {
        return is_part_of(text, prefix, false, false);
    }
    bool istarts_with(const std::string &text, const std::string &prefix) {
        return is_part_of(text, prefix, true, false);
    }

    std::string replace_all_copy(const std::string &text, const std::string &from, const std::string &to) {
        std::string res = text;
        size_t pos = 0;
        while (1) {
            pos = res.find(from, pos);
            if (pos == std::string::npos)
                break;
            res.replace(pos, from.length(), to);
            pos += to.length();
        }
        return res;
    }
}
