#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qmessagebox.h"
#include <QFileDialog>
#include "parsingTree.h"
#include <QRegularExpression>
#include <QtCore>
#include "breakpoint.h"
#include <QThread>
#include <QFuture>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setUIExitDebugMode();
    this->Arguments.clear();
    this->TextLines.clear();
    this->clearAll();
    this->currentMode = run;
    canInput = false;

    disconnect(ui->btnRunCode, &QPushButton::clicked, this, &MainWindow::debugRun);
    disconnect(ui->btnRunCode, &QPushButton::clicked, this, &MainWindow::runCode);
    connect(ui->btnRunCode, &QPushButton::clicked, this, &MainWindow::runCode);

    connect(ui->btnDebugMode, &QPushButton::clicked, this, &MainWindow::setUIForDebugMode);
    connect(ui->btnExitDebugMode, &QPushButton::clicked, this, &MainWindow::setUIExitDebugMode);
    connect(ui->btnClearCode,&QPushButton::clicked,this,&MainWindow::clearAll);
    connect(ui->btnDebugResume,&QPushButton::clicked,this,&MainWindow::debugResume);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_cmdLineEdit_editingFinished()
{
    QString cmd = ui->cmdLineEdit->text();
    ui->cmdLineEdit->setText("");
    qDebug()<<"Mode::"<<this->currentMode;

    if(this->currentMode == run){
        if(cmd != ""){
            //忽略空命令
            Command text(cmd);
            if(text.isDirectCommand()){
                //是直接指令，直接执行；不是直接指令，进行深入解析
                if(text.judgeValid() == 1){
                    //符合要求，执行,然后直接退出，不插入
                    commandRun(text);
                    return;
                }
                return;
                //不符合会报错
            }
            else{
                //带有行号的指令处理
                if(text.parseLineNum() == -1){
                    text.judgeValid();//解析行号失败
                    return;
                }
                if(text.parseLine() == -1){
                    text.judgeValid();//进一步解析失败
                    return;
                }
            }
            //解析成功，修改整体的状态
            ui->CodeDisplay->clear();//先清空
            ui->treeDisplay->clear();
            insertLine(text);//需要根据序号插入、替代、删除
            for (auto it = TextLines.begin(); it != TextLines.end(); ++it) {
                ui->CodeDisplay->append(it->commandText);//依次插入显示的文本框
                ui->treeDisplay->append(it->treeRead);
            }
            //接着修改树的栏;
        }
    }

    else if(this->currentMode == debug){
        //debug模式下
        if(cmd != ""){
            QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);
            if(parts.size()==2){
                if(parts[0] == "ADD"){
                    bool ok;
                    int num = parts[1].toInt(&ok);
                    qDebug()<<"add point"<<num;
                    if(ok){
                        if(!addPoint(num)){
                            QMessageBox::warning(this,"Debug Error","Line not found!");
                        }
                    }
                    else{
                        QMessageBox::warning(this,"Debug Error","Cannot read the line number!");
                    }
                }
                else if(parts[0] == "DELETE"){
                    bool ok;
                    int num = parts[1].toInt(&ok);
                    if(ok){
                        if(!deletePoint(num)){
                            QMessageBox::warning(this,"Debug Error","BreakPoint not found!");
                        }
                    }
                    else{
                        QMessageBox::warning(this,"Debug Error","Cannot read the line number!");
                    }
                }
                else{
                    QMessageBox::warning(this,"Debug Error","Unknown command!");
                }
            }
            else{
                QMessageBox::warning(this,"Debug Error","Unknown command!");
            }
        }
        //更新状态
        ui->breakPointsDisplay->clear();
        for(auto it = this->breakPoints.begin();it!=this->breakPoints.end();++it){
            ui->breakPointsDisplay->append(QString::number((*it).line));
        }
    }
}

void MainWindow::setUIForDebugMode(){
    this->currentMode = debug;

    disconnect(ui->btnRunCode, &QPushButton::clicked, this, &MainWindow::runCode);
    disconnect(ui->btnRunCode, &QPushButton::clicked, this, &MainWindow::debugRun);
    connect(ui->btnRunCode, &QPushButton::clicked, this, &MainWindow::debugRun);

    ui->btnClearCode->setVisible(false);
    ui->btnLoadCode->setVisible(false);
    ui->btnDebugMode->setVisible(false);

    ui->btnExitDebugMode->setVisible(true);
    ui->btnDebugResume->setVisible(true);

    ui->labelSyntaxTree->setVisible(false);
    ui->treeDisplay->setVisible(false);

    ui->labelMonitor->setVisible(true);
    ui->monitorDisplay->setVisible(true);
    ui->labelBreakPoints->setVisible(true);
    ui->breakPointsDisplay->setVisible(true);
}

void MainWindow::setUIExitDebugMode(){
    this->currentMode = run;

    disconnect(ui->btnRunCode, &QPushButton::clicked, this, &MainWindow::debugRun);
    disconnect(ui->btnRunCode, &QPushButton::clicked, this, &MainWindow::runCode);
    connect(ui->btnRunCode, &QPushButton::clicked, this, &MainWindow::runCode);

    ui->btnClearCode->setVisible(true);
    ui->btnLoadCode->setVisible(true);
    ui->btnDebugMode->setVisible(true);

    ui->btnExitDebugMode->setVisible(false);
    ui->btnDebugResume->setVisible(false);

    ui->labelSyntaxTree->setVisible(true);
    ui->treeDisplay->setVisible(true);

    ui->labelMonitor->setVisible(false);
    ui->monitorDisplay->setVisible(false);
    ui->labelBreakPoints->setVisible(false);
    ui->breakPointsDisplay->setVisible(false);
}

void MainWindow::clearAll(){
    ui->CodeDisplay->clear();
    ui->treeDisplay->clear();
    ui->textBrowser->clear();
    this->Arguments.clear();
    this->TextLines.clear();
    //清理断点、debug的表达式、临时结构代码
    ui->breakPointsDisplay->clear();
    ui->monitorDisplay->clear();
    this->breakPoints.clear();
    this->A.clear();
    this->T.clear();
}

void MainWindow::commandRun(Command &t){
    //直接指令
    qDebug()<<"a direct run!";
    if(t.type == list_raw){
        return;
    }
    else if(t.type == load_raw){
        QString fileName = QFileDialog::getOpenFileName(this, "选择文件", "", "文本文件 (*.txt)");

        if (!fileName.isEmpty()) {
            loadFile(fileName);  // 传递文件路径给 loadFile 函数
        }
        return;
    }
    else if(t.type == help_raw){
        QString help_message;
        help_message = "Help message:\n"
                       "    Here I write a BASIC interpreter for you. Some tricks can be used to help you use it more smoothly.\n"
                       "    To write a program, you can use:\n"
                       "        LET <var> = <expression>    --define a variable\n"
                       "        PRINT <var>/<expression>    --print a value\n"
                       "        INPUT <var>                 --input the value\n"
                       "        GOTO <line>                 --go to the line\n"
                       "        IF <expresion> THEN <line>  --if-branch\n"
                       "        REM                         --make remarks\n"
                       "        END                         --end the program\n"
                       "    To better build your program,you can use:\n"
                       "        RUN     --run the program\n"
                       "        HELP    --get help information\n"
                       "        CLEAR   --clear all the memory\n"
                       "        LOAD    --load from your local files\n"
                       "        QUIT    --exit the interpreter\n"
                       "    Just have fun with this interpreter!";
        QMessageBox::information(nullptr,"Help Information",help_message);
        return;
    }
    else if(t.type == run_raw){
        qDebug()<<"successful run!";
        runCode();
        return;
    }
    else if(t.type == print_raw){
        qDebug()<<"print_raw";
        commandPRINT(t);
        return;
    }
    else if(t.type == input_raw){
        commandINPUT(t);
        return;
    }
    else if(t.type == let_raw){
        commandLET(t);
        return;
    }
    else if(t.type == quit_raw){
        exit(0);//退出
        return;
    }
    else if(t.type == clear_raw){
        clearAll();
        return;
    }
}

void MainWindow::commandPRINT(Command &t){
    //裸指令按照前后顺序插入
    QString prefix_command = QString::number(this->T.size() + 1) + " ";
    //虚拟前缀
    Command newCommand((prefix_command + t.commandText));
    newCommand.parseLineNum();
    newCommand.parseLine();
    if(!newCommand.judgeValid()){
        return;
    }

    bool isValid = true;
    ParsingTree tree(newCommand);
    int num = getNodeValueT(tree.root->childNodes[0],isValid);
    if(!isValid){
        QMessageBox::warning
            (this,"Print Error",
             "Error occured :invalid expression");
        return;
    }
    else{
        ui->textBrowser->append(QString::number(num));
    }

    this->T.push_back(newCommand);
}

void MainWindow::commandLET(Command &t){
    //裸指令按照前后顺序插入
    QString prefix_command = QString::number(this->T.size() + 1) + " ";
    //虚拟前缀
    Command newCommand((prefix_command + t.commandText));
    newCommand.parseLineNum();
    newCommand.parseLine();

    //问题：未出现的变量
    bool isValid;
    ParsingTree tree(newCommand);
    int num = getNodeValueT(tree.root->childNodes[1],isValid);
    if(!isValid){
        QMessageBox::warning
            (this,"LET Error",
             "Error occured:invalid expression");
        return;
    }
    else{
        //先查找这个变量存不存在，决定是增加还是修改
        QString name = tree.root->childNodes[0]->s;
        bool isExist = false;
        if(this->A.empty()){
            isExist = false;
        }
        auto it = this->A.begin();
        for(;it!=A.end();++it){
            if((*it).name == name){
                isExist = true;
                break;
            }
            else{continue;}
        }

        if(isExist){
            (*it).data = num;
        }
        else{
            Value newVal(name,num);
            this->A.push_back(newVal);
        }
        this->T.push_back(newCommand);
        return;
    }
}

void MainWindow::commandINPUT(Command &t){
    //裸指令按照前后顺序插入
    QString prefix_command = QString::number(this->T.size() + 1) + " ";
    //虚拟前缀
    Command newCommand((prefix_command + t.commandText));
    newCommand.parseLineNum();
    newCommand.parseLine();

    //问题：错误的输入
    ParsingTree tree(newCommand);
    QString name = tree.root->childNodes[0]->s;

    // 设置提示文本为 "?"
    // 设置初始提示文本
    ui->cmdLineEdit->setText("? ");
    ui->cmdLineEdit->setFocus();
    ui->cmdLineEdit->setCursorPosition(2); // 光标移动到 "?" 后
    //检测是否存在
    bool isExist = false;
    if(this->A.empty()){
        isExist = false;
    }
    auto it = this->A.begin();
    for(;it!=A.end();++it){
        if((*it).name == name){
            isExist = true;
            break;
        }
        else{continue;}
    }
    // 创建事件循环
    QEventLoop loop;

    // 连接回车信号
    connect(ui->cmdLineEdit, &QLineEdit::returnPressed, this, [=, &loop]() mutable {
        QString input = ui->cmdLineEdit->text().mid(2).trimmed(); // 提取 "?" 后的内容
        bool isNumber;
        int value = input.toInt(&isNumber);

        if (isNumber) {
            qDebug() << "User entered number:" << value;
            // 存储数字到变量表
            if(isExist){
                (*it).data = value;
            }
            else{
                Value newVal(name,value);
                this->A.push_back(newVal);
            }
            ui->cmdLineEdit->clear();       // 清空输入框
            loop.quit();                    // 退出事件循环
        }
        else {
            QMessageBox::warning(this, "Input Error", "Please enter a valid number.");
            ui->cmdLineEdit->setText("? "); // 重新设置提示文本
            ui->cmdLineEdit->setCursorPosition(2); // 光标移动到 "?" 后
        }
    });

    // 阻塞主线程，等待用户输入正确
    loop.exec();
    loop.quit();
    disconnect(ui->cmdLineEdit, &QLineEdit::returnPressed, nullptr, nullptr);
    ui->cmdLineEdit->clear();  // 确保输入框清空
    this->T.push_back(newCommand);
    return;
}

void MainWindow::insertLine(Command &t) {
    // 如果 t.commandText 是纯数字，执行删除操作
    bool isNumeric;
    int lineNumToDelete = t.commandText.toInt(&isNumeric);
    if (isNumeric) {
        // 遍历容器，查找并删除对应的 lineNum
        for (auto it = this->TextLines.begin(); it != this->TextLines.end(); ++it) {
            if (it->lineNum == lineNumToDelete) {
                this->TextLines.erase(it);  // 删除元素
                return;  // 删除后直接返回
            }
        }
        // 如果没有找到对应的 lineNum，直接返回
        return;
    }

    // 否则是插入或替换操作
    ParsingTree tmp(t);
    t.treeRead = QString::number(t.lineNum) + " " + tmp.getTreeStruct();

    for (auto it = this->TextLines.begin(); it != this->TextLines.end(); ++it) {
        // 如果找到了相同的 lineNum，替换原有的元素
        if (it->lineNum == t.lineNum) {
            *it = t;  // 替换原有的元素
            return;   // 替换后不需要再继续插入，直接返回
        }
    }

    // 如果没有找到相同的 lineNum，插入新元素
    for (auto it = this->TextLines.begin(); it != this->TextLines.end(); ++it) {
        if (it->lineNum > t.lineNum) {
            this->TextLines.insert(it, t);  // 插入新元素
            return;  // 插入后直接返回
        }
    }

    // 如果没有找到更大的元素，说明插入到末尾
    this->TextLines.push_back(t);
}

void MainWindow::loadFile(QString &FilePath){
    this->Arguments.clear();
    this->TextLines.clear();
    //载入，读取，对每一行进行解析，此时只能是带行号的代码
    //出现问题报错，单句无误更新文本命令向量
    //整个文件无问题加载在缓冲区，并加载语法树
    QFile file(FilePath);  // 打开文件

    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr,"Failure","Cannot Open file！");
        return;
    }

    QTextStream in(&file);  // 创建 QTextStream 用于读取文件内容

    while (!in.atEnd()) {  // 循环直到文件末尾
        QString cmd = in.readLine();  // 逐行读取文件
        if(cmd != ""){
            //忽略空命令
            Command text(cmd);
            if(text.isDirectCommand()){
                //是直接指令，直接执行；不是直接指令，进行深入解析
                if(text.judgeValid() == 1){
                    //符合要求，执行,然后直接退出，不插入
                    commandRun(text);
                }
                return;
                //不符合会报错
            }
            else{
                //带有行号的指令处理
                if(text.parseLineNum() == -1){
                    text.judgeValid();//解析行号失败
                    return;
                }
                if(text.parseLine() == -1){
                    text.judgeValid();//进一步解析失败
                    return;
                }
            }
            //解析成功，修改整体的状态
            ui->CodeDisplay->clear();//先清空
            ui->treeDisplay->clear();
            insertLine(text);//需要根据序号插入、替代、删除
            for (auto it = TextLines.begin(); it != TextLines.end(); ++it) {
                ui->CodeDisplay->append(it->commandText);//依次插入显示的文本框
                ui->treeDisplay->append(it->treeRead);
            }
            //接着修改树的栏;
        }
    }

    file.close();  // 关闭文件
}

void MainWindow::runCode(){

    qDebug()<<"code run!!!!";
    const int lineCarriedMax = 30000;
    //先检查有无end语句，没有报错
    //利用runPointer
    //问题：死循环
    // 检查是否存在 end_num
    if (TextLines.empty()) {
        return;
    }
    this->Arguments.clear();
    ui->textBrowser->clear();

    bool isExist = false;
    for (auto it = TextLines.begin(); it != TextLines.end(); ++it) {
        if (it->type == end_num) {  // 假设 type 是 Command 的成员
            this->runPointer = it;
            isExist = true;
            break;
        }
    }

    if (!isExist) {
        QMessageBox::warning(this, "Running Error", "Error occurred: lack of ending");
        return;
    }

    this->runPointer = TextLines.begin();

    bool isEnd = false;
    int linesCarried = 0;

    while (!isEnd && linesCarried <= lineCarriedMax && runPointer != TextLines.end()) {
        bool isRight;
        if (runPointer->type == rem_num) {
            ++runPointer;
            continue;
        }
        else if (runPointer->type == end_num) {
            isEnd = true;
            ++linesCarried;
            break;
        }
        else if (runPointer->type == goto_num){
            runGoto(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == let_num){
            runLet(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == print_num){
            runPrint(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == input_num){
            canInput = true;
            runInput(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            canInput = false;
            continue;
        }
        else if (runPointer->type == if_num){
            runIf(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
    }
    if (linesCarried > lineCarriedMax){
        QMessageBox::warning(this, "Running Error", "Error occurred: deadly loop");
    }
    this->Arguments.clear();//清空变量表
}

void MainWindow::runGoto(Command &t,bool &ok){
    //问题：行号未出现
    ParsingTree tree(t);
    int lineNumN = tree.root->childNodes[0]->value;
    bool isExist = false;
    for(auto it = TextLines.begin();it!=TextLines.end();++it){
        if((*it).lineNum == lineNumN){
            this->runPointer = it;
            isExist = true;
            ok = true;
            return;
        }
        else{
            continue;
        }
    }
    if(!isExist){
        QMessageBox::warning
            (this,"Running Error",
            "Error occured at line"+QString::number(t.lineNum)+"invalid line number");
        return;
    }
}

void MainWindow::runLet(Command &t,bool &ok){
    //问题：未出现的变量
    bool isValid;
    ParsingTree tree(t);
    int num = getNodeValue(tree.root->childNodes[1],isValid);
    if(!isValid){
        QMessageBox::warning
            (this,"Running Error",
             "Error occured at line"+QString::number(t.lineNum)+":invalid expression");
        ok = false;
        return;
    }
    else{
        //先查找这个变量存不存在，决定是增加还是修改
        QString name = tree.root->childNodes[0]->s;
        bool isExist = false;
        if(this->Arguments.empty()){
            isExist = false;
        }
        auto it = this->Arguments.begin();
        for(;it!=Arguments.end();++it){
            if((*it).name == name){
                isExist = true;
                break;
            }
            else{continue;}
        }

        if(isExist){
            (*it).data = num;
        }
        else{
            Value newVal(name,num);
            this->Arguments.push_back(newVal);
        }

        ++runPointer;
        ok = true;
        return;
    }
}

void MainWindow::runPrint(Command &t,bool &ok){
    //问题：未出现的变量
    bool isValid;
    ParsingTree tree(t);
    int num = getNodeValue(tree.root->childNodes[0],isValid);
    if(!isValid){
        QMessageBox::warning
            (this,"Running Error",
             "Error occured at line"+QString::number(t.lineNum)+":invalid expression");
        ok = false;
        return;
    }
    else{
        ui->textBrowser->append(QString::number(num));
        ++runPointer;
        ok = true;
        return;
    }
}

void MainWindow::runInput(Command &t,bool &ok){
    //问题：错误的输入
    ParsingTree tree(t);
    QString name = tree.root->childNodes[0]->s;

    // 设置提示文本为 "?"
    // 设置初始提示文本
    ui->cmdLineEdit->setText("? ");
    ui->cmdLineEdit->setFocus();
    ui->cmdLineEdit->setCursorPosition(2); // 光标移动到 "?" 后
    //检测是否存在
    bool isExist = false;
    if(this->Arguments.empty()){
        isExist = false;
    }
    auto it = this->Arguments.begin();
    for(;it!=Arguments.end();++it){
        if((*it).name == name){
            isExist = true;
            break;
        }
        else{continue;}
    }
    // 创建事件循环
    QEventLoop loop;

    // 连接回车信号
    connect(ui->cmdLineEdit, &QLineEdit::returnPressed, this, [=, &loop]() mutable {
        QString input = ui->cmdLineEdit->text().mid(2).trimmed(); // 提取 "?" 后的内容
        bool isNumber;
        int value = input.toInt(&isNumber);

        if (isNumber) {
            qDebug() << "User entered number:" << value;
            // 存储数字到变量表
            if(isExist){
                (*it).data = value;
            }
            else{
                Value newVal(name,value);
                this->Arguments.push_back(newVal);
            }
            ui->cmdLineEdit->clear();       // 清空输入框
            loop.quit();                    // 退出事件循环
        }
        else {
            QMessageBox::warning(this, "Input Error", "Please enter a valid number.");
            ui->cmdLineEdit->setText("? "); // 重新设置提示文本
            ui->cmdLineEdit->setCursorPosition(2); // 光标移动到 "?" 后
            ok = true;
        }
    });

    // 阻塞主线程，等待用户输入正确
    loop.exec();
    loop.quit();
    qDebug()<<"END____";
    ok = true;
    runPointer++;
    disconnect(ui->cmdLineEdit, &QLineEdit::returnPressed, nullptr, nullptr);
    ui->cmdLineEdit->clear();  // 确保输入框清空
    return;
}

void MainWindow::runIf(Command &t,bool &ok){
    //问题：未出现的变量
    bool testExp;
    ParsingTree tree(t);

    //计算、解析
    bool f1,f2;
    int n1 = this->getNodeValue(tree.root->childNodes[0],f1);
    int n2 = this->getNodeValue(tree.root->childNodes[2],f2);

    if(!f1){
        QMessageBox::warning
            (this,"Running Error",
             "Error occured at line"+QString::number(t.lineNum)+":invalid expression1");
        ok = false;
        return;
    }
    if(!f2){
        QMessageBox::warning
            (this,"Running Error",
             "Error occured at line"+QString::number(t.lineNum)+":invalid expression2");
        ok = false;
        return;
    }

    if(tree.root->childNodes[1]->s == "="){
        testExp = (n1==n2);
    }
    else if(tree.root->childNodes[1]->s == ">"){
        testExp = (n1>n2);
    }
    else if(tree.root->childNodes[1]->s == "<"){
        testExp = (n1<n2);
    }
    else{
        QMessageBox::warning
            (this,"Running Error",
             "Error occured at line"+QString::number(t.lineNum)+":unknwon operator");
        ok = false;
        return;
    }

    if(testExp){
        int lineNumN = tree.root->childNodes[3]->value;
        bool isExist = false;
        for(auto it = TextLines.begin();it!=TextLines.end();++it){
            if((*it).lineNum == lineNumN){
                this->runPointer = it;
                isExist = true;
                ok = true;
                return;
            }
            else{
                continue;
            }
        }
        if(!isExist){
            QMessageBox::warning
                (this,"Running Error",
                 "Error occured at line"+QString::number(t.lineNum)+":invalid line number");
            ok = false;
            return;
        }
    }
    else{
        ++runPointer;
        ok = true;
        return;
    }
}

int MainWindow::getNodeValue(TreeNode *n,bool &ok){
    //识别是否成立，未定义的变量不能成立

    bool ok1=true,ok2=true;
    if(n->childNodes.empty()){
        //叶子节点
        if(n->Type == Number){
            ok = true;
            return n->value;
        }
        else{
            //变量
            QString name = n->s;
            for(auto it=this->Arguments.begin();it!=this->Arguments.end();++it){
                if((*it).name == name){
                    ok = true;
                    return it->data;
                }
                else{
                    continue;
                }
            }
            ok = false;
            return 0;
        }
    }
    else if(n->childNodes.size() == 1){
        ok = false;
        return 0;
    }

    int s1 = this->getNodeValue(n->childNodes[0],ok1);
    int s2 = this->getNodeValue(n->childNodes[1],ok2);

    if(n->s == "+"){
        ok = ok1 && ok2;
        return s1+s2;
    }
    else if(n->s == "-"){
        ok = ok1 && ok2;
        return s1-s2;
    }
    else if(n->s == "*"){
        ok = ok1 && ok2;
        return s1*s2;
    }
    else if(n->s == "/"){
        if(s2 == 0){
            ok = false;
            return 0;
        }
        ok = ok1 && ok2;
        return s1/s2;
    }
    else if(n->s == "**"){
        ok = ok1 && ok2;

        if (s2 < 0) {
            ok = false;
            return 0;
        }

        int result = 1;
        int base = s1;

        while (s2 > 0) {
            if (s2 % 2 == 1) { // 如果当前指数最低位是 1
                result *= base;
            }
            base *= base; // 底数平方
            s2 /= 2;      // 指数右移（除以 2）
        }

        return result;
    }
    else if(n->s == "MOD"){
        if (s1 == 0) {
            ok = false;
            return 0;
        }
        int r = s1 % s2; // 直接使用取模运算符
        if (r != 0 && ((s2 > 0 && r < 0) || (s2 < 0 && r > 0))) {
            r += s2;
        }
        ok = ok1 && ok2;
        return r;
    }
}

bool MainWindow::addPoint(int c){
    bool found = false;
    for(auto it = this->TextLines.begin();it != this->TextLines.end();++it){
        if((*it).lineNum == c){
            found = true;
            for(auto it2 = this->breakPoints.begin();it2 != this->breakPoints.end();++it2){
                if((*it2).line == c){
                    return true;
                }
                else{continue;}
            }
            BreakPoint b(c);
            int i = 0;
            while (i < this->breakPoints.size() && this->breakPoints[i].line < b.line) {
                ++i;
            }
            // 插入新元素
            this->breakPoints.insert(i, b);
            return true;
        }
        else{
            continue;
        }
        return found;
    }
}

bool MainWindow::deletePoint(int c){
    bool found = false;
    for(auto it = this->breakPoints.begin();it != this->breakPoints.end();++it){
        if((*it).line == c){
            this->breakPoints.erase(it);
            return true;
        }
        else{
            continue;
        }
        return found;
    }
}

void MainWindow::debugRun(){

    ui->textBrowser->clear();
    Arguments.clear();
    ui->monitorDisplay->clear();

    qDebug()<<"debug run!!!!";

    //运行到下一个点
    const int lineCarriedMax = 30000;
    //利用runPointer
    //问题：死循环
    if (TextLines.empty()) {
        return;
    }
    this->Arguments.clear();
    ui->textBrowser->clear();

    bool isExist = false;
    for (auto it = TextLines.begin(); it != TextLines.end(); ++it) {
        if (it->type == end_num) {  // 假设 type 是 Command 的成员
            this->runPointer = it;
            isExist = true;
            break;
        }
    }

    if (!isExist) {
        QMessageBox::warning(this, "Running Error", "Error occurred: lack of ending");
        return;
    }

    this->runPointer = TextLines.begin();

    bool isEnd = false;
    int linesCarried = 0;

    while (!isEnd
           && linesCarried <= lineCarriedMax
           && runPointer != TextLines.end()
           && !isBreakPoint(runPointer->lineNum)) {
        bool isRight;
        if (runPointer->type == rem_num) {
            ++runPointer;
            continue;
        }
        else if (runPointer->type == end_num) {
            isEnd = true;
            ++linesCarried;
            break;
        }
        else if (runPointer->type == goto_num){
            runGoto(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == let_num){
            runLet(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == print_num){
            runPrint(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == input_num){
            runInput(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == if_num){
            runIf(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
    }
    if (linesCarried > lineCarriedMax){
        QMessageBox::warning(this, "Running Error", "Error occurred: deadly loop");
    }
    updateVar();
    qDebug() << "Position:" << runPointer->lineNum;
}

void MainWindow::debugResume(){
    qDebug()<<"resume from"<< this->runPointer->lineNum ;
    ui->monitorDisplay->clear();

    //运行到下一个点
    const int lineCarriedMax = 30000;
    //利用runPointer
    if (TextLines.empty()) {
        return;
    }

    bool isExist = false;
    for (auto it = TextLines.begin(); it != TextLines.end(); ++it) {
        if (it->type == end_num) {  // 假设 type 是 Command 的成员
            isExist = true;
            break;
        }
    }

    if (!isExist) {
        QMessageBox::warning(this, "Running Error", "Error occurred: lack of ending");
        return;
    }

    bool isEnd = false;
    int linesCarried = 0;

    qDebug()<<"start resuming";

    do {
        qDebug()<<"THE LINE IN RUNNING:"<< runPointer->lineNum << "::" << runPointer->type;
        bool isRight;
        if (runPointer->type == rem_num) {
            ++runPointer;
            continue;
        }
        else if (runPointer->type == end_num) {
            isEnd = true;
            ++linesCarried;
            break;
        }
        else if (runPointer->type == goto_num){
            runGoto(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == let_num){
            runLet(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == print_num){
            runPrint(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == input_num){
            runInput(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
        else if (runPointer->type == if_num){
            runIf(*runPointer,isRight);
            if(!isRight){
                break;
            }
            ++linesCarried;
            continue;
        }
    }
    while(!isEnd
             && linesCarried <= lineCarriedMax
             && runPointer != TextLines.end()
             && !isBreakPoint(runPointer->lineNum));

    if (linesCarried > lineCarriedMax){
        QMessageBox::warning(this, "Running Error", "Error occurred: deadly loop");
    }
    qDebug()<<"resume END:"<<this->runPointer->lineNum;
    updateVar();
    qDebug() << "monitor display content:" << ui->monitorDisplay->toPlainText();
}

void MainWindow::updateVar(){
    ui->monitorDisplay->clear();
    for(auto it = this->Arguments.begin();it!=this->Arguments.end();++it){
        QString message;
        message = (*it).name + " = " + QString::number((*it).data);
        ui->monitorDisplay->append(message);
    }
}

bool MainWindow::isBreakPoint(int t){
    for(auto it = this->breakPoints.begin();it != this->breakPoints.end();++it){
        if(t == (*it).line){
            return true;
        }
    }
    return false;
}

int MainWindow::getNodeValueT(TreeNode *n,bool &ok){
    bool ok1=true,ok2=true;
    if(n->childNodes.empty()){
        //叶子节点
        if(n->Type == Number){
            ok = true;
            return n->value;
        }
        else{
            //变量
            QString name = n->s;
            for(auto it=this->A.begin();it!=this->A.end();++it){
                if((*it).name == name){
                    ok = true;
                    return it->data;
                }
                else{
                    continue;
                }
            }
            ok = false;
            return 0;
        }
    }
    else if(n->childNodes.size() == 1){
        ok = false;
        return 0;
    }

    int s1 = this->getNodeValueT(n->childNodes[0],ok1);
    int s2 = this->getNodeValueT(n->childNodes[1],ok2);

    if(n->s == "+"){
        ok = ok1 && ok2;
        return s1+s2;
    }
    else if(n->s == "-"){
        ok = ok1 && ok2;
        return s1-s2;
    }
    else if(n->s == "*"){
        ok = ok1 && ok2;
        return s1*s2;
    }
    else if(n->s == "/"){
        if(s2 == 0){
            ok = false;
            return 0;
        }
        ok = ok1 && ok2;
        return s1/s2;
    }
    else if(n->s == "**"){
        ok = ok1 && ok2;

        if (s2 < 0) {
            ok = false;
            return 0;
        }

        int result = 1;
        int base = s1;

        while (s2 > 0) {
            if (s2 % 2 == 1) { // 如果当前指数最低位是 1
                result *= base;
            }
            base *= base; // 底数平方
            s2 /= 2;      // 指数右移（除以 2）
        }

        return result;
    }
    else if(n->s == "MOD"){
        if (s1 == 0) {
            ok = false;
            return 0;
        }
        int r = s1 % s2; // 直接使用取模运算符
        if (r != 0 && ((s2 > 0 && r < 0) || (s2 < 0 && r > 0))) {
            r += s2;
        }
        ok = ok1 && ok2;
        return r;
    }
}
