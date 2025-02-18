#ifndef VALUE_H
#define VALUE_H

#include "qstring.h"

class Value{
    //变量
public:
    int data;//值
    QString name;//名称

    Value();
    Value(const QString &text,const int dataValue);
    ~Value();
    int copyValue(const Value &source);//复制值，复制之后检验，成功返回值，失败返回-1
    int getValue(const int source);//赋值函数,完成后检验，如果成功返回值，失败返回-1
    bool judgeName();//判断名称是否合法，若不合法则返回假;开头只能是字母或下划线，中间只能出现字母、数字、下划线

};

#endif // VALUE_H
