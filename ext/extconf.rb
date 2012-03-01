require 'mkmf'

path = File.expand_path(File.dirname __FILE__)
sqlite_dir = "sqlite-2.8.17"
path_to_sqlite = path+"/"+sqlite_dir

Dir.chdir path_to_sqlite
system "./configure --prefix="+path_to_sqlite
system "make && make install"

Dir.chdir path

if (find_library("sqlite","sqlite_open",path_to_sqlite+"/lib") and
    find_library("sqlite","main",path_to_sqlite+"/lib") and 
    find_header("sqlite.h",path_to_sqlite+"/include"))
  create_makefile( "_sqlite4rq" )
end



