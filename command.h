#ifndef COMMAND_H
#define COMMAND_H
#include <qstring.h>

enum commandType{
    //所有的命令类型
    del_line,//删除行
    rem_num,let_num,print_num,
    input_num,goto_num,if_num,end_num,
    //num后缀意味着是带行号的
    run_raw,list_raw,let_raw,print_raw,load_raw,
    input_raw,clear_raw,help_raw,quit_raw
    //raw后缀不带行号
};
class Command{
public:
    int lineNum;//行号
    commandType type;
    QString commandText;
    //修改后的命令，使其标准化便于解析
    //解析后去除行号，保留操作名称
    QString treeRead;
    int errorNum;
    //错误码：
    //行号不存在等错误在运行时报错
    //单独的行号用于删除某一行，重复行号替代。
    //0：无错误
    //1：直接指令，但是指令不存在
    //2：行号越界（在1~100000之外）
    //3：变量名命名错误
    //4：未知运算符（+-*/**mod()之外）
    //5：行号代码，未知的操作
    //6：参数缺少
    //7：指令语法异常

    Command(QString text);
    ~Command();

    //判断是否是无行号的直接指令，是则直接执行，
    //否则判断可否解析成行号命令
    bool isDirectCommand();

    //初步解析行号,成功返回行号，失败则设定错误码并返回-1.
    //对于程序而言，设置成向量形式储存
    //代码的替换与删除在这里识别
    int parseLineNum();

    //深度、利用语法树解析，储存变量，计算
    //成功返回行号，失败设定错误码并返回-1
    int parseLine();

    //利用错误码判断合法性，并报错
    //无错误返回1，有错误返回0
    int judgeValid();
};
//代码的执行利用类似于汇编的逐行执行，
//死循环问题通过设置全局变量，若循环次数超出上限则判定为死循环，强行结束

#endif // COMMAND_H
