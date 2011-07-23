require 'mkmf'

dir_config( "sqlite" )
have_library( "sqlite" )
if have_header( "sqlite.h" ) and have_library( "sqlite", "sqlite_open" )
  create_makefile( "_sqlite" )
end

