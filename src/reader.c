#include "reader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "match.h"
#define CAPACITY 2048
//static设置文件私有
static char buf[CAPACITY];
static void oom_error(const char * name){
    write(STDERR_FILENO, "lrg: ", 5);
    write(STDERR_FILENO, name, strlen(name));
    write(STDERR_FILENO, ": Cannot allocate memory\n", 25);
}
static int ensure_capacity(Reader * reader,size_t len){
    while(len >= reader->capacity){
        char * result = realloc(reader->line,reader->capacity * 2);
        if(result == NULL){
            return -1;
            //-1表示没空间了，上层接收返回值然后报错
        }
        reader->line = result;
        reader->capacity *= 2;//容量翻倍
    }
    return 0;
}//进行容量检测，不够进行动态扩容

static int match_action(Reader * reader, char * name){
    char str_stack[64];
    int content;//栈缓冲输出，统一使用write
    reader->match_count++;
    if(reader->reader_options->count_only == 0){
    if(reader->reader_options->line_number){
        if(reader->reader_options->file_number > 1){
            content = snprintf(str_stack, 64, ":%zu:", reader->line_number);
            write(STDOUT_FILENO, name, strlen(name));
            write(STDOUT_FILENO, str_stack, content);
        }
        else{
            content = snprintf(str_stack, 64, "%zu:", reader->line_number);
            write(STDOUT_FILENO, str_stack, content);
        }
    }
    ssize_t write_text = write(STDOUT_FILENO, reader->line, reader->end);
    if(write_text == -1){
        fprintf(stderr, "lrg: %s: %s\n", name, strerror(errno));
        return 2;
    }
    write(STDOUT_FILENO, "\n", 1);
    }
    return 0;
}
static ssize_t pop_line(Reader * reader, int * start_number, ssize_t read_text, char * name){
    //从起点开始检测
    for(int attempt = *start_number; attempt < read_text; attempt++){
        if(buf[attempt] == '\n'){
            size_t length = attempt - *start_number;//尾减头出长度
            if(ensure_capacity(reader, length + reader->end) == -1){
                oom_error(name);
                return 2;
            }
            memcpy(&reader->line[reader->end], buf + *start_number, length);//搬运
            reader->end += length;//统计字节
            reader->line_number++;
            if(match(reader->line, reader->end, reader->reader_options->pattern, reader->reader_options->ignore_case)){
                if(match_action(reader, name) == 2){
                    return 2;
                }
            }
            reader->end = 0;//很好，我又忘记清零了
            *start_number = attempt + 1;
            return 0;
        }
    }
    if(ensure_capacity(reader, read_text - *start_number + reader->end) == -1){
        oom_error(name);
        return 2;
    }
    memcpy(&reader->line[reader->end], buf + *start_number, read_text - *start_number);
    //在列表最后一位把剩下的所有字符全塞进去
    reader->end += (read_text - *start_number);//末尾移动
    *start_number = read_text;//很好，我又忘记更新了
    return 0;
}
static ssize_t feed(Reader * reader, ssize_t read_text, char * name){
    int start_number = 0;
    while(start_number != read_text){
        if(pop_line(reader, &start_number, read_text, name) == 2) return 2;
    }
    return 0;
}
int init_reader(Reader * reader, Options * options){
    reader->end = 0;
    reader->line = malloc(INIT_LINE_CONTAIN * sizeof(char));
    if(reader->line == NULL){
        reader->capacity = 0;
        return 2;
    }
    reader->capacity = INIT_LINE_CONTAIN;
    reader->reader_options = options;
    reader->line_number = 0;
    reader->match_count = 0;
    return 0;
}//初始化结构体
void new_free(Reader * reader){
    free(reader->line);
    reader->line = NULL;
}//统一管理，防止重复释放
int reader_process(Reader * reader, int fd, char * name){
    ssize_t get_number;
    //fd从main里open得来
    char str_stack[64];
    int content;//栈缓冲输出，统一使用write
    while(1){
        get_number = read(fd,buf,CAPACITY);
        if(get_number == 0) break;
        if(get_number == -1){
            if(errno == EINTR) continue;//读取被打断进行重试
            fprintf(stderr, "lrg: %s: %s\n", name, strerror(errno));
            return 2;
        }
        if(feed(reader, get_number, name) == 2){
            return 2;
        }
    }
    if (reader->end > 0) {
        reader->line_number++;
        if(match(reader->line, reader->end, reader->reader_options->pattern, reader->reader_options->ignore_case)){
            if(match_action(reader, name) == 2){
                return 2;
            }
        }
    }//残行处理
    if(reader->reader_options->count_only){
        if(reader->reader_options->file_number > 1){
            content = snprintf(str_stack, 64, ":%zu\n", reader->match_count);
            write(STDOUT_FILENO, name, strlen(name));
            write(STDOUT_FILENO, str_stack, content);
        }
        else{
            content = snprintf(str_stack, 64, "%zu\n", reader->match_count);
            write(STDOUT_FILENO, str_stack, content);
        }
    }
    if(reader->match_count == 0){
        return 1;
    }
    return 0;
}//中控