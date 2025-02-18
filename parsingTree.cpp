#include "qdebug.h"
#include "qstring.h"
#include "parsingTree.h"
#include "value.h"
#include <QStringList>
#include <QRegularExpression>
#include <QStack>

TreeNode::TreeNode(QString text,NodeType nt){
    this->childNodes.clear();
    this->Type = nt;
    this->s = text;
}

ParsingTree::ParsingTree(){
    this->root = nullptr;
}

ParsingTree::ParsingTree(Command &t){
    switch(t.type){
    case rem_num:
        this->treeType = remTree;
        buildRem(t);
        break;
    case goto_num:
        this->treeType = gotoTree;
        buildGoto(t);
        break;
    case end_num:
        this->treeType = endTree;
        buildEnd(t);
        break;
    case input_num:
        this->treeType = inputTree;
        buildInput(t);
        break;
    case let_num:
        this->treeType = letTree;
        buildLet(t);
        break;
    case print_num:
        this->treeType = printTree;
        buildPrint(t);
        break;
    case if_num:
        this->treeType = ifTree;
        buildIf(t);
        break;
    default:
        break;
    }

}

ParsingTree::~ParsingTree(){
    this->clear();
}

bool ParsingTree::isEmpty(){
    return (this->root == nullptr);
}

void ParsingTree::clear(TreeNode *rt){
    if(!rt){
        return;
    }
    for (auto it = rt->childNodes.begin(); it != rt->childNodes.end(); ++it) {
        clear(*it);
    }
    delete rt;
}

void ParsingTree::clear(){
    clear(this->root);
}

void ParsingTree::buildRem(Command &t){
    NodeType n = Other;
    this->root = new TreeNode("REM",n);
    QStringList parts = t.commandText.split(' ', Qt::SkipEmptyParts);
    // 检查分割后的字符串列表是否有足够的元素
    if (parts.size() >= 3) {
        // 获取 REM 后面的所有内容
        QString result = parts.mid(2).join(" ");  // 从索引2开始获取剩下的部分，并重新拼接
        TreeNode * tmp = new TreeNode(result,n);
        this->root->childNodes.push_back(tmp);
        t.errorNum = 0;
    }
    else{
        t.errorNum = 6;//参数缺乏
        return;
    }
}

void ParsingTree::buildGoto(Command &t){
    NodeType n = Other;
    this->root = new TreeNode("GOTO",n);

    QStringList parts = t.commandText.split(' ', Qt::SkipEmptyParts);

    // 检查分割后的字符串列表是否有足够的元素
    if (parts.size() == 3) {
        bool ok;
        int result = parts[2].toInt(&ok);  // 将最后一部分转为整数

        if (ok) {
            //成功识别
            TreeNode * tmp = new TreeNode(parts[2],Number);
            tmp->value = result;
            this->root->childNodes.push_back(tmp);
            t.errorNum = 0;
            return;
        }
        else {
            //未识别
            t.errorNum = 7;
            return;
        }
    }
    else {
        //格式错误
        t.errorNum = 6;
        return;
    }
}

void ParsingTree::buildEnd(Command &t){
    NodeType n = Other;
    this->root = new TreeNode("END",n);

    QStringList parts = t.commandText.split(' ', Qt::SkipEmptyParts);

    if(parts.size() > 2){
        t.errorNum = 7;
        return;
    }
    return;
}

void ParsingTree::buildInput(Command &t){
    NodeType n = Other;
    this->root = new TreeNode("INPUT",n);

    QStringList parts = t.commandText.split(' ', Qt::SkipEmptyParts);
    if(parts.size() > 3){
        t.errorNum = 7;
        return;
    }
    else if(parts.size() == 2){
        t.errorNum = 6;//缺少参数
        return;
    }
    else{
        Value val(parts[2],0);//利用变量类检测变量名是否符合规范
        if(!val.judgeName()){
            t.errorNum = 3;//变量命名错误
            return;
        }
        TreeNode *tmp = new TreeNode(parts[2],variable);
        this->root->childNodes.push_back(tmp);
        t.errorNum = 0;
        return;
    }
}

void ParsingTree::buildLet(Command &t){
    NodeType n = Other;
    this->root = new TreeNode("LET =",n);

    // 使用 split 按空格分割字符串，提取 LET 后面的单词
    QStringList parts = t.commandText.split(' ', Qt::SkipEmptyParts);

    if (parts.size() >= 5) {
        QString valName = parts[2];  // 变量名
        Value val(valName,0);//利用变量类检测变量名是否符合规范
        if(!val.judgeName()){
            t.errorNum = 3;//变量命名错误
            return;
        }
        //没有错误，则作为子节点
        TreeNode *l = new TreeNode(valName,variable);
        this->root->childNodes.push_back(l);

        // 使用 split 按等号分割字符串，获取等号右侧的文段
        QStringList rightParts = t.commandText.split('=');
        if (rightParts.size() >= 2) {
            QString rightPart = rightParts[1].trimmed();  // 获取等号后面的部分，并去除前后的空格
            //对这个部分继续进行分析（表达式）
            TreeNode *exp = new TreeNode("",Exp);
            exp = this->expressionTree(t,rightPart);
            this->root->childNodes.push_back(exp);
        }
    }

    else {
        t.errorNum = 6;//参数不足
        return;
    }
}

void ParsingTree::buildPrint(Command &t){
    NodeType n = Other;
    this->root = new TreeNode("PRINT",n);

    // 使用 split 按空格分割字符串，提取 LET 后面的单词
    QStringList parts = t.commandText.split(' ', Qt::SkipEmptyParts);

    if (parts.size() >= 3) {
        // 获取 PRINT 后的内容，即第三部分及之后的内容
        QString result = parts.mid(2).join(" ");  // 从索引2开始提取其余部分并连接为一个完整的字符串
        TreeNode *exp = new TreeNode(result,Exp);
        exp = this->expressionTree(t,result);
        this->root->childNodes.push_back(exp);

    }
    else {
        t.errorNum = 6;//参数不足
        return;
    }
}


void ParsingTree::buildIf(Command &t) {
    NodeType n = Other;
    this->root = new TreeNode("IF THEN", n);

    // 去除前后空白字符
    QString text = t.commandText.trimmed();

    // 查找 "IF" 和 "THEN" 的位置
    int posIf = text.indexOf(" IF ");
    int posThen = text.indexOf(" THEN ");

    // 确保 "IF" 和 "THEN" 都存在，且位置顺序正确
    if (posIf == -1 || posThen == -1 || posIf >= posThen) {
        t.errorNum = 7;  // 格式错误
        return;
    }

    // 提取 "IF" 前的行号并进行 trim
    QString lineNumberStr = text.left(posIf).trimmed();  // 行号
    bool ok;
    int lineNum = lineNumberStr.toInt(&ok);  // 转换为整数
    if (!ok || lineNum <= 0 || lineNum >= 1000000) {
        t.errorNum = 7;  // 行号不合法
        return;
    }

    // 提取 "THEN" 后的跳转行号并进行 trim
    QString thenLineStr = text.mid(posThen + 6).trimmed();  // 提取 THEN 后的部分
    int thenLineNum = thenLineStr.toInt(&ok);  // 转换为整数
    if (!ok || thenLineNum <= 0 || thenLineNum >= 1000000 || thenLineNum == lineNum) {
        t.errorNum = 7;  // 跳转行号不合法
        return;
    }

    // 提取 "IF" 和 "THEN" 之间的条件部分并进行 trim
    QString condition = text.mid(posIf + 4, posThen - posIf - 4).trimmed();  // 条件部分

    // 分割条件，寻找运算符（>、<、=）
    QString leftExpr, operatorStr, rightExpr;
    bool operatorFound = false;

    // 通过遍历条件部分，找到运算符的位置并进行 trim
    int opPos = -1;
    for (int i = 0; i < condition.length(); ++i) {
        QChar ch = condition[i];
        if (ch == '<' || ch == '>' || ch == '=') {
            opPos = i;
            operatorFound = true;
            break;
        }
    }

    if (operatorFound) {
        leftExpr = condition.left(opPos).trimmed();  // 左侧表达式
        operatorStr = condition.mid(opPos, 1).trimmed();  // 运算符
        rightExpr = condition.mid(opPos + 1).trimmed();  // 右侧表达式
    }

    // 确保条件部分合法
    if (!operatorFound || leftExpr.isEmpty() || operatorStr.isEmpty() || rightExpr.isEmpty()) {
        t.errorNum = 7;  // 条件部分格式不正确
        return;
    }

    // 构建语法树节点
    // 1. 左表达式节点
    TreeNode *exp1 = new TreeNode(leftExpr, Exp);
    exp1 = this->expressionTree(t, leftExpr);  // 调用 expressionTree 解析左表达式
    this->root->childNodes.push_back(exp1);

    // 2. 运算符节点
    TreeNode *op = new TreeNode(operatorStr, Operator);
    this->root->childNodes.push_back(op);

    // 3. 右表达式节点
    TreeNode *exp2 = new TreeNode(rightExpr, Exp);
    exp2 = this->expressionTree(t, rightExpr);  // 调用 expressionTree 解析右表达式
    this->root->childNodes.push_back(exp2);

    // 4. THEN 行号节点
    TreeNode *ln = new TreeNode(thenLineStr, Number);
    ln->value = thenLineNum;
    qDebug()<<ln->value;
    this->root->childNodes.push_back(ln);

    t.errorNum = 0;  // 解析成功
}

QString ParsingTree::getTreeStruct(){
    QString total = "";
    //依托前序遍历，与层数有关
    QStack<TreeNode *> nodes;//储存节点
    QStack<int> depth;//维护深度
    if(this->isEmpty()){
        return "";
    }

    nodes.push(this->root);
    depth.push(0);

    while(!nodes.empty()){
        int k = depth.pop();
        TreeNode *t = nodes.pop();
        QString tmp;

        tmp = t->s;
        for(int i = 0; i<k;++i){
            total += "    ";
        }
        total += tmp;
        total += '\n';
        if(!t->childNodes.empty()){
            for(auto it = t->childNodes.rbegin();it != t->childNodes.rend();++it){
                depth.push(k+1);
                nodes.push(*it);
            }
        }
    }

    return total;
}

TreeNode *ParsingTree::expressionTree(Command &t,QString exp){
    //先构建节点、解析结构，再构建树
    QVector<TreeNode*> nodes;
    //数：至结束或者下一个空格
    //运算符
    //变量：至结束或者下一个空格
    int pos = 0;
    const int pos_max = exp.size()-1;
    while(pos<=pos_max){
        TreeNode *tmp;
        if (exp[pos] == '('){
            tmp = new TreeNode("(",Operator);
            nodes.push_back(tmp);
            pos++;
            continue;
        }
        else if (exp[pos] == ')'){
            tmp = new TreeNode(")",Operator);
            nodes.push_back(tmp);
            pos++;
            continue;
        }
        else if (exp[pos] == '+'){
            tmp = new TreeNode("+",Operator);
            nodes.push_back(tmp);
            pos++;
            continue;
        }
        else if (exp[pos] == '/'){
            tmp = new TreeNode("/",Operator);
            nodes.push_back(tmp);
            pos++;
            continue;
        }
        else if (exp[pos] == '*'){
            if(pos<pos_max && exp[pos+1] == '*'){
                tmp = new TreeNode("**",Operator);
                nodes.push_back(tmp);
                pos+=2;
                continue;
            }
            else{
                tmp = new TreeNode("*",Operator);
                nodes.push_back(tmp);
                pos++;
                continue;
            }
        }
        else if(exp[pos] == 'M'){
            if(pos<pos_max-1 && exp[pos+1] == 'O' && exp[pos+2] == 'D'){
                if(pos == pos_max - 2){
                    tmp = new TreeNode("MOD",Operator);
                    nodes.push_back(tmp);
                    pos+=3;
                    continue;
                }
                else if(pos < pos_max-2
                           && (exp[pos+3] == ' '
                               ||exp[pos+3] == '('
                               ||exp[pos+3] == ')')){
                    tmp = new TreeNode("MOD",Operator);
                    nodes.push_back(tmp);
                    pos+=3;
                    continue;
                }
                else{
                    //如果其后不是数字，作为变量处理
                    int p = pos+3;
                    int s = pos+3;
                    while(p<=pos_max && exp[p]!=' '){
                        p++;
                    }
                    QString substring = exp.mid(s,p-(pos+3));
                    bool ok;
                    int num = substring.toInt(&ok);

                    if(ok){
                        //识别到数字
                        TreeNode *tmp1,*tmp2;
                        tmp1 = new TreeNode("MOD",Operator);
                        tmp2 = new TreeNode(substring,Number);
                        tmp2->value = num;
                        nodes.push_back(tmp1);
                        nodes.push_back(tmp2);
                        pos+=(p - pos);
                        continue;
                    }
                    else{
                        tmp = new TreeNode(substring,variable);
                        //检测变量是否合法
                        Value k(substring,0);
                        if(k.judgeName()){
                            nodes.push_back(tmp);
                            pos+=(p - pos);
                            continue;
                        }
                        else{
                            t.errorNum = 3;
                            return nullptr;
                        }
                    }
                }
            }
            else{
                //作为变量处理
                int p = pos;
                while(p<=pos_max && exp[p]!=' ' && exp[p]!='+'
                       && exp[p]!='-'&& exp[p]!='*'&& exp[p]!='/'
                       && exp[p]!='('&& exp[p]!=')'){
                    p++;
                }
                QString substring = exp.mid(pos,p-pos);
                tmp = new TreeNode(substring,variable);
                nodes.push_back(tmp);
                pos+=(p - pos);
                continue;
            }
        }
        else if(exp[pos] == ' '){
            pos++;
            continue;
        }
        else if(exp[pos] >= '0' && exp[pos] <='9'){
            //直到数字结束
            int p = pos;
            while(p<=pos_max && exp[p]>='0' && exp[p]<='9'){
                p++;
            }
            QString substring = exp.mid(pos,p-pos);
            bool ok;
            int num = substring.toInt(&ok);
            if (ok){
                tmp = new TreeNode(substring,Number);
                tmp->value = num;
                nodes.push_back(tmp);
                pos+=(p - pos);
                continue;
            }
        }
        else if(exp[pos] == '-'){
            if(pos == 0){
                //负号
                int p = pos+1;
                while(pos<= pos_max && pos == ' '){
                    p++;
                }
                int start = p;
                int end = p;
                while(end<=pos_max && exp[end]<='9' && exp[end] >= '0'){
                    end ++;
                }
                if(end == start){
                    t.errorNum = 7;
                    return nullptr;
                }
                else{
                    QString sub = exp.mid(start,end - start);
                    int num = sub.toInt();
                    tmp = new TreeNode("-"+sub,Number);
                    tmp->value = -num;
                    nodes.push_back(tmp);
                    pos+=(end-pos);
                }
            }
            else{
                int p = pos-1;
                while(p>0 && exp[p]==' '){
                    p--;
                }
                QChar oop = exp[p];
                if(oop == '+'||oop == '-'||oop == '*'||oop == '/'){
                    t.errorNum = 7;
                    return nullptr;
                }
                else if(oop == '('||oop == ' '){
                    //负号
                    int p = pos+1;
                    while(pos<= pos_max && pos == ' '){
                        p++;
                    }
                    int start = p;
                    int end = p;
                    while(end<=pos_max && exp[end]<='9' && exp[end] >= '0'){
                        end ++;
                    }
                    if(end == start){
                        t.errorNum = 7;
                        return nullptr;
                    }
                    else{
                        QString sub = exp.mid(start,end - start);
                        int num = sub.toInt();
                        tmp = new TreeNode("-"+sub,Number);
                        tmp->value = -num;
                        nodes.push_back(tmp);
                        pos+=(end-pos);
                    }
                }
                else{
                    //减号
                    tmp = new TreeNode("-",Operator);
                    nodes.push_back(tmp);
                    pos++;
                    continue;
                }
            }
        }
        else{
            //其他全部认为是变量名
            int p = pos+1;
            while(p<=pos_max&&exp[p]!=' '
                   &&exp[p]!='+'&&exp[p]!='-'
                   &&exp[p]!='*'&&exp[p]!='/'
                   &&exp[p]!='('&&exp[p]!=')'){
                p++;
            }
            QString sub = exp.mid(pos,p-pos);
            Value tk(sub,0);
            if(tk.judgeName()){
                tmp = new TreeNode(sub,variable);
                nodes.push_back(tmp);
                pos+=(p-pos);
                continue;
            }
            else{
                t.errorNum = 3;
                return nullptr;
            }
        }
    }

    //后续处理
    QStack <TreeNode *> nums;
    QStack <TreeNode *> ops;

    for(auto it = nodes.begin();it!=nodes.end();++it){
        if((*it)->Type == Number || (*it)->Type == variable){
            nums.push(*it);
            continue;
        }
        else if((*it)->Type == Operator){
            if((*it)->s == "("){
                ops.push(*it);
                continue;
            }
            else if((*it)->s == ")"){
                //先寻找上一个左括号，不存在直接判定为错误
                //找到后，依次弹出直到左括号出现
                bool isExistleft = false;
                for (auto it = ops.cbegin(); it != ops.cend(); ++it) {
                    if((*it)->s == "("){
                        isExistleft = true;
                        break;
                    }
                    else{continue;}
                }

                if(!isExistleft){
                    t.errorNum = 7;
                    return nullptr;
                }

                while(ops.top()->s!="("){
                    if(nums.size()<2){
                        t.errorNum = 7;
                        return nullptr;
                    }
                    TreeNode *tmp = ops.pop();
                    TreeNode *tmp2 = nums.pop();
                    TreeNode *tmp1 = nums.pop();
                    tmp->childNodes.push_back(tmp1);
                    tmp->childNodes.push_back(tmp2);
                    nums.push(tmp);
                }
                ops.pop();//弹出左括号
            }
            else if((*it)->s == "+"||(*it)->s == "-"){
                if(ops.empty()||ops.top()->s == "("){
                    ops.push(*it);
                    continue;
                }

                else{
                    //其他情况弹出到空或者左括号出现
                    while(!ops.empty() && ops.top()->s != "("){
                        if(nums.size()<2){
                            t.errorNum = 7;
                            return nullptr;
                        }
                        TreeNode *tmp = ops.pop();
                        TreeNode *tmp2 = nums.pop();
                        TreeNode *tmp1 = nums.pop();
                        tmp->childNodes.push_back(tmp1);
                        tmp->childNodes.push_back(tmp2);
                        nums.push(tmp);
                    }
                    ops.push(*it);
                    continue;
                }
            }
            else if((*it)->s == "*"||(*it)->s == "/"||(*it)->s == "MOD"){
                if(ops.empty()||ops.top()->s == "("||ops.top()->s == "+"||ops.top()->s == "-"){
                    ops.push(*it);
                    continue;
                }

                else{
                    //其他情况弹出到空或者左括号+-出现
                    while(!ops.empty() && ops.top()->s != "("
                           &&ops.top()->s == "+"&&ops.top()->s == "-"){
                        if(nums.size()<2){
                            t.errorNum = 7;
                            return nullptr;
                        }
                        TreeNode *tmp = ops.pop();
                        TreeNode *tmp2 = nums.pop();
                        TreeNode *tmp1 = nums.pop();
                        tmp->childNodes.push_back(tmp1);
                        tmp->childNodes.push_back(tmp2);
                        nums.push(tmp);
                    }
                    ops.push(*it);
                }
            }
            else if((*it)->s == "**"){
                //右结合，直接压栈
                ops.push(*it);
                continue;
            }
        }
        else{
            t.errorNum = 7;
            return nullptr;
        }
    }
    //再弹、清理一次
    while(!ops.empty() && nums.size()!=1){
        TreeNode *tmp = ops.pop();
        TreeNode *tmp2 = nums.pop();
        TreeNode *tmp1 = nums.pop();
        tmp->childNodes.push_back(tmp1);
        tmp->childNodes.push_back(tmp2);
        nums.push(tmp);
    }

    //运行结束时，nums中仅剩需要的头节点
    //ops为空
    //不符合的认为表达式不合法
    if(!ops.empty() || nums.size()!=1){
        t.errorNum = 7;
        return nullptr;
    }

    return nums.pop();
}

int ParsingTree::getPriority(TreeNode *op){
    if(op->s == "**"){
        return 3;
    }
    else if(op->s == "*"||op->s == "/"||op->s == "MOD"){
        return 2;
    }
    else if(op->s == "+"||op->s == "-"){
        return 1;
    }
    else if(op->s == "("||op->s == ")"){
        return 0;
    }

}
