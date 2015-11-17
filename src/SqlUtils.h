#pragma once
#include <vector>
#include <string>
#include <stdlib.h>//atoi
#include <functional>


#define SQLGEN_MYSQL

#ifdef SQLGEN_MYSQL
#include <my_global.h>
#include <mysql.h>
#pragma comment(lib, "mysqlclient.lib")
#endif

namespace sqlgen
{
    struct SqlResultReader
    {
        int nrows, nfields;
        virtual const char* nextField() = 0;
        virtual void nextRow() = 0;
    };

    //////////////////////////////////////////////////////////////////////////

    template<typename T>
    struct SqlType 
    {
        static T fromSql(SqlResultReader& r){return T(r); }
    };

    template<typename T>
    T fromSql(SqlResultReader& r, T&){ return SqlType<T>::fromSql(r); }

    //////////////////////////////////////////////////////////////////////////

    template<typename T>
    struct SqlType<T&> : SqlType<T> {};

    template<typename T>
    struct SqlType<const T&> : SqlType<T> {};

    template<>
    struct SqlType<int>
    {        
        static int fromSql(SqlResultReader& r){ return atoi(r.nextField()); }
    };

    template<>
    struct SqlType<std::string>
    {
        static std::string fromSql(SqlResultReader& r){ return r.nextField(); }
    };

    template<typename T>
    struct SqlType< std::vector<T> > 
    {
        static std::vector<T> fromSql(SqlResultReader& r)
        {
            std::vector<T> ret;
            ret.reserve(r.nrows);
            for(int i=0; i<r.nrows; i++){
                ret.emplace_back(SqlType<T>::fromSql(r));
            }
            return ret;
        }
    };    


    //////////////////////////////////////////////////////////////////////////

    template<typename A0>
    void unpackResultValues(SqlResultReader& r, const std::function<void(A0)>& f)
    {
        f( SqlType<A0>::fromSql(r) );
    }

    template<typename A0, typename A1>
    void unpackResultValues(SqlResultReader& r, const std::function<void(A0,A1)>& f)
    {
        A0 a0 = SqlType<A0>::fromSql(r);
        A1 a1 = SqlType<A1>::fromSql(r);
        f(a0, a1);
    }

    //////////////////////////////////////////////////////////////////////////

    namespace type_traits
    {
        template <typename T>
        struct function_traits : function_traits<decltype(&T::operator())>
        {};
        // For generic types, directly use the result of the signature of its 'operator()'

        template <typename C, typename R, typename A0>
        struct function_traits<R(C::*)(A0) const>    
        { 
            typedef A0 Arg0;
            typedef R (functionSig)(A0);
        };
        
        template <typename C, typename R, typename A0, typename A1>
        struct function_traits<R(C::*)(A0, A1) const>    
        { 
            typedef A0 Arg0;
            typedef A1 Arg1;
            typedef R (functionSig)(A0, A1);
        };
    }


#ifdef SQLGEN_MYSQL

    struct MysqlResultReader : SqlResultReader
    {
        MYSQL_RES* result;
        int curField;
        char** current;

        MysqlResultReader():current(0),curField(0), result(0){}        
        ~MysqlResultReader();
        const char* nextField();
        void nextRow();
        bool init(MYSQL* con);
    };


    template<typename Func>
    void query(const std::string& ss, Func f) 
    {
        mysql_query(con, ss.c_str());
        MysqlResultReader r;
        if (r.init(con)) {
            std::function<typename type_traits::function_traits<Func>::functionSig > ff(f);
            unpackResultValues(r, ff);            
        }
    }

#endif


}
