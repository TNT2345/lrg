#include "match.h"
int match(const char * s, size_t len, const char * pattern, int is_ignore){
    size_t pattern_len = 0;
    while(pattern[pattern_len] != '\0'){
        pattern_len++;
    }
    if (pattern_len == 0) return 1;//特殊判断，我又忘了
    if(pattern_len > len) return 0;//短路判断
    for(size_t i = 0; i <= len - pattern_len; i++){
        size_t pattern_index = 0;
        size_t j = i;
        while(pattern_index < pattern_len){
            if(is_ignore == 0){
                if(s[j] != pattern[pattern_index]) break;
            }
            else{
                if(s[j] >= 'A' && s[j] <= 'Z'){
                    if(s[j] != pattern[pattern_index] && (s[j] + 32) != pattern[pattern_index]){
                        break;
                    }
                }
                else if(s[j] >= 'a' && s[j] <= 'z'){
                    if(s[j] != pattern[pattern_index] && (s[j] - 32) != pattern[pattern_index]){
                        break;
                    }
                }
                else{
                    if(s[j] != pattern[pattern_index]) break;
                }
            }
            j++;
            pattern_index++;
        }
        if(pattern_index == pattern_len) return 1;
    }
    return 0;
}