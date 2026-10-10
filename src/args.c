#include "args.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static int is_option(const char * s){
    return s[0] == '-' && s[1] != '\0';
}
//初步判定是不是选项，同时排除只含一个-的情况

//原来写的太烂了，我给重构了
int init_options(int argc, char **argv, Options * options){
    //一开始先初始化！！！
    options->count_only = 0;
    options->ignore_case = 0;
    options->line_number = 0;
    options->file_number = 0;
    options->files = malloc(argc * sizeof(char*));
    if (options->files == NULL) return -2;
    options->pattern = NULL;
    int can_be_op = 1;
    for(int element = 1; element < argc; element++){
        char * token = argv[element];
        if(can_be_op && is_option(token)){
            for (int i = 1; token[i] != '\0'; i++){
                if(token[i] == 'n') options->line_number = 1;
                else if(token[i] == 'i') options->ignore_case = 1;
                else if(token[i] == 'c') options->count_only = 1;
                else{
                    free(options->files);
                    return -1;
                }
            }
            continue;
        }
        else can_be_op = 0;
        if(options->pattern == NULL){
            options->pattern = token;
            continue;
        }
        else{
            options->files[options->file_number] = token;
            options->file_number++;
        }
    }
    if(options->pattern == NULL){
        free(options->files);
        return -1;
        //忘记了会有人不输PATTERN
    }
    return 0;
}