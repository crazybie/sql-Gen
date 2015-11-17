#ifndef SQLGEN_HPP
#define SQLGEN_HPP

#include <string>
#include <vector>

namespace sqlgen
{
    enum SqlPrimaryType { SqlNoType, SqlNull, SqlString, SqlInt, SqlBool, SqlFloat };

    enum RuntimeType { RttiNone, RttiBinExp, /*TODO*/ };

    struct GenContext;
    using std::string;
    using std::vector;

    typedef int(*LogAssert)(const char*);
    void setAssertLogger(LogAssert l);

 
    struct Exp
    {
        virtual void            toSql(GenContext& o)const = 0;     
        virtual SqlPrimaryType  getSqlType() const=0;
        virtual RuntimeType     getRtti()const{ return RttiNone;}
    };
        
    inline GenContext& operator<<(GenContext& o, const Exp& i){ i.toSql(o); return o; }

    struct Literal : Exp
    {
        SqlPrimaryType type;
        int i;
        float f;
        const char* l;        

        Literal():type(SqlNoType){}
        Literal(const char* l_):l(l_),type(SqlString){}
        Literal(const string& l_):l(l_.c_str()),type(SqlString){}
        Literal(int ii):i(ii),type(SqlInt){}
        Literal(float ff):f(ff),type(SqlFloat){}

        SqlPrimaryType  getSqlType()const{return type;}
        void            toSql(GenContext& o)const;
    };

    struct BinExp : Exp
    {
        enum OpType{ 
            And, Or, LargerThan, LessThan, Equ, LargerEqu, LessEqu, NotEqu, Like, LogicOpLast=Like, 
            Assign, Add, Sub, Mul, Div, Mod,            
        } opType;
        const Exp& l;
        const Exp& r;

        BinExp(OpType t, const Exp& ll, const Exp& rr):opType(t),l(ll),r(rr){}
        SqlPrimaryType      getSqlType()const;
        RuntimeType         getRtti()const{return RttiBinExp;}
        static const char*  opCppTypeStr(OpType t);
        void                toSql(GenContext& o)const;
    };


    struct Variable : Exp
    {
        SqlPrimaryType      m_type;
        const string&       m_fieldName;

        Variable(SqlPrimaryType t, const string& name_):m_type(t),m_fieldName(name_){}
        void            toSql(GenContext& o)const;
        SqlPrimaryType  getSqlType()const{return m_type;}
        BinExp          like(const Literal& s){return BinExp(BinExp::Like, *this, s);}
        BinExp          operator=(const Literal& l){return BinExp(BinExp::Assign, *this, l);}
    };

    struct Star : Exp
    {
        SqlPrimaryType  getSqlType()const{return SqlNoType;}
        void            toSql(GenContext& o)const;
    };

    struct FuncCall : Exp
    {
        const Exp& arg;
        enum FuncType{ Distinct, Max, Min, Avg, Count, Sum, } ftype ;
        
        FuncCall(FuncType t, const Exp& a): ftype(t),arg(a){}
        SqlPrimaryType  getSqlType()const{return arg.getSqlType();}
        void            toSql(GenContext& o)const;
    };

    inline FuncCall max(const Exp& e)       {return FuncCall(FuncCall::Max, e);}
    inline FuncCall avg(const Exp& e)       {return FuncCall(FuncCall::Avg, e);}
    inline FuncCall min(const Exp& e)       {return FuncCall(FuncCall::Min, e);}
    inline FuncCall count(const Exp& e)     {return FuncCall(FuncCall::Count, e);}
    inline FuncCall sum(const Exp& e)       {return FuncCall(FuncCall::Sum, e);}
    inline FuncCall distinct(const Exp& e)  {return FuncCall(FuncCall::Distinct, e);}





    //////////////////////////////////////////////////////////////////////////

    
    struct Table;

    struct Field : Variable
    {
        Table& m_table;
        string m_fieldName;

        Field(Table* tb, SqlPrimaryType t, const string& name_);
        ~Field();
        void            toSql(GenContext& o)const;
        using Variable::operator=;
    };

    struct Join : Exp
    {
        Table& fromTable;
        Table& toJoin;
        const BinExp& onCond;

        Join(Table& fromTable_, Table& toJoin_, const BinExp& onCond_);
        SqlPrimaryType  getSqlType()const{return SqlNoType;}
        void            toSql(GenContext& o)const;
    };

    struct Table
    {
        string m_tableName;

        explicit Table(const string& name);
        ~Table();
        Join join(Table& t, const BinExp& on){ return Join(*this, t, on);}
    };

    //////////////////////////////////////////////////////////////////////////

    
    struct Insert
    {
        const Table*            m_table;        
        vector<const BinExp*>   m_values;
        int                     m_numcols;
        
        Insert();
        ~Insert();
        void    setNumColums(int numCols);
        Insert& insertInto(const Table& tt){m_table=&tt; return *this;}        
        Insert& values(const BinExp& v);
        Insert& values(const BinExp& v, const BinExp& v2);
        Insert& values(const BinExp& v, const BinExp& v2, const BinExp& v3);
        Insert& values(const BinExp& v, const BinExp& v2, const BinExp& v3, const BinExp& v4);
        Insert& values(const BinExp& v, const BinExp& v2, const BinExp& v3, const BinExp& v4, const BinExp& v5);
        string  toSql()const;
        operator string()const{return toSql();}
    };

    enum OrderType
    {
        OrderAsc, 
        OrderDesc
    };

    struct Select
    {
        Table*              m_tb;
        const Exp*          m_where;
        const Field*        m_groupby;
        const Field*        m_orderby;
        OrderType           m_orderType;
        int                 m_limit, m_offset;
        vector<const Exp*>  m_fields;        
        const Join*         m_join;
        const BinExp*       m_having;

        Select();
        ~Select();
        Select& select(const Exp& f);
        Select& select(const Exp& f, const Exp& f2);
        Select& select(const Exp& f, const Exp& f2, const Exp& f3);
        Select& select(const Exp& f, const Exp& f2, const Exp& f3, const Exp& f4);
        Select& select(const Exp& f, const Exp& f2, const Exp& f3, const Exp& f4, const Exp& f5);
        Select& from(Table& t){m_tb = &t; return *this;}
        Select& from(const Join& j){m_join = &j;return *this;}   
        Select& where(const Exp& c){m_where = &c; return *this;}
        Select& groupBy(const Field& c){ m_groupby=&c; return *this; }
        Select& orderBy(const Field& c, OrderType order=OrderAsc){m_orderby=&c; m_orderType=order; return *this;}
        Select& having(const BinExp& c){m_having=&c; return *this;}
        Select& limit(int v){ m_limit=v; return *this; }
        Select& offset(int v){m_offset=v; return *this;}
        string  toSql() const;
        operator string() const{return toSql();}
    };
    
    struct Update 
    {
        Table*                  m_table;
        vector<const BinExp*>   m_values;
        const Exp*              m_where;

        Update();
        ~Update();
        Update& update(Table& t){m_table=&t; return *this; }        
        Update& set(const BinExp& v);
        Update& set(const BinExp& v, const BinExp& v2);
        Update& set(const BinExp& v, const BinExp& v2, const BinExp& v3);
        Update& set(const BinExp& v, const BinExp& v2, const BinExp& v3, const BinExp& v4);
        Update& set(const BinExp& v, const BinExp& v2, const BinExp& v3, const BinExp& v4, const BinExp& v5);        
        Update& where(const Exp& v){m_where = &v;return *this;}
        string  toSql()const;
        operator string()const{return toSql();}
    };


    struct Delete
    {
        Table*      m_table;
        const Exp*  m_where;

        Delete():m_table(0),m_where(0){}
        Delete& from(Table& t){m_table=&t; return *this;}
        Delete& where(const Exp& e){m_where=&e; return *this;}
        string  toSql()const;
        operator string()const{return toSql();}
    };

#define OP +
#define TYPE Add
#include __FILE__

#define OP -
#define TYPE Sub
#include __FILE__

#define OP *
#define TYPE Mul
#include __FILE__

#define OP /
#define TYPE Div
#include __FILE__

#define OP %
#define TYPE Mod
#include __FILE__

#define OP &&
#define TYPE And
#include __FILE__

#define OP ||
#define TYPE Or
#include __FILE__

#define OP >
#define TYPE LargerThan
#include __FILE__

#define OP <
#define TYPE LessThan
#include __FILE__

#define OP ==
#define TYPE Equ
#include __FILE__

#define OP <=
#define TYPE LessEqu
#include __FILE__

#define OP >=
#define TYPE LargerEqu
#include __FILE__

#define OP !=
#define TYPE NotEqu
#include __FILE__
}

#pragma once

#else

    inline const BinExp operator OP (const Exp& l,     const Exp& r)       { return BinExp(BinExp::TYPE, l, r); }
    
    // help the compiler to deduce the type.
    inline const BinExp operator OP (const Field& l,   const Literal& r)   { return BinExp(BinExp::TYPE, l, r); }
    inline const BinExp operator OP (const Exp& l,     const Literal& r)   { return BinExp(BinExp::TYPE, l, r); }
    inline const BinExp operator OP (const Literal& l, const Field& r)     { return BinExp(BinExp::TYPE, l, r); }
    inline const BinExp operator OP (const Literal& l, const Exp& r)       { return BinExp(BinExp::TYPE, l, r); }

#undef OP
#undef TYPE

#endif