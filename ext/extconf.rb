require 'mkmf'

dir_config( "sqlite4rq" )
have_library( "sqlite4rq" )
if have_header( "sqlite.h" ) and have_library( "sqlite", "sqlite_open" )
  create_makefile( "_sqlite4rq" )
end

