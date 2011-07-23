/* --------------------------------------------------------------------------
 * sqlite.c -- glue code between for the SQLite database and Ruby.
 * Copyright (C) 2003 Jamis Buck (jgb3@email.byu.edu)
 * --------------------------------------------------------------------------
 * The  SQLite/Ruby module  is  free software; you can redistribute it and/or
 * modify  it  under the terms of the GNU General Public License as published
 * by  the  Free  Software  Foundation;  either  version 2 of the License, or
 * (at your option) any later version.
 * 
 * The SQLite/Ruby Interface is free software; you can redistribute it and/or
 * modify  it  under the terms of the BSD License as published by  the  Free
 * Software  Foundation. See also the rq LICENSE file. 
 *
 * The license was changed for older sqlite-1.3.1 in agreement with Jamis Buck.
 *
 * The SQLite/Ruby Interface is distributed in the hope that it will be useful,
 *  but   WITHOUT   ANY   WARRANTY;  without  even  the  implied  warranty  of
 * MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.
 * --------------------------------------------------------------------------
 * This file defines the glue code between the SQLite database engine and
 * the Ruby interpreter.
 *
 * Author: Jamis Buck (jgb3@email.byu.edu)
 * Date: June 2003
 * -------------------------------------------------------------------------- 
 * NOTE: rq still users the older sqlite2 engine and bindings. This will 
 * change.
 */

#include <sqlite.h>

#include <stdio.h>
#include "ruby.h"
#include "stdarg.h"

/* these constants defines the current version of the SQLite/Ruby module */

#define LIB_VERSION_MAJOR  1
#define LIB_VERSION_MINOR  3
#define LIB_VERSION_TINY   1

/* this wraps the sqlite db pointer.  We need to do it this way so that we
 * can keep the object alive even after we've closed the DB handle, and we
 * need to be able to tell the difference between an open and a closed DB
 * handle.  This way, if the 'db' member is NULL, we know the database is
 * not open. */

typedef struct
{
  sqlite *db;
  int     use_array;
} SQLITE_RUBY_DATA;


/* This represents the information for a callback during query execution. */

typedef struct
{
  VALUE callback;         /* the Method object to invoke for each row */
  VALUE arg;              /* the application-defined cookie value to pass to the method */
  VALUE columns;          /* a ruby array of all of the column names */
  VALUE types;            /* a ruby hash of all of the column types (may be null) */
  int   built_columns;    /* whether or not the 'columns' member is valid yet */
  int   do_translate;     /* whether or not to do type translation */
  SQLITE_RUBY_DATA *self; /* a reference to the database instance being queried */
} SQLITE_RUBY_CALLBACK;

/* This represents the information for a callback on a custom SQL function */

typedef struct
{
  VALUE callback;        /* the Method object to invoke for each function invocation */
  VALUE finalize;        /* the Method object to invoke when an aggregate function is finished */
  VALUE arg;             /* the app-defined cookie value */
} SQLITE_CUSTOM_FUNCTION_CB;


/* global variables for defining the classes, modules, and symbols that are used by this
 * module. */

static VALUE mSQLite;
static VALUE cSQLite;
static VALUE cSQLiteTypeTranslator;
static VALUE cSQLiteException;
static VALUE cSQLiteQueryContext;
static VALUE oSQLiteQueryAbort;
static ID    idCallMethod;
static ID    idInstanceEvalMethod;
static ID    idTranslate;
static ID    idFields;
static ID    idFieldsEqual;

static struct {
  char *name;
  VALUE object;
} g_sqlite_exceptions[] = {
  { "OK", 0 },
  { "SQL", 0 },
  { "Internal", 0 },
  { "Permissions", 0 },
  { "Abort", 0 },
  { "Busy", 0 },
  { "Locked", 0 },
  { "OutOfMemory", 0 },
  { "ReadOnly", 0 },
  { "Interrupt", 0 },
  { "IOError", 0 },
  { "Corrupt", 0 },
  { "NotFound", 0 },
  { "Full", 0 },
  { "CantOpen", 0 },
  { "Protocol", 0 },
  { "Empty", 0 },
  { "SchemaChanged", 0 },
  { "TooBig", 0 },
  { "Constraint", 0 },
  { "Mismatch", 0 },
  { "Misuse", 0 },
  { "UnsupportedOSFeature", 0 },
  { "Authorization", 0 },
  { NULL, 0 }
};


/* free's the given database handle when the Ruby engine garbage collects it. */
static void static_free_database_handle( SQLITE_RUBY_DATA *hdb );

/* called when a query is processing another row */
static int static_ruby_sqlite_callback( void *pArg, int argc, char **argv, char **columns );

/* called when a custom SQL function is invoked */
static void static_custom_function_callback( sqlite_func* ctx, int argc, const char **argv );

/* called when a custom aggregate SQL function is invoked */
static void static_custom_aggregate_callback( sqlite_func* ctx, int argc, const char **argv );

/* called when the custom aggregate SQL finalize function is invoked */
static void static_custom_finalize_callback( sqlite_func* ctx );

/* determines whether the given pragma is enabled or not */
static int static_pragma_enabled( sqlite* db, const char *pragma );

/* configure the various exception subclasses used by the module */
static void static_configure_exception_classes();

/* raise an exception */
static void static_raise_db_error( int code, char *msg, ... );


static VALUE static_database_new( VALUE klass,
                                  VALUE dbname,
                                  VALUE mode );

static VALUE static_database_close( VALUE self );

static VALUE static_database_exec( VALUE self,
                                   VALUE sql,
                                   VALUE callback,
                                   VALUE parm );

static VALUE static_last_insert_rowid( VALUE self );

static VALUE static_changes( VALUE self );

static VALUE static_interrupt( VALUE self );

static VALUE static_complete( VALUE self,
                              VALUE sql );

static VALUE static_create_function( VALUE self,
                                     VALUE name,
                                     VALUE argc,
                                     VALUE callback,
                                     VALUE parm );

static VALUE static_create_aggregate( VALUE self,
                                      VALUE name,
                                      VALUE argc,
                                      VALUE step,
                                      VALUE finalize,
                                      VALUE parm );

static VALUE static_aggregate_count( VALUE self );

static VALUE static_get_properties( VALUE self );

static VALUE static_is_doing_type_translation( VALUE self );

static VALUE static_set_type_translation( VALUE self, VALUE value );


static void static_free_database_handle( SQLITE_RUBY_DATA *hdb )
{
  if( hdb )
  {
    if( hdb->db )
    {
      sqlite_close( hdb->db );
      hdb->db = NULL;
    }

    free( hdb );
  }
}


static int static_ruby_sqlite_callback( void *pArg, int argc, char **argv, char **columns )
{
  SQLITE_RUBY_CALLBACK *hook = (SQLITE_RUBY_CALLBACK*)pArg;
  VALUE result;
  int i;
  VALUE rc;

  if( hook->self->use_array )
  {
    result = rb_ary_new();
  }
  else
  {
    result = rb_hash_new();
  }

  if( !hook->built_columns )
  {
    hook->columns = rb_ary_new2( argc );
  }

  for( i = 0; i < argc; i++ )
  {
    VALUE val;

    if( !hook->built_columns )
    {
      rb_ary_push( hook->columns, rb_str_new2( columns[i] ) );
      if( hook->types != Qnil )
      {
        VALUE type;
        char *type_name = columns[ i + argc ];
        
        type = rb_str_new2( type_name == NULL ? "STRING" : type_name );
        rb_hash_aset( hook->types, rb_ary_entry( hook->columns, i ), type );
        rb_hash_aset( hook->types, INT2FIX(i), type );
      }
    }

    if( argv != NULL )
    {
      val = ( argv[i] ? rb_str_new2( argv[i] ) : Qnil );

      if( hook->do_translate )
      {
        VALUE type = rb_hash_aref( hook->types, INT2FIX(i) );
        val = rb_funcall( cSQLiteTypeTranslator, idTranslate, 2, type, val );
      }

      if( hook->self->use_array )
      {
        rb_ary_store( result, i, val );
      }
      else
      {
        rb_hash_aset( result, rb_ary_entry( hook->columns, i ), val );
        rb_hash_aset( result, INT2FIX(i), val );
      }
    }
  }

  if( hook->self->use_array )
  {
    if( rb_respond_to( result, idFieldsEqual ) )
    {
      rb_funcall( result, idFieldsEqual, 1, hook->columns );
    }
    else
    {
      rb_iv_set( result, "@fields", hook->columns );
      if( !rb_respond_to( result, idFields ) )
        rb_funcall( result, idInstanceEvalMethod, 1, rb_str_new2( "def fields;@fields;end" ) );
    }
  }

  hook->built_columns = 1;

  rb_iv_set( result, "@argument", hook->arg );
  rb_funcall( result, idInstanceEvalMethod, 1, rb_str_new2( "def argument;@argument;end" ) );

  if( hook->types != Qnil )
  {
    rb_iv_set( result, "@column_types", hook->types );
    rb_funcall( result, idInstanceEvalMethod, 1, rb_str_new2( "def column_types;@column_types;end" ) );
  }

  rc = rb_funcall( hook->callback, idCallMethod, 1, result );

  return ( rc == oSQLiteQueryAbort ? 1 : 0 );
}


static void static_custom_function_callback( sqlite_func* ctx, int argc, const char **argv )
{
  SQLITE_CUSTOM_FUNCTION_CB *data;
  VALUE rc;
  VALUE args;
  int i;

  data = (SQLITE_CUSTOM_FUNCTION_CB*)sqlite_user_data( ctx );

  args = rb_ary_new2( argc );
  rb_ary_push( args, Data_Wrap_Struct( cSQLiteQueryContext, 0, 0, ctx ) );

  for( i = 0; i < argc; i++ )
  {
    if( argv[i] )
      rb_ary_push( args, rb_str_new2( argv[i] ) );
    else
      rb_ary_push( args, Qnil );
  }

  rc = rb_apply( data->callback, idCallMethod, args );

  switch( TYPE(rc) )
  {
    case T_STRING:
      sqlite_set_result_string( ctx, STR2CSTR(rc), RSTRING(rc)->len );
      break;
    case T_FIXNUM:
      sqlite_set_result_int( ctx, FIX2INT(rc) );
      break;
    case T_FLOAT:
      sqlite_set_result_double( ctx, NUM2DBL(rc) );
      break;
  }
}


static void static_custom_aggregate_callback( sqlite_func* ctx, int argc, const char **argv )
{
  SQLITE_CUSTOM_FUNCTION_CB *data;
  VALUE args;
  VALUE *hash;
  int i;

  hash = (VALUE*)sqlite_aggregate_context( ctx, sizeof( VALUE ) );
  if( *hash == 0 ) *hash = rb_hash_new();

  data = (SQLITE_CUSTOM_FUNCTION_CB*)sqlite_user_data( ctx );

  args = rb_ary_new2( argc );
  rb_ary_push( args, Data_Wrap_Struct( cSQLiteQueryContext, 0, 0, ctx ) );

  for( i = 0; i < argc; i++ )
  {
    if( argv[i] )
      rb_ary_push( args, rb_str_new2( argv[i] ) );
    else
      rb_ary_push( args, Qnil );
  }

  rb_apply( data->callback, idCallMethod, args );
}


static void static_custom_finalize_callback( sqlite_func* ctx )
{
  SQLITE_CUSTOM_FUNCTION_CB *data;
  VALUE rc;
  int i;

  data = (SQLITE_CUSTOM_FUNCTION_CB*)sqlite_user_data( ctx );
  rc = rb_funcall( data->finalize, idCallMethod, 1, Data_Wrap_Struct( cSQLiteQueryContext, 0, 0, ctx ) );

  switch( TYPE(rc) )
  {
    case T_STRING:
      sqlite_set_result_string( ctx, STR2CSTR(rc), RSTRING(rc)->len );
      break;
    case T_FIXNUM:
      sqlite_set_result_int( ctx, FIX2INT(rc) );
      break;
    case T_FLOAT:
      sqlite_set_result_double( ctx, NUM2DBL(rc) );
      break;
  }
}

static int static_pragma_enabled_callback( void *arg, int argc, char **argv, char **columnNames )
{
  int *result = (int*)arg;

  if( ( strcmp( argv[0], "ON" ) == 0 ) || ( strcmp( argv[0], "1" ) == 0 ) )
  {
    *result = 1;
  }
  else
  {
    *result = 0;
  }

  return SQLITE_OK;
}

static int static_pragma_enabled( sqlite* db, const char *pragma )
{
  int result;
  int return_code;
  char sql[ 256 ];
  char *msg;

  sprintf( sql, "PRAGMA %s", pragma );
  return_code = sqlite_exec( db, sql, static_pragma_enabled_callback, &result, &msg );

  if( return_code != SQLITE_OK )
  {
    VALUE err = rb_str_new2( msg );
    free( msg );

    static_raise_db_error( return_code,
                           "could not determine status of pragma '%s' (%s)",
                           pragma, STR2CSTR(err) );
  }

  return result;
}


/**
 * Opens a SQLite database. If the database does not exist, it will be
 * created. The mode parameter is currently unused and should be set to 0.
 */
static VALUE static_database_new( VALUE klass,
                                  VALUE dbname,
                                  VALUE mode )
{
  SQLITE_RUBY_DATA *hdb;
  sqlite *db;
  char   *s_dbname;
  int     i_mode;
  char   *errmsg;
  VALUE   v_db;

  Check_Type( dbname, T_STRING );
  Check_Type( mode,   T_FIXNUM );

  s_dbname = STR2CSTR(dbname);
  i_mode   = FIX2INT(mode);

  db = sqlite_open( s_dbname, i_mode, &errmsg );
  if( db == NULL )
  {
    VALUE err = rb_str_new2( errmsg );
    free( errmsg );

    static_raise_db_error( -1, "%s", STR2CSTR( err ) );
  }

  hdb = ALLOC( SQLITE_RUBY_DATA );
  hdb->db = db;
  hdb->use_array = 0; /* default to FALSE */

  v_db = Data_Wrap_Struct( klass, NULL, static_free_database_handle, hdb );

  static_set_type_translation( v_db, Qfalse );

  return v_db;
}


/**
 * Closes an open database. No further methods should be invoked on the database
 * after closing it.
 */
static VALUE static_database_close( VALUE self )
{
  SQLITE_RUBY_DATA *hdb;

  Data_Get_Struct( self, SQLITE_RUBY_DATA, hdb );
  if( hdb->db != NULL )
  {
    sqlite_close( hdb->db );
    hdb->db = NULL;
  }

  return Qnil;
}


/**
 * This is the base method used for querying the database. You will rarely use this method
 * directly; rather, you should use the #execute method, instead. The +sql+ parameter is
 * the text of the sql to execute, +callback+ is a proc object to be invoked for each row
 * of the result set, and +parm+ is an application-specific cookie value that will be passed to
 * the +callback+.
 */
static VALUE static_database_exec( VALUE self,
                                   VALUE sql,
                                   VALUE callback,
                                   VALUE parm )
{
  SQLITE_RUBY_DATA *hdb;
  SQLITE_RUBY_CALLBACK hook;
  char *s_sql;
  int i;
  char *err = NULL;
  VALUE v_err;

  Check_Type( sql, T_STRING );
  s_sql = STR2CSTR(sql);

  Data_Get_Struct( self, SQLITE_RUBY_DATA, hdb );
  if( hdb->db == NULL )
    static_raise_db_error( -1, "attempt to access a closed database" );

  hook.callback = callback;
  hook.arg = parm;
  hook.built_columns = 0;
  hook.columns = Qnil;
  hook.self = hdb;
  hook.do_translate = ( rb_iv_get( self, "@type_translation" ) == Qtrue );

  if( static_pragma_enabled( hdb->db, "show_datatypes" ) )
  {
    hook.types = rb_hash_new();
  }
  else
  {
    hook.types = Qnil;
    hook.do_translate = 0; /* show_datatypes must be anbled for type translation */
  }

  i = sqlite_exec( hdb->db,
                   s_sql,
                   static_ruby_sqlite_callback,
                   &hook,
                   &err );

  if( err != 0 )
  {
    v_err = rb_str_new2( err );
    free( err );
  }

  switch( i )
  {
    case SQLITE_OK:
    case SQLITE_ABORT:
      break;
    default:
      static_raise_db_error( i, "%s", STR2CSTR(v_err) );
  }

  return INT2FIX(0);
}


/**
 * Returns the key value of the last inserted row.
 */
static VALUE static_last_insert_rowid( VALUE self )
{
  SQLITE_RUBY_DATA *hdb;

  Data_Get_Struct( self, SQLITE_RUBY_DATA, hdb );

  if( hdb->db == NULL )
    static_raise_db_error( -1, "attempt to access a closed database" );

  return INT2FIX( sqlite_last_insert_rowid( hdb->db ) );
}


/**
 * Returns the number of rows that were affected by the last query.
 */
static VALUE static_changes( VALUE self )
{
  SQLITE_RUBY_DATA *hdb;

  Data_Get_Struct( self, SQLITE_RUBY_DATA, hdb );

  if( hdb->db == NULL )
    static_raise_db_error( -1, "attempt to access a closed database" );

  return INT2FIX( sqlite_changes( hdb->db ) );
}


/**
 * Interrupts the currently executing query, causing it to abort. If there
 * is no current query, this does nothing.
 */
static VALUE static_interrupt( VALUE self )
{
  SQLITE_RUBY_DATA *hdb;

  Data_Get_Struct( self, SQLITE_RUBY_DATA, hdb );

  if( hdb->db == NULL )
    static_raise_db_error( -1, "attempt to access a closed database" );

  sqlite_interrupt( hdb->db );

  return Qnil;
}


/**
 * Queries whether or not the given SQL statement is complete or not.
 * This is primarly useful in interactive environments where you are
 * prompting the user for a query, line-by-line.
 */
static VALUE static_complete( VALUE self,
                              VALUE sql )
{
  Check_Type( sql, T_STRING );

  return ( sqlite_complete( STR2CSTR( sql ) ) ? Qtrue : Qfalse );
}

/**
 * Causes the database instance to use an array to represent rows
 * in query results, if 'boolean' is not Qnil or Qfalse.
 */
static VALUE static_set_use_array( VALUE self, VALUE boolean )
{
  int use_array = RTEST( boolean );
  SQLITE_RUBY_DATA *hdb;

  Data_Get_Struct( self, SQLITE_RUBY_DATA, hdb );
  hdb->use_array = use_array;

  return boolean;
}

/**
 * Queries the database instance to determine whether or not arrays
 * are being used to represent rows in query results.
 */
static VALUE static_is_use_array( VALUE self )
{
  SQLITE_RUBY_DATA *hdb;

  Data_Get_Struct( self, SQLITE_RUBY_DATA, hdb );
  return ( hdb->use_array ? Qtrue : Qfalse );
}

/**
 * Defines a custom SQL function with the given name and expected parameter count.
 * The +callback+ must be a proc object, which will be invoked for each row of the
 * result set. The +parm+ will be passed to the callback as well.
 */
static VALUE static_create_function( VALUE self,
                                     VALUE name,
                                     VALUE argc,
                                     VALUE callback,
                                     VALUE parm )
{
  SQLITE_RUBY_DATA *hdb;
  SQLITE_CUSTOM_FUNCTION_CB *data;
  char *s_name;
  int   i_argc;
  int   rc;

  Data_Get_Struct( self, SQLITE_RUBY_DATA, hdb );
  s_name = STR2CSTR( name );
  i_argc = FIX2INT( argc );

  if( hdb->db == NULL )
    static_raise_db_error( -1, "attempt to access a closed database" );

  data = ALLOC( SQLITE_CUSTOM_FUNCTION_CB );
  data->callback = callback;
  data->arg = parm;

  rc = sqlite_create_function( hdb->db,
                               s_name,
                               i_argc,
                               static_custom_function_callback,
                               data );

  if( rc != 0 )
    static_raise_db_error( rc, "error registering custom function" );

  return Qnil;
}


/**
 * Defines a custom aggregate SQL function with the given name and argument count
 * (+argc+). The +step+ parameter must be a proc object, which will be called for
 * each iteration during the processing of the aggregate. The +finalize+ parameter
 * must also be a proc object, and is called at the end of the processing of the
 * aggreate. The +parm+ parameter will be passed to both callbacks.
 */
static VALUE static_create_aggregate( VALUE self,
                                      VALUE name,
                                      VALUE argc,
                                      VALUE step,
                                      VALUE finalize,
                                      VALUE parm )
{
  SQLITE_RUBY_DATA *hdb;
  SQLITE_CUSTOM_FUNCTION_CB *data;
  char *s_name;
  int   i_argc;
  int   rc;

  Data_Get_Struct( self, SQLITE_RUBY_DATA, hdb );
  s_name = STR2CSTR( name );
  i_argc = FIX2INT( argc );

  if( hdb->db == NULL )
    static_raise_db_error( -1, "attempt to access a closed database" );

  data = ALLOC( SQLITE_CUSTOM_FUNCTION_CB );
  data->callback = step;
  data->finalize = finalize;
  data->arg = parm;

  rc = sqlite_create_aggregate( hdb->db,
                                s_name,
                                i_argc,
                                static_custom_aggregate_callback,
                                static_custom_finalize_callback,
                                data );

  if( rc != 0 )
    static_raise_db_error( rc, "error registering custom function" );

  return Qnil;
}


/**
 * Returns the number of rows in the aggregate context.
 */
static VALUE static_aggregate_count( VALUE self )
{
  sqlite_func *ctx;

  Data_Get_Struct( self, sqlite_func, ctx );

  return INT2FIX( sqlite_aggregate_count( ctx ) );
}


/**
 * Returns the properties attribute of the aggregate context. This will always
 * be a Hash object, which your custom functions may use to accumulate information
 * into.
 */
static VALUE static_get_properties( VALUE self )
{
  sqlite_func *ctx;

  Data_Get_Struct( self, sqlite_func, ctx );

  return *(VALUE*)sqlite_aggregate_context( ctx, sizeof( VALUE ) );
}

/**
 * Query whether or not the database is doing automatic type translation between
 * the SQLite type (always a string) and the corresponding Ruby type.
 */
static VALUE static_is_doing_type_translation( VALUE self )
{
  return rb_iv_get( self, "@type_translation" );
}

/**
 * Specify whether or not the database should do automatic type translation.
 */
static VALUE static_set_type_translation( VALUE self, VALUE value )
{
  if( value == Qnil || value == Qfalse )
  {
    value = Qfalse;
  }
  else
  {
    value = Qtrue;
  }

  return rb_iv_set( self, "@type_translation", value );
}

static void static_configure_exception_classes()
{
  int i;

  for( i = 1; g_sqlite_exceptions[ i ].name != NULL; i++ )
  {
    char name[ 128 ];

    sprintf( name, "%sException", g_sqlite_exceptions[ i ].name );
    g_sqlite_exceptions[ i ].object = rb_define_class_under( mSQLite, name, cSQLiteException );
  }
}

static void static_raise_db_error( int code, char *msg, ... )
{
  va_list args;
  char message[ 2048 ];
  VALUE exc;

  va_start( args, msg );
  vsnprintf( message, sizeof( message ), msg, args );
  va_end( args );

  exc = ( code <= 0 ? cSQLiteException : g_sqlite_exceptions[ code ].object );

  rb_raise( exc, message );
}

void Init__sqlite()
{
  VALUE version;

  mSQLite = rb_define_module( "SQLite" );

  idCallMethod  = rb_intern( "call" );
  idInstanceEvalMethod = rb_intern("instance_eval");
  idTranslate = rb_intern("translate");
  idFields = rb_intern("fields");
  idFieldsEqual = rb_intern("fields=");
  
  cSQLite = rb_define_class_under( mSQLite, "Database", rb_cObject );
  cSQLiteException = rb_define_class_under( mSQLite, "DatabaseException", rb_eStandardError );
  cSQLiteQueryContext = rb_define_class_under( mSQLite, "QueryContext", rb_cHash );
  cSQLiteTypeTranslator = rb_define_class_under( mSQLite, "TypeTranslator", rb_cObject );

  static_configure_exception_classes();

  rb_define_singleton_method( cSQLite, "new", static_database_new, 2 );

  rb_define_method( cSQLite, "close", static_database_close, 0 );
  rb_define_method( cSQLite, "exec", static_database_exec, 3 );
  rb_define_method( cSQLite, "last_insert_rowid", static_last_insert_rowid, 0 );
  rb_define_method( cSQLite, "changes", static_changes, 0 );
  rb_define_method( cSQLite, "interrupt", static_interrupt, 0 );
  rb_define_method( cSQLite, "complete?", static_complete, 1 );
  rb_define_method( cSQLite, "create_function", static_create_function, 4 );
  rb_define_method( cSQLite, "create_aggregate", static_create_aggregate, 5 );
  rb_define_method( cSQLite, "type_translation?", static_is_doing_type_translation, 0 );
  rb_define_method( cSQLite, "type_translation=", static_set_type_translation, 1 );
  rb_define_method( cSQLite, "use_array=", static_set_use_array, 1 );
  rb_define_method( cSQLite, "use_array?", static_is_use_array, 0 );

  rb_define_const( cSQLite, "VERSION", rb_str_new2( sqlite_libversion() ) );
  rb_define_const( cSQLite, "ENCODING", rb_str_new2( sqlite_libencoding() ) );

  version = rb_ary_new();
  rb_ary_push( version, INT2FIX( LIB_VERSION_MAJOR ) );
  rb_ary_push( version, INT2FIX( LIB_VERSION_MINOR ) );
  rb_ary_push( version, INT2FIX( LIB_VERSION_TINY ) );
  rb_define_const( mSQLite, "LIB_VERSION", version );

  oSQLiteQueryAbort = rb_funcall( rb_cObject, rb_intern( "new" ), 0 );
  rb_define_const( mSQLite, "ABORT", oSQLiteQueryAbort );

  rb_define_method( cSQLiteQueryContext, "aggregate_count", static_aggregate_count, 0 );
  rb_define_method( cSQLiteQueryContext, "properties", static_get_properties, 0 );
}
