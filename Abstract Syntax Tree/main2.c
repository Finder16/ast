#include <stdio.h>
#include "json_c.c"

int ifCount(json_value json);
int IfRealCnt(json_value json);
int ifCountSwitch(json_value json);

//body -> block_items[0~n] -> _nodetype (if, for, while, dowhile)
//if -> iftrue, iffalse -> compound? -> block_items[0~n] -> _nodetype (if, for, while, dowhile)
//if -> iftrue, iffalse -> if -> iftrue, iffalse -> if...
//DoWhile -> stmt ->block_items[0~n] -> _nodetype (if, for, while, dowhile)
//While -> stmt ->block_items[0~n] -> _nodetype (if, for, while, dowhile)
//for -> stmt ->block_items[0~n] -> _nodetype (if, for, while, dowhile)
//switch ->stmt ->block_items[0~n] -> _nodetype (if, for, while, dowhile, switch)

int ifCount(json_value json){
    int count = 0;
    json_value block_items = json_get(json, "block_items");
    json_value blockArr[json_len(block_items)];

    //block_items의 개수만큼 반복
    for(int i=0; i<json_len(block_items); i++){
        blockArr[i] = json_get(block_items, i);
    }


    //If 개수를 세기 위한 재귀 함수
    for(int j = 0; j<json_len(block_items); j++) {
        if (!(strcasecmp(json_get_string(blockArr[j], "_nodetype"), "If"))){
            count++;
            //if는 iftrue, iffalse로 나뉘어져 있음 -> 중첩 if문 전용 함수 돌입
            count += IfRealCnt(blockArr[j]);
        }
        //for, While, DoWhile, Switch는 if가 나올 때 까지 무한 재귀 혹은 종료
        else if (!(strcasecmp(json_get_string(blockArr[j], "_nodetype"), "For"))){
            json_value stmt = json_get(blockArr[j], "stmt");
            count += ifCount(stmt);
        }
        else if (!(strcasecmp(json_get_string(blockArr[j], "_nodetype"), "While"))){
            json_value stmt = json_get(blockArr[j], "stmt");
            count += ifCount(stmt);
        }
        else if (!(strcasecmp(json_get_string(blockArr[j], "_nodetype"), "DoWhile"))){
            json_value stmt = json_get(blockArr[j], "stmt");
            count += ifCount(stmt);
        }
        //Switch문은 구조가 달라 따로 함수를 만들어줌
        else if (!(strcasecmp(json_get_string(blockArr[j], "_nodetype"), "Switch"))){
            json_value stmt = json_get(blockArr[j], "stmt");
            json_value blockItemsSwitch = json_get(stmt, "block_items");
            json_value blockArr_switch[json_len(blockItemsSwitch)];
            for(int k=0; k<json_len(blockItemsSwitch); k++){
                blockArr_switch[k] = json_get(blockItemsSwitch, k);
                count += ifCountSwitch(blockArr_switch[k]);
            }
        }
        else
            continue;
    }
    return count;
}

//block_items[0~n] -> stmts[0~n] -> _nodetype (if, for, while, dowhile, switch)
int ifCountSwitch(json_value json){
    int count = 0;

    json_value stmts = json_get(json, "stmts");
    json_value stmtsArr[json_len(stmts)];

    for(int i=0; i<json_len(stmts); i++){
        stmtsArr[i] = json_get(stmts, i);
    }
    for(int i = 0; i<json_len(stmts);i++){
        if (!(strcasecmp(json_get_string(stmtsArr[i], "_nodetype"), "If"))){
            count++;
            count += IfRealCnt(stmtsArr[i]);
        }
        else if (!(strcasecmp(json_get_string(stmtsArr[i], "_nodetype"), "For"))){
            json_value stmt = json_get(stmtsArr[i], "stmt");
            count += ifCount(stmt);
        }
        else if (!(strcasecmp(json_get_string(stmtsArr[i], "_nodetype"), "While"))){
            json_value stmt = json_get(stmtsArr[i], "stmt");
            count += ifCount(stmt);
        }
        else if (!(strcasecmp(json_get_string(stmtsArr[i], "_nodetype"), "DoWhile"))){
            json_value stmt = json_get(stmtsArr[i], "stmt");
            count += ifCount(stmt);
        }
        else if (!(strcasecmp(json_get_string(stmtsArr[i], "_nodetype"), "Switch"))){
            json_value stmt = json_get(stmtsArr[i], "stmt");
            json_value blockItemsSwitch = json_get(stmt, "block_items");
            json_value blockArr_switch[json_len(blockItemsSwitch)];
            for(int k=0; k<json_len(blockItemsSwitch); k++){
                blockArr_switch[k] = json_get(blockItemsSwitch, k);
                count += ifCountSwitch(blockArr_switch[k]);
            }
        }
        else
            continue;
    }

    return count;
}

//if문 돌입 시 중첩 if문 경우를 위한 함수
int IfRealCnt(json_value json){
    int count = 0;
    json_value iftrue = json_get(json, "iftrue");
    json_value iffalse = json_get(json, "iffalse");
    //if문 아래에 바로 if가 나오지 않으면 ifCount로 복귀
    if(strcasecmp(json_get_string(iftrue, "_nodetype"), "Compound"))
        //중첩 if가 없으면 복귀
        if(strcasecmp(json_get_string(iftrue, "_nodetype"), "If"))
            return count;
        else {
            count++;
            count += IfRealCnt(iftrue);
        }
    else{
        count += ifCount(iftrue);
    }
    //else가 없으면 복귀
    if(json_is_null(iffalse))
        return count;
    else {
        //else 이후 분기문 체크
        if (strcasecmp(json_get_string(iffalse, "_nodetype"), "Compound"))
            //else if 없으면 복귀
            if(strcasecmp(json_get_string(iffalse, "_nodetype"), "If"))
                return count;
            else{
                count++;
                count += IfRealCnt(iffalse);
            }
        else{
            count += ifCount(iffalse);
        }
    }
    return count;
}

char *readFile(char *filename, int *readSize)    // 파일을 읽어서 내용을 반환하>는 함수
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        return NULL;

    int size;
    char *buffer;

    // 파일 크기 구하기
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buffer = malloc(size + 1);
    memset(buffer, 0, size + 1);

    // 파일 내용 읽기
    if (fread(buffer, size, 1, fp) < 1)
    {
        *readSize = 0;
        free(buffer);
        fclose(fp);
        return NULL;
    }

    // 파일 크기를 넘겨줌
    *readSize = size;
    fclose(fp);    // 파일 포인터 닫기
    return buffer;
}

int main( int argc, char *argv[]) {

    system("chcp 65001");
    int size;

    //파일 경로 받음
    char *doc = readFile(argv[1], &size);
    if (doc == NULL)
        return -1;

    json_value json = json_create(doc);

    //ext의 value 뽑아옴
    json_value frameArr = json_get(json, "ext");

    //ext의 value의 개수만큼 반복
    json_value objarr[json_len(frameArr)];
    for(int i=0; i<json_len(frameArr); i++)
    {
        objarr[i] = json_get(frameArr, i);
    }

    //구조체 등은 TypeDef이므로 제외하기 위한 반복문
    int count = 0;
    for(int j = 0; j<json_len(frameArr); j++) {
        if (!(strcasecmp(json_get_string(objarr[j], "_nodetype"), "FuncDef")))
            count++;
        else
            continue;
    }

    printf("이 파일의 함수의 개수: %d\n\n", count);

    //함수 정보 출력
    for(int j = 0; j<json_len(frameArr); j++) {
        //구조체 등은 TypeDef이므로 제외하기 위한 반복문
        if (!(strcasecmp(json_get_string(objarr[j], "_nodetype"), "FuncDef")))
            printf("");
        else
            continue;
        json_value deCl = json_get(objarr[j], "decl");

        //decl -> name의 value를 뽑아옴
        const char *s = json_get_string(deCl, "name");

        printf("함수[%d]의 이름: %s \n", j - (json_len(frameArr) - count) + 1, s);
        json_value type1 = json_get(deCl, "type");
        json_value type2 = json_get(type1, "type");
        json_value type3 = json_get(type2, "type");
        json_value type = json_get(type3, "names");

        printf("└함수[%d]의 리턴 타입: ", j - (json_len(frameArr) - count) + 1);

        //return 타입이 long long int 같은 경우 배열로 저장되기 때문에 반복
        for (int i = 0; i < json_len(type); i++) {
            printf("%s ", json_get_string(type, i));
        }
        printf("\n");

        json_value args;
        json_value params;

        //파라미터가 없는 경우 예외처리
        if (json_is_null(json_get(type1, "args"))) {
            printf("└함수[%d]의 파라미터 타입: 파라미터가 없습니다.\n", j - (json_len(frameArr) - count) + 1);
            printf("└함수[%d]의 파라미터 이름: 파라미터가 없습니다.\n", j - (json_len(frameArr) - count) + 1);
            printf("└If 문의 개수: %d", ifCount(json_get(objarr[j], "body")));
        } else {
            args = json_get(type1, "args");
            params = json_get(args, "params");

            //파라미터가 여러개일 경우 배열로 저장되기 때문에 반복
            json_value paramsArr[json_len(params)];
            for (int i = 0; i < json_len(params); i++) {
                paramsArr[i] = json_get(params, i);
            }
            printf("└함수[%d]의 파라미터 타입: ", j - (json_len(frameArr) - count) + 1);
            json_value param3;

            //파라미터 타입이 long long int 같은 경우 배열로 저장되기 때문에 반복
            for (int k = 0; k < json_len(params); k++) {
                json_value param = json_get(paramsArr[k], "type");
                json_value param2 = json_get(param, "type");

                //구조체 타입의 경우 type을 세번 타고 가야 하기 때문에 예외 처리
                if (!(strcasecmp(json_get_string(param2, "_nodetype"), "TypeDecl"))) {
                    json_value param4 = json_get(param2, "type");
                    param3 = json_get(param4, "names");
                } else
                    param3 = json_get(param2, "names");


                char *param3arr[json_len(param3)];

                printf("(%d)", k + 1);

                //타입 출력
                for (int i = 0; i < json_len(param3); i++) {
                    param3arr[i] = json_get_string(param3, i);
                    printf("%s\t ", param3arr[i]);

                }
            }
            printf("\n");
            printf("└함수[%d]의 파라미터 이름: ", j - (json_len(frameArr) - count) + 1);
            for (int k = 0; k < json_len(params); k++) {
                if(json_is_null(json_get(paramsArr[k], "name"))) {
                    printf("파라미터가 없습니다.\n");
                    break;
                }
                char *param3arr = json_get_string(paramsArr[k], "name");
                printf("(%d)%s\t ", k + 1, param3arr);
            }
            printf("\n");
            printf("└If 문의 개수: %d\n", ifCount(json_get(objarr[j], "body")));
        }
        printf("\n");
    }

    return 0;
}