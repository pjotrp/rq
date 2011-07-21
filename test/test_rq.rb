#!/usr/bin/ruby
#
# Test frame work for rq - tests submitting jobs to a queue,
# and the handling of them. Does not test NFS (yet).
#
# Copyright (C) 2011 Pjotr Prins <pjotr.prins@thebird.nl> 

require 'yaml'

print "rq integration test suite by Pjotr Prins 2011\n"

src = File.join('..',File.dirname(__FILE__))
$rq = File.join(src,'bin','rq');
raise "Run from test folder!" if !File.executable?($rq)
$queue = './test_queue'

def rq_exec(args)
  cmd = $rq + ' ' + $queue + ' ' + args
  print cmd
  system(cmd)
end

def rq_status()
  cmd = $rq + ' ' + $queue + ' status'
  YAML.load(`#{cmd}`)
end

def error(line, msg)
  p rq_status()
  rq_exec('shutdown')
  print `rm -rf #{$queue}`
  $stderr.print(__FILE__," ",line,": ",msg)
  exit 1
end

def test_equal(line,s1,s2) 
  error(line,"#{s1} does not equal #{s2}") if s1 != s2
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
rq_exec("create")
p rq_status()

# get status
status = rq_status()
test_equal(__LINE__,status["exit_status"],{"failures"=>0, "ok"=>0, "successes"=>0})

# submit short job
rq_exec('submit "/bin/ls /etc"')

# first time is pending
test_equal(__LINE__,rq_status()['jobs']['pending'],1)

rq_exec('submit "/bin/ls /etc"')
p rq_status()
test_equal(__LINE__,rq_status()['jobs']['total'],2)

# fire up daemon
rq_exec("feed --daemon --log=rq.log --max_feed=1 --min_sleep 1 --max_sleep 1")
5
sleep(2)
test_equal(__LINE__,rq_status()['jobs']['total'],2)

# get status
status = rq_status()
p status
test_equal(__LINE__,status["jobs"]['total'],2)

# Now add longer jobs
rq_exec('submit "sleep 4"')
status = rq_status()
test_equal(__LINE__,status['jobs']['pending']+status['jobs']['running'],1)
rq_exec('submit "sleep 4"')
p rq_status()
test_equal(__LINE__,rq_status()['jobs']['total'],4)

# Kill rq
rq_exec('shutdown')
status = rq_status()
test_equal(__LINE__,status['jobs']['pending']+status['jobs']['running'],2)
sleep(2)
pstab = `ps xau|grep rq`
# print pstab
pstab.grep("runner|#{$rq}") do | s |
  error(__LINE__,"Still running "+s)
end

print `rm -rf #{$queue}`

# Done!
print <<MSG

Congratulations, all tests passed!

This means rq accepts and executes jobs. Note, the tests 
are not exhaustive, but passing them is a good sign.

MSG


