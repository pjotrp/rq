#!/usr/bin/ruby
#
# Test frame work for rq
#
# Copyright (C) 2011 Pjotr Prins <pjotr.prins@thebird.nl> 

src = File.join('..',File.dirname(__FILE__))
$rq = File.join(src,'bin','rq');
raise "Run from test folder!" if !File.executable?($rq)
queue = './test_queue'

def rq_exec(args)
  cmd = $rq + ' ' + args.join(' ')
  print cmd
  system(cmd)
end

def test_equal(line,s1,s2) 
  if s1 != s2
    $stderr.print(__FILE__," ",line,": #{s1} does not equal #{s2}")
  end
end

# Check dependencies
print "Running tests...\n"

$:.unshift '/var/lib/gems/1.8/gems/sqlite-ruby-2.2.3/lib/'
require 'sqlite'
require 'sqlite/version'
test_equal(__LINE__,SQLite::Version::STRING,"2.2.3")

# In the first step we set up the queue
print `rm -rf #{queue}`
rq_exec([queue,'-v4','create'])

print "Tests passed!"
