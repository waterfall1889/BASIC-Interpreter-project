#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "parsingTree.h"
#include "value.h"
#include "command.h"
#include <vector>
#include "breakpoint.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum mode{
        run,debug
    }currentMode;

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QVector<Value> Arguments;//储存所有变量
    QVector<Command> TextLines;//储存所有代码

    QVector<Value> A;//储存临时变量
    QVector<Command> T;//储存临时代码
    void insertLine(Command &t);//按照行号插入
    QVector<Command>::iterator runPointer;
    bool canInput;

private slots:
    void on_cmdLineEdit_editingFinished();

private:
    Ui::MainWindow *ui;

    void setUIForDebugMode();
    void setUIExitDebugMode();
    void clearAll();//delete all
    void commandRun(Command &t);//运行直接指令

    void commandLET(Command &origin);
    void commandINPUT(Command &origin);
    void commandPRINT(Command &origin);


    void loadFile(QString &filePath);//载入文件内容

    void runCode();//运行代码，运行时若发生错误则退出，运行时间超过10s则退出

    void runGoto(Command &t,bool &ok);
    void runLet(Command &t,bool &ok);
    void runPrint(Command &t,bool &ok);
    void runInput(Command &t,bool &ok);
    void runIf(Command &t,bool &ok);

    int getNodeValue(TreeNode *n,bool &ok);
    int getNodeValueT(TreeNode *n,bool &ok);

    bool addPoint(int c);
    bool deletePoint(int c);//失败返回false
    QVector<BreakPoint> breakPoints;//记录断点

    void debugRun();//从开始运行
    void debugResume();//运行到出现断点
    void updateVar();//更新变量区域
    bool isBreakPoint(int t);

};
#endif // MAINWINDOW_H
