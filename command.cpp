#include "command.h"
#include "qstring.h"
#include "qmessagebox.h"
#include "parsingTree.h"

Command::Command(QString text){
    this->lineNum = 0;//预设值
    this->commandText = text.trimmed().simplified();;//预处理字符串，去除两侧空格、中间多余的空格
}

Command::~Command(){}

bool Command::isDirectCommand(){
    //判断是否是直接的、无行号命令
    //是：行号设为0，不添加，直接执行
    //否：若无行号则设置错误码，否则进行下一步判断
    QStringList parts = this->commandText.split(" ", Qt::SkipEmptyParts);
    QString p = parts[0];
    if((this->commandText[0] >= '0' && this->commandText[0]<= '9') || this->commandText[0] == '-'){
        //以数字开头，默认为带行号的代码
        return false;
    }
    if(this->commandText == "RUN"){
        this->lineNum = 0;
        this->errorNum = 0;
        this->type = run_raw;
        return true;
    }
    else if(this->commandText == "LIST"){
        this->lineNum = 0;
        this->errorNum = 0;
        this->type = list_raw;
        return true;
    }
    else if(this->commandText == "LOAD"){
        this->lineNum = 0;
        this->errorNum = 0;
        this->type = load_raw;
        return true;
    }
    else if(this->commandText == "CLEAR"){
        this->lineNum = 0;
        this->errorNum = 0;
        this->type = clear_raw;
        return true;
    }
    else if(this->commandText == "HELP"){
        this->lineNum = 0;
        this->errorNum = 0;
        this->type = help_raw;
        return true;
    }
    else if(this->commandText == "QUIT"){
        this->lineNum = 0;
        this->errorNum = 0;
        this->type = quit_raw;
        return true;
    }
    else if(parts[0] == "PRINT"){
        if(parts.size() < 2){
            this->errorNum = 6;
            this->lineNum = 0;
            this->type = print_raw;
            return false;
        }
        this ->lineNum = 0;
        this->errorNum = 0;
        this->type = print_raw;
        return true;
    }
    else if(parts[0] == "INPUT"){
        if(parts.size() < 2){
            this->errorNum = 6;
            this->lineNum = 0;
            this->type = input_raw;
            return false;
        }
        this ->lineNum = 0;
        this->errorNum = 0;
        this->type = input_raw;
        return true;
    }
    else if(parts[0] == "LET"){
        if(parts.size() < 4){
            this->errorNum = 6;
            this->lineNum = 0;
            this->type = let_raw;
            return false;
        }
        this ->lineNum = 0;
        this->errorNum = 0;
        this->type = let_raw;
        return true;
    }
    else{
        this->lineNum = 0;
        this->errorNum = 1;//未知指令错误
        return true;
    }
}

int Command::parseLineNum(){
    //解析行号，修正字符串
    QStringList parts = this->commandText.split(" ", Qt::SkipEmptyParts);  // 按空格分割
    if (!parts.isEmpty()) {
        bool ok;
        int number = parts[0].toInt(&ok);  // 获取第一个部分并转换为整数
        if (ok) {
            this->lineNum = number;
        } else {
            qDebug() << "无法提取数字！";
        }
    }
    if(this->lineNum >1000000 || this->lineNum <=0){
        this->errorNum = 2;
        return -1;
    }
    this->errorNum = 0;
    return this->lineNum;
}

int Command::parseLine(){
    //深入解析
    //解析成功返回行号，不成功则返回-1，设置错误码
    // 查找第一个空格的位置
    int spaceIndex = this->commandText.indexOf(' ');

    if (spaceIndex != -1) {
        // 提取空格后面的部分（即指令）
        QString command = this->commandText.mid(spaceIndex + 1).trimmed();
        //根据标识符确定种类
        //不是标识符范围内的，设置错误码然后报错
        if (command.left (3)== "REM"){
            //注释类
            this->type = rem_num;
        }
        else if(command.left(5) == "PRINT"){
            //打印类
            this->type = print_num;
        }
        else if(command.left(3) == "LET"){
            //定义类
            this->type = let_num;
        }
        else if(command.left(4) == "GOTO"){
            //跳转类
            this->type = goto_num;
        }
        else if(command.left(5) == "INPUT"){
            //输入类
            this->type = input_num;
        }
        else if(command.left(2) == "IF"){
            //判断类
            this->type = if_num;
        }
        else if(command.left(3) == "END"){
            //结束类
            this->type = end_num;
        }
        else{
            //未知代码
            this->errorNum = 5;
            return -1;
        }
        //对于分辨种类后的命令，进行表达式树的生成解析
        //这里解析其语法正确性，使用parsingTree类中的函数
        ParsingTree tree(*this);
        if(this->errorNum !=0){
            return -1;
        }
    }
    else {
        // 删除所在行
        this->errorNum = 0;
        this->type = del_line;
        return this->lineNum;
    }
    //更新表达式树的显性表达放在mainwindow中实现
    return this->lineNum;
}

int Command::judgeValid(){
    //0：无错误,返回1
    //1：直接指令，但是指令不存在
    //2：行号越界（在1~100000之外）
    //3：变量名命名错误
    //4：未知运算符（+-*/**mod()之外）
    //5：行号代码，未知的操作
    //6：参数缺少
    //7：指令语法异常
    switch (this->errorNum) {
    case 0:
        return 1;
    case 1:
        QMessageBox::warning(nullptr, "ERROR", "Command not found！");
        break;
    case 2:
        QMessageBox::warning(nullptr, "ERROR", "Line number out of range！");
        break;
    case 3:
        QMessageBox::warning(nullptr, "ERROR", "Invalid variable name！");
        break;
    case 4:
        QMessageBox::warning(nullptr, "ERROR", "Unknown operator！");
        break;
    case 5:
        QMessageBox::warning(nullptr, "ERROR", "Unknown command！");
        break;
    case 6:
        QMessageBox::warning(nullptr, "ERROR", "Lack of argument!");
        break;
    case 7:
        QMessageBox::warning(nullptr, "ERROR", "Invalid expression！");
        break;
    }
    return 0;
}
