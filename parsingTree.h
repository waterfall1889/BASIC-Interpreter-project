#ifndef PARSINGTREE_H
#define PARSINGTREE_H

#include "qstring.h"
#include "command.h"

enum NodeType{
    Number,Operator,variable,Exp,Other
};

enum TreeType{
    remTree,
    gotoTree,
    endTree,
    inputTree,
    letTree,
    printTree,
    ifTree
};

class TreeNode{
    //表达式树节点
    //三种类型，同一层的节点用向量储存
public:
    NodeType Type;
    int value;//储存数或变量的值
    QString s;//名称

    std::vector<TreeNode *> childNodes;//储存子节点

    TreeNode(QString text,NodeType nt);//根据字符串决定类型
};

class ParsingTree{
public:
    TreeNode *root;
    TreeType treeType;

    ParsingTree();
    ParsingTree(Command &t);
    //依托于命令的构造，只用于解析语法检查，以及完善语法树显性展示
    //运行时的解析需要与总命令表交互，因此在mainwindow中实现

    ~ParsingTree();

    void clear(TreeNode *rt);
    void clear();//清空的函数

    bool isEmpty();//是否为空
    //计算值需要与总体的变量表交互，在mainwindow里实现。

    //分类解析,解析中出现错误需要
    void buildRem(Command &t);
    void buildGoto(Command &t);
    void buildEnd(Command &t);
    void buildInput(Command &t);
    void buildLet(Command &t);
    void buildPrint(Command &t);
    void buildIf(Command &t);

    TreeNode *expressionTree(Command &t,QString exp);
    //生成表达式树，若有错需修改错误码，无错返回生成的根节点,变量全部设定为0，在run时需要与表同步
    int getValue(TreeNode *exp);//得到某表达式节点的值

    QString getTreeStruct();//利用层序遍历得到树的结构输出
    int getPriority(TreeNode *op);//优先级
};
#endif // PARSINGTREE_H
