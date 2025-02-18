#include "value.h"

Value::Value(){
    this->data = 0;
    this->name = "";
}

Value::Value(const QString &text,const int dataValue){
    this->data = dataValue;
    this->name = text;
}

Value::~Value(){}

int Value::copyValue(const Value &source){
    this->data = source.data;
    if(this->data == source.data){
        return this->data;
    }
    return -1;
}

int Value::getValue(const int source){
    this->data = source;
    if(this->data == source){
        return this->data;
    }
    return -1;
}

bool Value::judgeName(){
    if (this->name == ""
        ||this->name == "REM"
        ||this->name == "RUN"
        ||this->name == "LET"
        ||this->name == "PRINT"
        ||this->name == "INPUT"
        ||this->name == "GOTO"
        ||this->name == "IF"
        ||this->name == "THEN"
        ||this->name == "END"
        ||this->name == "LOAD"
        ||this->name == "LIST"
        ||this->name == "CLEAR"
        ||this->name == "HELP"
        ||this->name == "QUIT"
        ||this->name == "MOD"){
        //排除预留的字符
        return false;
    }
    if (this->name[0]!='_'
        &&!(this->name[0]>='a'&&this->name[0]<='z')
        &&!(this->name[0]>='A'&&this->name[0]<='Z')
        ){
        //首位仅可为下划线和字母
        return false;
    }
    for (int i = 1;i<=this->name.length()-1;++i){
        if (this->name[0]!='_'
            &&!(this->name[0]>='a'&&this->name[0]<='z')
            &&!(this->name[0]>='A'&&this->name[0]<='Z')
            &&!(this->name[0]>='0'&&this->name[0]<='9')
            ){
            //仅可为下划线和字母或数字
            return false;
        }
    }
    return true;
}
