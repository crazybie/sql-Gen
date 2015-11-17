// Generate at Fri Jan 16 17:26:39 2015
#pragma once
#include "sqlgen.h"
#include "SqlUtils.h"


namespace dao
{
    using namespace sqlgen;
    
    #pragma warning(push)
    #pragma warning(disable:4355) //'this' : used in base member initializer list
    
    struct Class : Table
    {
        Field name                ; 
        Field age                 ; 
        
        Class()
            : Table("Class")
            , name        (this, SqlString   , "name")
            , age         (this, SqlInt      , "age")
        {}
    };
       
    struct Users : Table
    {
        struct Row
        {
            string  name;
            int     age;
            string  addr;
            int     score;
            string  tag;
            
            Row(){}

            Row(SqlResultReader& r)
                : name(fromSql(r, name)) 
                , age(fromSql(r, age))
                , addr(fromSql(r, addr))
                , score(fromSql(r,score))
                , tag(fromSql(r, tag))
            {                
            }              
        };

        Field name                ; 
        Field age                 ; 
        Field addr                ; 
        Field score               ; 
        Field tag                 ; 
        
        Users()
            : Table("Users")
            , name        (this, SqlString   , "name")
            , age         (this, SqlInt      , "age")
            , addr        (this, SqlString   , "addr")
            , score       (this, SqlInt      , "score")
            , tag         (this, SqlString   , "tag")
        {}

#define UnpackRowValues_Users(d, table)  \
        table.name  = d.name    ,       \
        table.age   = d.age     ,       \
        table.addr  = d.addr    ,       \
        table.score = d.score   ,       \
        table.tag   = d.tag
    };
       

    #pragma warning(pop)
}