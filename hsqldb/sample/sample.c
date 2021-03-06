/*
 * @(#)$Id: sample.c 3635 2010-06-07 00:18:34Z unsaved $
 *
 * HyperSQL Database Engine
 *
 * Copyright (c) 2009, The HSQL Development Group
 */


#include <stdio.h>
#ifdef _WINDOWS
#include <windows.h>
#endif
#include <sqlext.h>
// sqlext.h pulls in all other ODBC header files that we need
#include <string.h>
#include <stdlib.h>

extern int detectOdbcFailure(SQLRETURN rv, SQLHENV c, char* failMsg);
extern int print_ret(char* msg, int retval);
extern int print2_ret(char* msg, char* msg2, int retval);

/**
 * This test HyperSQL client uses the ODBC DSN "tstdsn" to connect up to a
 * HyperSQL server.  Just configure your own DSN to use the HyperSQL ODBC
 * driver, specifying the HyperSQL server host name, database name, user,
 * password, etc.
 *
 * Sample C program accessing HyperSQL.
 *
 * ODBC C API ref at
 *  http://msdn.microsoft.com/en-us/library/ms714562(VS.85).aspx .
 * Summary of functions at
 *  http://msdn.microsoft.com/en-us/library/ms712628(VS.85).aspx
 *
 * To build on UNIX with unixODBC:<PRE><CODE>
 *     gcc -lodbc -o sample sample.c
 * </CODE></PRE>
 *
 * To build in Windows with MSVC++ (Express variant is free):<PRE><CODE>
 *      cl /nologo /D _WINDOWS /D ODBCVER=0x0351 /c sample.c
 *      link odbc32.lib /nologo /machine:x86 sample.obj /out:sample.exe
 * </CODE></PRE>
 *
 * @author Blaine Simpson (blaine dot simpson at admc dot com)
 */
int main(int argc, char** argv) {
    SQLRETURN odbcret;
    SQLHENV sqlhenv;
    SQLHENV conn;
    SQLHSTMT stmt;
    char *cp;
    long in_idval;
    const int cstrmax = 100;
    char *in_vcval = malloc(cstrmax);
    long out_idval;
    char *out_vcval = malloc(cstrmax);
    char *out_etimeval = malloc(cstrmax);
    SQLLEN ntsval = SQL_NTS;
    int detect;

    // I. CONNECT
    odbcret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlhenv);
    if (odbcret != SQL_SUCCESS && odbcret != SQL_SUCCESS_WITH_INFO)
        return print_ret("Failed to allocate an ODBC environment handle", 1);

    odbcret =
        SQLSetEnvAttr(sqlhenv, SQL_ATTR_ODBC_VERSION, (void*) SQL_OV_ODBC3, 0);
    if (odbcret != SQL_SUCCESS && odbcret != SQL_SUCCESS_WITH_INFO)
        return print_ret("Failed to set ODBC version 3.0", 2);

    odbcret = SQLAllocHandle(SQL_HANDLE_DBC, sqlhenv, &conn);
    if (odbcret != SQL_SUCCESS && odbcret != SQL_SUCCESS_WITH_INFO)
        return print_ret("Failed to allocate an ODBC connection handle", 3);

    odbcret = SQLSetConnectAttr(
            conn, SQL_ATTR_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF, 0);
    if (odbcret != SQL_SUCCESS && odbcret != SQL_SUCCESS_WITH_INFO)
        return print_ret("Failed to allocate an ODBC connection handle", 3);
    // May also want to set timeout values in the same way

    // Can override the DSN-defined user name and/or password here:
    detect = detectOdbcFailure(
            SQLConnect(conn, (SQLCHAR*) "tstdsn", SQL_NTS, (SQLCHAR*) NULL, 0,
                (SQLCHAR*) NULL, 0),
            conn, "Connection failure");
    if (detect) return detect;


    // II. PREPARE OBJECTS FOR USE
    detect = detectOdbcFailure(
            SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt), conn,
            "Failed to allocate an ODBC statement handle");
    if (detect) return detect;

    // Just using this char pointer because some non-ANSI compilers won't let
    // us declare a char array/pointer here.
    cp = "DROP TABLE tsttbl IF EXISTS";
    detect = detectOdbcFailure(SQLExecDirect(stmt, cp, SQL_NTS), conn,
            "DROP statement failed");
    if (detect) return detect;

    // Some recent change to the HyperSQL server or to unixODBC
    // has made this commit necessary, at least on UNIX.  Some other
    // transaction control command would probably be more
    // appropriate here.
    detect = detectOdbcFailure(SQLEndTran(SQL_HANDLE_DBC, conn, SQL_COMMIT),
            conn, "COMMIT failed");
    if (detect) return detect;

    cp = "CREATE TABLE tsttbl(\n\
    id BIGINT generated BY DEFAULT AS IDENTITY,\n\
    vc VARCHAR(20),\n\
    entrytime TIMESTAMP DEFAULT current_timestamp NOT NULL\n\
)";
    detect = detectOdbcFailure(SQLExecDirect(stmt, cp, SQL_NTS), conn,
            "CREATE TABLE statement failed");
    if (detect) return detect;

    detect = detectOdbcFailure(SQLCloseCursor(stmt), conn,
            "Failed to close Cursor for re-use");
    if (detect) return detect;


    // III. INSERT DATA
    // Non-parameter INSERT
    cp = "INSERT INTO tsttbl (id, vc) values (1, 'one')";
    detect = detectOdbcFailure(SQLExecDirect(stmt, cp, SQL_NTS), conn,
            "1st Insertion failed");
    if (detect) return detect;

#ifdef _WINDOWS
    // TODO:  PROBLEM with Parameterized INPUT in Windows (works fine on UNIX).
    // For some reason, even if we are do a Prepare/Execute (and our
    // driver is set to always use server-side Preparation), the client side
    // is doing the substitution... and doing a bad Lob of it too.
    // Therefore, we do all INSERTs statically for Windows here:
    cp = "INSERT INTO tsttbl (id, vc) values (2, 'two')";
    detect = detectOdbcFailure(SQLExecDirect(stmt, cp, SQL_NTS), conn,
            "2nd Insertion failed");
    if (detect) return detect;
    cp = "INSERT INTO tsttbl (id, vc) values (3, 'three')";
    detect = detectOdbcFailure(SQLExecDirect(stmt, cp, SQL_NTS), conn,
            "3rd Insertion failed");
    if (detect) return detect;
    cp = "INSERT INTO tsttbl (id, vc) values (4, 'four')";
    detect = detectOdbcFailure(SQLExecDirect(stmt, cp, SQL_NTS), conn,
            "4th Insertion failed");
    if (detect) return detect;
    cp = "INSERT INTO tsttbl (id, vc) values (5, 'five')";
    detect = detectOdbcFailure(SQLExecDirect(stmt, cp, SQL_NTS), conn,
            "5th Insertion failed");
    if (detect) return detect;
#else
    // Parameterized INSERT
    cp = "INSERT INTO tsttbl (id, vc) values (?, ?)";
    detect = detectOdbcFailure(SQLPrepare(stmt, (SQLCHAR*) cp, SQL_NTS), conn,
        "Preparation of Insertion stmt failed");
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_BIGINT,
            0, 0, &in_idval, 0, NULL), conn,
            "Bind of 'id' input failed");
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
            20, 0, in_vcval, cstrmax, &ntsval), conn,
            "Bind of 'vc' input failed");
    if (detect) return detect;

    in_idval = 2;
    strcpy(in_vcval, "two");
    detect = detectOdbcFailure(SQLExecute(stmt), conn,
            "Insertion of 2nd row failed");
    if (detect) return detect;
    in_idval = 3;
    strcpy(in_vcval, "three");
    detect = detectOdbcFailure(SQLExecute(stmt), conn,
            "Insertion of 3rd row failed");
    if (detect) return detect;
    in_idval = 4;
    strcpy(in_vcval, "four");
    detect = detectOdbcFailure(SQLExecute(stmt), conn,
            "Insertion of 4th row failed");
    if (detect) return detect;
    in_idval = 5;
    strcpy(in_vcval, "five");
    detect = detectOdbcFailure(SQLExecute(stmt), conn,
            "Insertion of 5th row failed");
    if (detect) return detect;
#endif

    detect = detectOdbcFailure(SQLEndTran(SQL_HANDLE_DBC, conn, SQL_COMMIT),
            conn, "COMMIT failed");
    if (detect) return detect;
    
    detect = detectOdbcFailure(SQLCloseCursor(stmt), conn,
            "Failed to close Cursor for re-use");
    if (detect) return detect;


    // IV. QUERIES
    // Non-Parameter QUERY
    cp = "SELECT * FROM tsttbl WHERE id < 3";
    detect = detectOdbcFailure(SQLExecDirect(stmt, cp, SQL_NTS), conn,
            "Non-parameter query failed");
    // Would return SQL_NO_DATA if no rows inserted.
    // Don't need to bind until before fetches are performed.
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindCol(stmt, 1, SQL_C_SLONG, &out_idval, 0, NULL), conn,
            "Bind of 'id' output failed");
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindCol(stmt, 2, SQL_C_CHAR, out_vcval, cstrmax, &ntsval),
            conn, "Bind of 'vc' output failed");
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindCol(stmt, 3, SQL_C_CHAR, out_etimeval, cstrmax, &ntsval),
            conn, "Bind of 'entrytime' output failed");
    if (detect) return detect;

    while ((odbcret = SQLFetch(stmt)) != SQL_NO_DATA) {
        if (detectOdbcFailure(odbcret, conn, "Fetch failed")) return detect;
        printf("%dl|%s|%s\n", out_idval, out_vcval, out_etimeval);
    }
    
    detect = detectOdbcFailure(SQLCloseCursor(stmt), conn,
            "Failed to close Cursor for re-use");
    if (detect) return detect;

#if _WINDOWS
    // Input parameters not working on Windows.  See comment above.
    cp = "SELECT * FROM tsttbl WHERE id > 3";
    detect = detectOdbcFailure(SQLExecDirect(stmt, cp, SQL_NTS), conn,
            "Non-parameter query failed");
    // Would return SQL_NO_DATA if no rows inserted.
    // Don't need to bind until before fetches are performed.
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindCol(stmt, 1, SQL_C_SLONG, &out_idval, 0, NULL), conn,
            "Bind of 'id' output failed");
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindCol(stmt, 2, SQL_C_CHAR, out_vcval, cstrmax, &ntsval),
            conn, "Bind of 'vc' output failed");
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindCol(stmt, 3, SQL_C_CHAR, out_etimeval, cstrmax, &ntsval),
            conn, "Bind of 'entrytime' output failed");
    if (detect) return detect;

    while ((odbcret = SQLFetch(stmt)) != SQL_NO_DATA) {
        if (detectOdbcFailure(odbcret, conn, "Fetch failed")) return detect;
        printf("%dl|%s|%s\n", out_idval, out_vcval, out_etimeval);
    }
#else

    // Parameterized QUERY
    cp = "SELECT * FROM tsttbl WHERE id > ?";
    detect = detectOdbcFailure(SQLPrepare(stmt, (SQLCHAR*) cp, SQL_NTS), conn,
        "Preparation of Query stmt failed");
    if (detect) return detect;
    in_idval = 3;
    detect = detectOdbcFailure(
            SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_BIGINT,
            0, 0, &in_idval, 0, NULL), conn,
            "Bind of 'id' input failed");
    if (detect) return detect;
    detect = detectOdbcFailure(SQLExecute(stmt), conn,
            "Parameterized query failed");
    // Would return SQL_NO_DATA if no rows selected
    // Don't need to bind until before fetches are performed.
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindCol(stmt, 1, SQL_C_SLONG, &out_idval, 0, NULL), conn,
            "Bind of 'id' output failed");
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindCol(stmt, 2, SQL_C_CHAR, out_vcval, cstrmax, &ntsval),
            conn, "Bind of 'vc' output failed");
    if (detect) return detect;
    detect = detectOdbcFailure(
            SQLBindCol(stmt, 3, SQL_C_CHAR, out_etimeval, cstrmax, &ntsval),
            conn, "Bind of 'entrytime' output failed");
    if (detect) return detect;
#endif

    while ((odbcret = SQLFetch(stmt)) != SQL_NO_DATA) {
        if (detectOdbcFailure(odbcret, conn, "Fetch failed")) return detect;
        printf("%dl|%s|%s\n", out_idval, out_vcval, out_etimeval);
    }
    
    detect = detectOdbcFailure(SQLCloseCursor(stmt), conn,
            "Failed to close Cursor");
    if (detect) return detect;

    SQLDisconnect(conn);
    SQLFreeHandle(SQL_HANDLE_DBC, conn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlhenv);
    //return print_ret("Success", 0);
    return 0;
}

/**
 * Displays error message and prepare for program exit.
 */
int barf(SQLHENV c, char* failMsg) {
    char sqlhmsg[200], sqlhstat[10];
    SQLSMALLINT junksmall;
    SQLINTEGER errint;

    SQLGetDiagRec(SQL_HANDLE_DBC, c, 1, sqlhstat, &errint,
            sqlhmsg, 100, &junksmall);
    return print2_ret(failMsg, sqlhmsg, 1);
}

/**
 * Displays error message and prepare for program exit if the given
 * rv indicates ODBC failure.
 */
int detectOdbcFailure(SQLRETURN rv, SQLHENV c, char* failMsg) {
    if (rv == SQL_SUCCESS || rv == SQL_SUCCESS_WITH_INFO) return 0;
    return barf(c, failMsg);
}

/**
 * 2-param wrapper for print2_ret() function.
 */
int print_ret(char* msg, int retval) {
    return print2_ret(msg, (char*) NULL, retval);
}

/**
 * Displays message to stderr and returns given value.
 *
 * Function name here is a hack, because I don't remember how to overload C
 * functions (in a portable way).
 */
int print2_ret(char* msg, char* msg2, int retval) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    if (msg2 != NULL) {
        fputs(msg2, stderr);
        fputc('\n', stderr);
    }
    return retval;
}
