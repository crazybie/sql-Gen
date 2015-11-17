#include "stdafx.h"
#include "SqlUtils.h"


namespace sqlgen{


    const char* MysqlResultReader::nextField()
    {
        if (!current) nextRow(); 
        const char* f = current[curField];
        curField++;
        if (curField >= nfields) {
            curField = 0;
            current = 0;
        }
        return f;
    }

    void MysqlResultReader::nextRow()
    {
        current = mysql_fetch_row(result);
    }

    bool MysqlResultReader::init( MYSQL* con )
    {
        result = mysql_store_result(con);
        if (result) {
            nrows = static_cast<int>( mysql_num_rows(result) );
            nfields = mysql_num_fields(result);
            result = result;
            return true;
        }
        return false;
    }

    MysqlResultReader::~MysqlResultReader()
    {
        if (result) mysql_free_result(result);
    }

}
