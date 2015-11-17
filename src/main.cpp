#include "stdafx.h"
#include "tableDef.h"


using namespace sqlgen;
using namespace dao;

using std::vector;
using std::string;




//#define PROFILE
#define DB_MYSQL
//#define DB_SQLITE

//////////////////////////////////////////////////////////////////////////
int cnt=0;
size_t maxSz=0;

void* operator new(size_t sz) throw(std::bad_alloc){
    cnt++;
    maxSz=__max(maxSz, sz);
    return malloc(sz); 
}
void operator delete(void* p){
    free(p);
}

//////////////////////////////////////////////////////////////////////////




#ifdef PROFILE

void exe(const char* s){}
void open_db(){}
void close_db(){}

#else



#ifdef DB_SQLITE

#include "sqlite/sqlite3.h"
sqlite3* db;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    printf("========================\n");
    for(i=0; i<argc; i++){
        printf("%8s = %-8s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}
void exe(const char* s){
    char* errmsg=0;
    puts(s);
    sqlite3_exec(db, s, callback, 0, &errmsg);
    if (errmsg) printf("sqlite3 errro: %s\n", errmsg);
}
void open_db(){
    sqlite3_open(":memory:", &db);
}
void close_db(){
    sqlite3_close(db);    
}
#endif



#ifdef DB_MYSQL



MYSQL *con;
void open_db(){
    con = mysql_init(NULL);
    if (mysql_real_connect(con, "localhost", "root", "abcd1234", NULL, 0, NULL, 0) == NULL){
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }
    mysql_select_db(con, "test");
}

void close_db(){ mysql_close(con); }

void exe(const char* s){
    puts(s);    
    if (mysql_query(con, s)) {
        fprintf(stderr, "%s\n", mysql_error(con));  
    }
    if (MYSQL_RES *result = mysql_store_result(con)){
        printf("========================\n");
        int num_fields = mysql_num_fields(result);
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) { 
            for(int i = 0; i < num_fields; i++) { 
                printf("%8s ", row[i] ? row[i] : "NULL"); 
            } 
            printf("\n"); 
        }
        mysql_free_result(result);
    }
}

#endif
        


#endif

void exe(const std::string& s){ exe(s.c_str()); }




Users userTable;
Class classTable;

void test()
{
    cnt=0;

    exe("drop table Users");
    exe("drop table Class");
    exe("create table Users(name varchar(255), age int, addr varchar(255), score int, tag varchar(255))");
    exe("create table Class(name varchar(255), age int)");

    string sql;
    
    exe(sql=Select().select(Literal("1")));

    sql=Insert().insertInto(userTable)
        .values(userTable.name="lis", userTable.age=12, userTable.addr="aaaa", userTable.score=1, userTable.tag="t")
        .values(userTable.name="ggs", userTable.age=12, userTable.addr="aaaa", userTable.score=11, userTable.tag="t")
        .values(userTable.name="ggs2", userTable.age=14, userTable.addr="aaaa", userTable.score=33, userTable.tag="k")
        .values(userTable.name="mid", userTable.age=18, userTable.addr="aaaa", userTable.score=44, userTable.tag="k")
        .values(userTable.name="tes", userTable.age=33, userTable.addr="aaaa", userTable.score=55, userTable.tag="k")
        ;
    exe(sql);
    
    sql=Select().select(userTable.name, userTable.age, userTable.addr, userTable.score, userTable.tag)
        .from(userTable)
        .where(userTable.name=="lis" && userTable.age==12 && userTable.addr.like("aa%") && userTable.score>=1 && userTable.tag!="k");
    exe(sql);

        
    sql=Select().from(userTable)
        .where(userTable.age+10>20)
        //.orderBy(userTable.name, Order_Desc);
        .groupBy(userTable.name).having(count(userTable.age) >= 1);
    exe(sql);


    sql=Update().update(userTable)
        .set(userTable.age=33, userTable.name="lis", userTable.addr="bbb", userTable.score=999,userTable.tag="OK")
        .where(userTable.name=="lis");
    exe(sql);
    exe(Select().from(userTable).where(userTable.name=="lis"));
    

    sql=Insert().insertInto(classTable).values(classTable.name="English", classTable.age=18);
    exe(sql);

    
    sql=Select()
        .select(userTable.name,classTable.name)
        .from(userTable.join(classTable, classTable.age==userTable.age))
        .where(userTable.age==18)
        .orderBy(userTable.name, OrderDesc);
    exe(sql);

    exe(Delete().from(classTable).where(classTable.name=="English"));
    exe(Select().select(count(Star())).from(classTable).where(classTable.name=="English"));
}

void testDataQuery()
{
    exe("drop table Users");
    exe("drop table Class");
    exe("create table Users(name varchar(255), age int, addr varchar(255), score int, tag varchar(255))");
    exe("create table Class(name varchar(255), age int)");
    
    Users::Row data;
    data.name="ddd";
    data.age=18;
    data.score=99;
    data.addr="ttt";
    data.tag="k";
    exe(Insert().insertInto(userTable).values(UnpackRowValues_Users(data,userTable)));


    exe(Select().from(userTable));
    query(Select().from(userTable),
        [](const vector<Users::Row>& u){});

    query(Select().select(count(Star()), Literal(1)).from(userTable), 
        [](int a, int b){ });
}

int main()
{
    open_db();

    testDataQuery();

    
#ifdef PROFILE
    for(int i=0;i<100000; i++)
        test();
#else
    test();
#endif
        
    printf("num memory alloc: %d, maxSize:%d\n", cnt, maxSz);

#ifndef PROFILE
    getchar();
#endif

    close_db();
    return 0;
}