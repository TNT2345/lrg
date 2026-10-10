# lr-grep交付文档

## 一、前言

这是我第一次自己从无到有搭建起一个项目，说实话，这个过程其实挺艰难的，因为在构建这个项目的时候，我做不到一步到位把框架完美搭好，只能先自己写出几个小板块自己先测试测试，先自己手动玩玩，学习时简单测试了一些功能用法

功能的迭代所带来的问题一开始我确实没有考虑过，之前在完工前的小版本一直是单文件支持，并且不支持`stdin`，经过了一段时间的重构才加入这两项支持，并且修复若干遗留bug

并且，修bug也是一个及其漫长的过程，每个板块单独就会出好几个bug，然后把他们链接在一块之后又出现了一堆匪夷所思的bug  ~~甚至修了一个多出两个，有丝分裂，也是让我自己尝到石山的威力了~~

## 二、构建方法

我使用了Makefile来构建自己的项目

```makefile
lrg: src/main.c src/reader.c src/args.c src/match.c src/reader.h src/args.h src/match.h
	gcc -std=gnu11 -Wall -g src/main.c src/reader.c src/args.c src/match.c -o lrg
clean:
	rm -f lrg
```

这个项目结构并不复杂，写在一行进行构建完全可以

**构建环境：Linux,gcc(C99+)**

**构建命令：先跳转到`lrg`的目录，使用`cd lrg`，然后再输入`make`进行构建，会产生`lrg`的构建文件**

## 三、使用说明

满足以下格式

```shell
lrg [-nic] PATTERN [FILE...]
```

具体支持以下调用形式

```shell
lrg PATTERN FILE               # 基本匹配
lrg -n PATTERN FILE            # 单个文件
lrg -ni PATTERN FILE FILE2     # 选项可以合并，等价于 -n -i
lrg PATTERN                    # 省略 FILE 时读取标准输入
```

其中`[-nic]`选项含义为

| 选项 | 含义                                            |
| ---- | ----------------------------------------------- |
| `-n` | 在每行前输出行号（从 1 开始，每个文件独立计数） |
| `-i` | 匹配时忽略 ASCII 字母的大小写                   |
| `-c` | 只输出匹配行数（与 -n 同时则忽略 -n）           |

**运行原则遵循：**

- **选项可合并**
- **省略 FILE 时读标准输入**
- **PATTERN 是字面量，非正则**
- **空 pattern 匹配所有行**

## 四、运行示例

```shell
tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg error tests/sample.log
error: give up

tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg -n error tests/sample.log
5:error: give up

tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg -i ErRor tests/sample.log
ERROR: retry later
error: give up

tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg -c boot tests/sample.log
2

tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg -ni hello tests/case.txt
1:Hello
2:HELLO
3:hello
4:HeLLo

tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg -n -i hello tests/case.txt
1:Hello
2:HELLO
3:hello
4:HeLLo

tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg -nxy hello tests/case.txt
usage: lrg [-nic] PATTERN [FILE...]

tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg -xxx xxx; echo "exit=$?"
usage: lrg [-nic] PATTERN [FILE...]
exit=2

tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg -n hi
aaaaaaasafadgdsgs
hihi
2:hihi

tnt2345@tnt2345-VirtualBox:~/lrg$ cat tests/notes.txt | ./lrg -n good
2:all good

tnt2345@tnt2345-VirtualBox:~/lrg$ ./lrg -n "" tests/sample.log
1:boot ok
2:hello world
3:ERROR: retry later
4:boot ok again
5:error: give up
```

## 五、设计说明

### 一、板块划分

要把这个项目构建起来，首先要知道这个项目应该由哪几个部分组成

#### 中控部分

**用于接收解析后的输入指令，并且负责文件的打开，关闭，将解析后的指令传递给文本读取模块，同时统一管理报错信息**

#### 解析器

**判定用户在终端输入格式是否合法，同时将用户的合法输入拆分成：选项、模式、文件，三部分，以此来传递给中控部分**

#### 查找器

**用于查找给定字符串是否含有目标子字符串，并且接收解析器的结果，判定是否忽略大小写**

#### 文本读取

**读取给定文本的内容，将文本按照行进行分隔，并且将行传递给查找器，接收查找器返回的结果，最后进行输出**

我建议使用这个顺序进行构建：**解析器、查找器、中控部分、文本读取**

因为解析器和查找器不存在对其他板块的依赖，而文本读取**及其依赖于**中控部分的文件打开与关闭，解析器的结果，查找器的参与

~~一开始由于学习文本读取，导致一开始的构建方向与现在所描述的相反，带来了项目上的不便，下面讲解的时候将按照我的建议顺序进行讲解~~

### 二、解析器 -> `args.c,args.h`

为了给文本读取部分便捷的传递读取到的数据，需要建立一个结构体**`Options`**来管理这些数据

```c
typedef struct {
    int line_number;//-n 在每行前输出行号（从 1 开始，每个文件独立计数）
    int ignore_case;//-i 匹配时忽略 ASCII 字母的大小写
    int count_only;//-c 只输出匹配行的数量，不输出行内容（与 -n 同时出现时忽略 -n）
    const char * pattern;//查找的目标字符串
    char **files;//文件名称
    int file_number;//文件个数
} Options;
```

在`args.c`内部，内置一个`init_options`的函数

```c
int init_options(int argc, char **argv, Options * options);//初始化选项
```

`argc`代表了`char **argv`的下标，同时，**下标一定要从1开始取**，因为第0项是前置的`./lrg`

为了判定选项部分的终止，我们定义一个`can_be_op`的变量：**一旦读到不符合选项格式的字符串，`can_be_op`置0，进行短路判断**

```c
static int is_option(const char * s){
    return s[0] == '-' && s[1] != '\0';
}
//初步判定是不是选项，同时排除只含一个-的情况
```

然后遍历所有字符串，只有`can_be_op && is_option`为1，才可以进入选项设置

再进行模式和文件的录入

```c
if(options->pattern == NULL){
	options->pattern = token;
    continue;
}
else{
	options->files[options->file_number] = token;
    options->file_number++;
}
```

只有`pattern`为`NULL`时，才可以被写入，一旦被写入过，之后的所有输入都按文件处理

### 三、查找器 -> `match.c,match.h`

在`match.h`内，定义以下函数

```c
int match(const char * s, size_t len, const char * pattern, int is_ignore);
//给定已知字符串和长度，找是否有指定子串，有区分大小写开关
```

在`match`函数内部，先进行**特判**

```c
if (pattern_len == 0) return 1;//空串默认匹配成功
if(pattern_len > len) return 0;//短路判断
```

然后再进行循环比对，我没有引入其他自动忽略大小写，或者自动比较字符串的函数或者包，下面说一下我的实现思路：

```c
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
```

先说不忽略大小写的情况，先用`pattern`的第一个字符和整个字符串比对，如果第一项比对不过，直接**短路判断**，再比较字符串下一个字符，如果`pattern`全部比对成功才返回1

再说忽略大小写的情况，我使用了`ASCII`码进行反向判断，如果这个字符是英文字母，并且本身不是字符串里对应的字符，并且它的大写/小写也不是对应的字符，就直接短路判断，其他类型的字符正常比对

### 四、中控部分 -> `main.c`

在中控部分调用**初始化**的函数，之后判定文件数目是否为0，以此来用不同的策略

```c
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
```

如果检测到没有文件，就打开`STDIN_FILENO`，获取外部输入

如果检测到有文件，就循环执行：**打开->读取->关闭**，的操作，直到所有文件都读完

其他分支进行错误处理，**之后在错误处理板块会归纳在一起总结**

### 五、文本读取 -> `reader.c,reader.h`

先在`reader.h`中声明以下内容和函数：

```c
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
void new_free(Reader * reader);//更好的free，free后置空防重复释放
int reader_process(Reader *r, int fd, char *name);//中控
```

接下来讲述一下`reader_process`的实验原理：

在文件开头定义一个容量为`CAPACITY`(2048)的`char`数组，用来从文件中承接字符数据，递给其他函数进行下一步分析，如果检测到有一次检测到读取的字符数量为0，说明读取完了，停止操作，如果检测到读取数量为-1，说明读取被打断了，要进行重试，最后，如果检测到残行（末尾没有换行），进行查找后判定是否弹出，同时在末尾补上`\n`（同时要记住在内部设置好关于各种选项的分支）

```c
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
```

然后把得到的这些数据传递给函数`feed`，负责**反复调用行检测**的函数，直到读取到数组中的最后一位

```c
static ssize_t feed(Reader * reader, ssize_t read_text, char * name){
    int start_number = 0;
    while(start_number != read_text){
        if(pop_line(reader, &start_number, read_text, name) == 2) return 2;
    }
    return 0;
}
```

而在`pop_line`函数中，会先按照`\n`为标志读取出一行，然后调用`match`，来判定是否要输出，如果判定通过，再检测`line`的容量，如果容量不够，就要进行二倍扩容，直到容量能装下行内容为止，这种策略**内存不会随文件增长**，因为占用的内存只和最长的一行有关系

```c
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
```

完成这些之后，只要处理好报错情况和选项关系就完成了

## 六、错误处理表

**下面将各种情况处理整理成了表格：**

| 情况                   | 行为                                                      | 退出码 |
| ---------------------- | --------------------------------------------------------- | ------ |
| 用法错误               | 弹出`usage: lrg [-nic] PATTERN [FILE...]`，不处理任何文件 | 2      |
| 文件无法访问           | 弹出`lrg: 文件名: 原因`，继续处理其余文件                 | 2      |
| 内存分配失败           | 弹出`lrg: 文件名: Cannot allocate memory`                 | 2      |
| `read`被打断           | 重试                                                      | ——     |
| 无任何行被选中，无错误 | 无输出                                                    | 1      |

## 七、已知限制

- **只支持字面量匹配，不支持正则**
- **只能处理纯文本，不能处理二进制文件**
- **只支持`-n`/`-i`/`-c`，不支持其他选项**
- **如果行数超出堆容量，极端情况下会出现OOM错误**

## 八、参考资料与 AI 对话链接

### 参考资料

`man 1 grep`,`man 2 open`,`man 2 read`,`man 2 close`,`man 3 errno`,`man 1 make`

[《跟我一起写 Makefile》](https://seisman.github.io/how-to-write-makefile/)

[CSAPP第10章](https://hansimov.gitbook.io/csapp) 

### AI对话记录

由于是本地AI，此处选取了各个节点的部分对话文档（反复微调和优化的部分省略）

[AI对话记录01](docs/AI01.md)

[AI对话记录02](docs/AI02.md)

[AI对话记录03](docs/AI03.md)

[AI对话记录04](docs/AI04.md)
