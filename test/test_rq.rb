#!/usr/bin/ruby
#
# Test frame work for rq
#
# Copyright (C) 2011 Pjotr Prins <pjotr.prins@thebird.nl> 

require 'yaml'

src = File.join('..',File.dirname(__FILE__))
$rq = File.join(src,'bin','rq');
raise "Run from test folder!" if !File.executable?($rq)
$queue = './test_queue'

def rq_exec(args)
  cmd = $rq + ' ' + $queue + ' ' + args.join(' ')
  print cmd
  `#{cmd}`
end

def error(line, msg)
  $stderr.print(__FILE__," ",line,": ",msg)
  exit 1
end

def test_equal(line,s1,s2) 
  error("#{s1} does not equal #{s2}") if s1 != s2
end

# Check dependencies
print "Running tests...\n"

# $:.unshift '/var/lib/gems/1.8/gems/sqlite-ruby-2.2.3/lib/'
# test_equal(__LINE__,SQLite::Version::STRING,"2.2.3")

gempath = '/var/lib/gems/1.8/gems/sqlite-1.3.1/lib'
error("Expect "+gempath) if !File.directory?(gempath)
$:.unshift gempath
require 'sqlite'

# In the first step we set up the queue
print `rm -rf #{$queue}`
rq_exec(['create'])

# get status
status = YAML.load(rq_exec(['status']))
p status
test_equal(__LINE__,status["exit_status"],{"failures"=>0, "ok"=>0, "successes"=>0})
# test_equal(__LINE__,SQLite::Version::STRING,"2.2.3")
print "All tests passed!"
