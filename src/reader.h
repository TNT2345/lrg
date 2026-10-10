#ifndef READER_H
#define READER_H
//新学的头文件保护，防止多次定义
#include "args.h"
#include <stddef.h>
#include <sys/types.h>
#define INIT_LINE_CONTAIN 256 //初始堆容量
typedef struct
{
    char * line;//指向一块malloc的内存
    size_t end;//最后一个字符的位置
    size_t capacity;//堆空间总容量
    size_t line_number;//行数
    size_t match_count;//是否有匹配行
    Options * reader_options;//存入全局option
} Reader;
//总控Reader结构体
int init_reader(Reader * reader, Options * options);//初始化结构体
void new_free(Reader * reader);//更好的free
int reader_process(Reader *r, int fd, char *name);//中控
#endif