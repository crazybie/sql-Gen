#include "stdafx.h"
#include "SqlGen.h"
#include <stdarg.h>
#include <memory>

//#define STD_STREAM

#ifdef STD_STREAM
#include <sstream>
#endif

#ifdef _DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif


namespace sqlgen 
{
#ifdef STD_STREAM
    using std::stringstream;
#else
    struct stringstream
    {
        char temp[128];
        string buf;
        stringstream() { buf.reserve(1024);}
        stringstream& operator<<(int t)             { sprintf_s(temp, "%d", t); buf+=temp; return *this;}
        stringstream& operator<<(const char* t)     { buf+=t; return *this;}
        stringstream& operator<<(const string& t)   { buf+=t; return *this;}
        stringstream& operator<<(const float t)     { sprintf_s(temp, "%f", t); buf+=temp; return *this;}
        string str() { return std::move(buf); }
    };
#endif

    struct GenContext
    {
        bool useFullFieldName;
        bool useBraces;
        stringstream s;

        GenContext()
            :useFullFieldName(false)
            ,useBraces(false)
        {}

        GenContext& operator<<(int t)           { s<<t; return *this;}
        GenContext& operator<<(const char* t)   { s<<t; return *this;}
        GenContext& operator<<(const string& t) { s<<t; return *this;}
        GenContext& operator<<(float t)         { s<<t; return *this;}
        string str(){return s.str();}
    };

    static LogAssert assertLogger = puts;

    void setAssertLogger( LogAssert l )
    {
        assertLogger = l;
    }

#if DEBUG
#define sqlAssert(exp, msg, ...) _assert(exp, #exp, __FILE__, __LINE__, msg, __VA_ARGS__)

    void _assert( bool v, const char* exp, const char* f, int line, const char* msg, ... )
    {
        if (v) return;

        char buf[255];
        char* p=buf, *end=buf+sizeof(buf);
        va_list va;
        va_start(va, msg);
        p+=sprintf_s(p, end-p, "#======== Assert ========#\nMsg : ");
        p+=vsprintf_s(p, end-p,  msg, va);
        p+=sprintf_s(p, end-p, "\nExp : %s at %s(%d)\n", exp, f, line);
        p+=sprintf_s(p, end-p, "#========================#\n");
        va_end(va);

        if (assertLogger) assertLogger(buf);
        _asm { int 3 };
    }

#else
#define sqlAssert(...)
#endif
    
    static const char* primaryTypeStr( SqlPrimaryType t )
    {
        const char* s[]={"NoType","Null", "String", "Int", "Bool", "Float", "VarType"};
        return s[t];
    }
        
    SqlPrimaryType BinExp::getSqlType() const
    {
        if (opType <= LogicOpLast) return SqlBool; 
        if (opType == Assign) return SqlNoType;
        return l.getSqlType();
    }

    const char* BinExp::opCppTypeStr( OpType t )
    {
        const char* op[]={
            "&&", "||", ">", "<", "==", ">=", "<=", "!=", "<like>"
            "=", "+", "-", "*", "/", "%",
        };
        return op[t];
    }

    void BinExp::toSql( GenContext& o ) const
    {
        sqlAssert(l.getSqlType()==r.getSqlType(), "left operand type(%s) != right operand type(%s)",
            primaryTypeStr(l.getSqlType()), primaryTypeStr(r.getSqlType()));

        const char* op[]={
            " AND ", " OR ", " > ", " < ", "=", " >= ", " <= ", " <> ", " LIKE ", 
            "=", "+", "-", "*", "/", "%",            
        };

        if (opType==Like){
            sqlAssert(r.getSqlType()==SqlString, "`like` clause need a string, got: %s", primaryTypeStr(r.getSqlType()));
        }

        bool genBraces=o.useBraces;
        if (genBraces) o <<"(";

        if (l.getRtti()==RttiBinExp || r.getRtti()==RttiBinExp) 
            o.useBraces = true;
        else
            o.useBraces = false;
        o << l << op[opType] << r;
        
        if (genBraces) o << ")";
    }

    void Literal::toSql( GenContext& o ) const
    {
        switch(type){
        case SqlString: o << "\'"<<l<<"\'"; break;
        case SqlInt: o<<i; break;
        case SqlFloat: o<<f; break;
        case SqlNull: o<<"NULL"; break;
        }
    }

    void Star::toSql( GenContext& o ) const
    {
        o<<"*";
    }

    void FuncCall::toSql( GenContext& o ) const
    {
        const char* op[]={"DISTINCT", "MAX", "MIN", "AVG", "COUNT", "SUM" };
        o<< op[ftype]<<"(" << arg <<")";
    }

    void Join::toSql( GenContext& o ) const
    {
        sqlAssert(onCond.opType==BinExp::Equ, "join's `on` clause need a equal(==) expression, got: %s.",
            BinExp::opCppTypeStr(onCond.opType));

        o<<fromTable.m_tableName<<" JOIN " << toJoin.m_tableName << " ON " << onCond;
    }

    Join::Join( Table& fromTable_, Table& toJoin_, const BinExp& onCond_ ) :fromTable(fromTable_),toJoin(toJoin_),onCond(onCond_)
    {
        //prevent inlining.
    }


    Field::Field( Table* tb, SqlPrimaryType t, const string& name_ ) :m_table(*tb),m_fieldName(name_),Variable(t,m_fieldName)
    {
        //prevent inlining.
    }

    void Field::toSql( GenContext& o ) const
    {
        if (o.useFullFieldName) o<<m_table.m_tableName<<"."; o<<m_fieldName;
    }

    Field::~Field()
    {
        //prevent inlining.
    }

    void Variable::toSql( GenContext& o ) const
    {
        o<<m_fieldName;
    }

    Table::Table( const string& name ) :m_tableName(name)
    {
        //prevent inlining.
    }

    Table::~Table()
    {
        //prevent inlining.
    }



    //////////////////////////////////////////////////////////////////////////
    

    Select::Select() :m_limit(0),m_offset(0),m_tb(0),m_where(0),m_groupby(0),m_orderby(0),m_join(0),m_having(0)
    {
        //prevent inlining.
        m_fields.reserve(16);
    }

    Select::~Select()
    {
        //prevent inlining.
    }

    Select& Select::select( const Exp& f )
    {
        m_fields.push_back(&f);
        return *this;
    }

    Select& Select::select( const Exp& f, const Exp& f2 )
    {
        m_fields.push_back(&f);
        m_fields.push_back(&f2);
        return *this;
    }

    Select& Select::select( const Exp& f, const Exp& f2, const Exp& f3 )
    {
        m_fields.push_back(&f);
        m_fields.push_back(&f2);
        m_fields.push_back(&f3);
        return *this;
    }

    Select& Select::select( const Exp& f, const Exp& f2, const Exp& f3, const Exp& f4 )
    {
        m_fields.push_back(&f);
        m_fields.push_back(&f2);
        m_fields.push_back(&f3);
        m_fields.push_back(&f4);
        return *this;
    }

    Select& Select::select( const Exp& f, const Exp& f2, const Exp& f3, const Exp& f4, const Exp& f5 )
    {
        m_fields.push_back(&f);
        m_fields.push_back(&f2);
        m_fields.push_back(&f3);
        m_fields.push_back(&f4);
        m_fields.push_back(&f5);
        return *this;
    }       

    string Select::toSql()const
    {
        GenContext o;
        if (m_join) o.useFullFieldName=true;

        o << "SELECT ";
        for(unsigned i=0; i<m_fields.size(); i++){            
            if (i) o <<",";             
            o << *m_fields[i];
        }        
        if (!m_fields.size()) o << "*";
        if (m_tb || m_join) {
            o << " FROM ";
            if (m_tb) o << m_tb->m_tableName;
            if (m_join) o << *m_join;

            if (m_where) {            
                sqlAssert(m_where->getSqlType()==SqlBool, "`where` clause need a bool expression, got: %s", primaryTypeStr(m_where->getSqlType()));
                o << " WHERE " << *m_where;
            }
        }

        sqlAssert(!m_orderby || !m_groupby, "can't have both `order by` and `group by` clause.");
        if (m_orderby) o << " ORDER BY " << *m_orderby << (m_orderType==OrderAsc?" ASC ":" DESC ");
        if (m_groupby) o << " GROUP BY " << *m_groupby;

        if (m_having) o << " HAVING " << *m_having;

        sqlAssert(m_limit >= 0, "limit must be a positive value. got: %d", m_limit);
        if (m_limit) o << " LIMIT " << m_limit;

        sqlAssert(m_offset >= 0, "offset must be a positive value. got: %d", m_offset);
        if (m_offset) o << " OFFSET " << m_offset;
        return o.str();
    }

    //////////////////////////////////////////////////////////////////////////

    Update::Update()
    {
        //prevent inlining.
        m_values.reserve(16);
    }

    Update::~Update()
    {
        //prevent inlining.
    }

    Update& Update::set( const BinExp& v )
    {
        m_values.push_back(&v);
        return *this;
    }

    Update& Update::set( const BinExp& v, const BinExp& v2 )
    {
        m_values.push_back(&v);
        m_values.push_back(&v2);
        return *this;
    }

    Update& Update::set( const BinExp& v, const BinExp& v2, const BinExp& v3 )
    {
        m_values.push_back(&v);
        m_values.push_back(&v2);
        m_values.push_back(&v3);
        return *this;
    }

    Update& Update::set( const BinExp& v, const BinExp& v2, const BinExp& v3, const BinExp& v4 )
    {
        m_values.push_back(&v);
        m_values.push_back(&v2);
        m_values.push_back(&v3);
        m_values.push_back(&v4);
        return *this;
    }

    Update& Update::set( const BinExp& v, const BinExp& v2, const BinExp& v3, const BinExp& v4, const BinExp& v5 )
    {        
        m_values.push_back(&v);
        m_values.push_back(&v2);
        m_values.push_back(&v3);
        m_values.push_back(&v4);
        m_values.push_back(&v5);
        return *this;
    }

    string Update::toSql() const
    {        
        GenContext o;
        o << "UPDATE " << m_table->m_tableName <<" SET ";
        for(unsigned i=0; i<m_values.size(); i++){
            const BinExp& v=*m_values[i];
            sqlAssert(v.opType==BinExp::Assign, "`set` clause need an assign expression, got: %s", BinExp::opCppTypeStr(v.opType));
            if (i) o<<",";
            o<< v;
        }
        if (m_where) {
            sqlAssert(m_where->getSqlType()==SqlBool, "`where` clause need a bool exp, got: %s", primaryTypeStr(m_where->getSqlType())); 
            o << " WHERE " << *m_where;
        }
        return o.str();
    }

    //////////////////////////////////////////////////////////////////////////

    Insert::Insert():m_numcols(0)
    {
        //prevent inlining.
        m_values.reserve(32);
    }

    Insert::~Insert()
    {
        //prevent inlining.
    }

    Insert& Insert::values( const BinExp& v )
    {
        setNumColums(1);
        m_values.push_back(&v);
        return *this;
    }

    Insert& Insert::values( const BinExp& v, const BinExp& v2 )
    {
        setNumColums(2);
        m_values.push_back(&v);
        m_values.push_back(&v2);
        return *this;
    }

    Insert& Insert::values( const BinExp& v, const BinExp& v2, const BinExp& v3 )
    {
        setNumColums(3);
        m_values.push_back(&v);
        m_values.push_back(&v2);
        m_values.push_back(&v3);
        return *this;
    }

    Insert& Insert::values( const BinExp& v, const BinExp& v2, const BinExp& v3, const BinExp& v4 )
    {
        setNumColums(4);
        m_values.push_back(&v);
        m_values.push_back(&v2);
        m_values.push_back(&v3);
        m_values.push_back(&v4);
        return *this;
    }

    Insert& Insert::values( const BinExp& v, const BinExp& v2, const BinExp& v3, const BinExp& v4, const BinExp& v5 )
    {
        setNumColums(5);
        m_values.push_back(&v);
        m_values.push_back(&v2);
        m_values.push_back(&v3);
        m_values.push_back(&v4);
        m_values.push_back(&v5);
        return *this;
    }

    void Insert::setNumColums( int numCols )
    {        
        if (m_numcols){
            sqlAssert(m_numcols==numCols, "values should have same number of colums. expect %d, got %d.", m_numcols, numCols);
        }
        m_numcols = numCols;
    }

    string Insert::toSql()const
    {
        GenContext o;

        o << "INSERT INTO " << m_table->m_tableName << "(";
        for(int i=0; i<m_numcols; i++){
            const BinExp& v = *m_values[i];
            if (i>0) o<<",";
            o << v.l;
        }  
        o << ") VALUES ";

        int nrow = m_values.size()/m_numcols;
        int i=0;
        for(int r=0; r<nrow; r++){
            if (r) o<<",";
            for(int c=0; c<m_numcols; c++){
                const BinExp& v = *m_values[i++];
                sqlAssert(v.opType==BinExp::Assign, "`values()` function need assign expressions, got: %s", BinExp::opCppTypeStr(v.opType)); 

                if (!c) o<<"(";
                else o<<",";
                o << v.r;
                if (c==m_numcols-1) o << ")";
            }            
        }
        return o.str();
    }

    //////////////////////////////////////////////////////////////////////////

    string Delete::toSql() const
    {
        GenContext o;
        o << "DELETE FROM " << m_table->m_tableName;
        if (m_where) o << " WHERE "<< *m_where;
        return o.str();
    }


}

