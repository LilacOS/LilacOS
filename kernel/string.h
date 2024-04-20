#include "types.h"

/**
 * 计算字符串 str 的长度，不包括结尾的 '\0'
 */
size_t strlen(const char *str) {

    size_t len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

/**
 * 比较两个字符串
 * 若 str1 字典序比 str2 小，返回 -1
 * 若 str1 字典序比 str2 大，返回 1
 * 若 str1 与 str2 相等，返回 0
 */
int strcmp(char *str1, char *str2) {
    while (*str1 != '\0' && *str2 != '\0') {
        if (*str1 < *str2) {
            return -1;
        } else if (*str1 > *str2) {
            return 1;
        }
        str1++;
        str2++;
    }

    // 检查字符串的长度
    if (*str1 == '\0' && *str2 == '\0') {
        return 0; // 两个字符串相等
    } else if (*str1 == '\0') {
        return -1; // str1 较短
    } else {
        return 1; // str2 较短
    }
}