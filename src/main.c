#include "reader.h"
#include "args.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
//以后需要更改历史遗留问题，改为多文件支持
int main(int argc, char **argv){
    Reader reader;
    Options options;
    int error = 1;
    if(init_options(argc, argv, &options)){
        fprintf(stderr, "usage: lrg [-nic] PATTERN [FILE...]\n");
        return 2;
        //输入非法
    }
    if(init_reader(&reader, &options) == 2){
        new_free(&reader);
        free(options.files);
        return 2;
        //初始化失败
    }
    int result;
    if(options.file_number == 0){
        reader.line_number = 0;
        reader.end = 0;
        reader.match_count = 0;
        result = reader_process(&reader, STDIN_FILENO, "(input)");
        if(result == 2){
            error = 2;
        }
        else if(error == 1 && result == 0){
            error = 0;
        }
    }
    else{
        for(int i = 0; i < options.file_number; i++){
            reader.line_number = 0;
            reader.end = 0;
            reader.match_count = 0;
            char * name = options.files[i];
            int fd = open(name, O_RDONLY, 0);
            if(fd == -1){
                fprintf(stderr, "lrg: %s: %s\n", name, strerror(errno));
                error = 2;
                continue;
            //打开失败
            }
            result = reader_process(&reader, fd, name);
            if(result == 2){
                error = 2;
            }
            else if(error == 1 && result == 0){
                error = 0;
            }
            close(fd);
        }
    }
    new_free(&reader);
    free(options.files);
    return error;
}