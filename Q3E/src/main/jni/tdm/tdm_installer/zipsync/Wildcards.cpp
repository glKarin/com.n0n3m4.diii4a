#include "Wildcards.h"
#include <stdint.h>

inline bool CharMatch(char p, char s) {
    if (p == s)
        return true;
    //case-insensitive
    if (uint8_t(p - 'A') <= uint8_t('Z' - 'A'))
        p += 'a' - 'A';
    if (uint8_t(s - 'A') <= uint8_t('Z' - 'A'))
        s += 'a' - 'A';
    return s == p;
}

bool WildcardMatch(const char *pat, const char *str) {
    switch (*pat) {
        case 0:
            return *str == 0;
        case '*':
            return WildcardMatch(pat + 1, str) || (*str && WildcardMatch(pat, str + 1));
        case '?':
            return *str && (*str != '.') && WildcardMatch(pat + 1, str + 1);
        default:
            return CharMatch(*pat, *str) && WildcardMatch(pat + 1, str + 1);
    }
}
