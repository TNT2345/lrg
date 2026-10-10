#ifndef MATCH_H
#define MATCH_H
#include <unistd.h>
int match(const char * s, size_t len, const char * pattern, int is_ignore);
//给定已知字符串和长度，找是否有指定子串，有区分大小写开关
#endif