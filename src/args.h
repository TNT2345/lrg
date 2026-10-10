#ifndef ARGS_H
#define ARGS_H
typedef struct {
    int line_number;//-n 在每行前输出行号（从 1 开始，每个文件独立计数）
    int ignore_case;//-i 匹配时忽略 ASCII 字母的大小写
    int count_only;//-c 只输出匹配行的数量，不输出行内容（与 -n 同时出现时忽略 -n）
    const char * pattern;//查找的目标字符串
    char **files;//文件名称
    int file_number;//文件个数
} Options;
int init_options(int argc, char **argv, Options * options);//初始化选项
#endif