#! /usr/bin/ruby -w
#
# Test frame work for rq - tests submitting jobs to a queue,
# and the handling of them. Does not test NFS (yet).
#
# Copyright (C) 2011 Pjotr Prins <pjotr.prins@thebird.nl> 

require 'yaml'

print "rq integration test suite by Pjotr Prins 2011\n"

src = File.join(File.dirname(__FILE__))
$rq = File.join(src,'rq');
# raise "Run from test folder!" if !File.executable?($rq)
$queue = './test_queue'

def rq_exec(args)
  cmd = $rq + ' ' + $queue + ' ' + args
  print "====> ",cmd," <===="
  system(cmd)
end

def rq_status()
  cmd = $rq + ' ' + $queue + ' status'
  YAML.load(`#{cmd}`)
end

def error(line, msg)
  p rq_status()
  rq_exec('shutdown')
  $stderr.print(__FILE__," ",line,": ",msg)
  exit 1
end

def test_equal(line,s1,s2) 
  error(line,"#{s1} does not equal #{s2}") if s1 != s2
end

def kill_rq()
  pstab = `ps xau|grep rq`
  pstab.split(/\n/).grep(/#{$rq}/) do | s |
    # rq_exec('shutdown')
    s =~ /\S+\s+(\d+)/
    pid = $1
    print "+++#{pid}+++\n"
    system("kill -9 #{pid}")
    sleep(1)
  end
  print `rm -rf #{$queue}`
  pstab = `ps xau|grep rq`
  pstab.split(/\n/).grep(/#{$rq}/) do | s |
    error(__LINE__,"Sorry, still running:\n"+s)
  end
  pstab = `ps xau|grep rq`
  pstab.split(/\n/).grep(/rq_jobrunnerdaemon/) do | s |
    s =~ /\S+\s+(\d+)/
    pid = $1
    print "+++#{pid}+++\n"
    system("kill -9 #{pid}")
    sleep(1)
  end
  pstab = `ps xau|grep rq`
  pstab.split(/\n/).grep(/rq_jobrunnerdaemon/) do | s |
    error(__LINE__,"Still running "+s)
  end
end


# Check dependencies
print "Running tests...\n"

kill_rq()

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
sleep(2)
test_equal(__LINE__,rq_status()['jobs']['total'],2)

# get status
status = rq_status()
p status
test_equal(__LINE__,status["jobs"]['total'],2)

# Now add longer jobs
rq_exec('submit "sleep 15"')  # does not finish
sleep(1)
status = rq_status()
test_equal(__LINE__,status['jobs']['pending']+status['jobs']['running'],1)
rq_exec('submit "sleep 15"')   # will be removed
p rq_status()
test_equal(__LINE__,rq_status()['jobs']['total'],4)

# Delete last job
rq_exec('delete 4')
sleep(2)
print "\n---\n"
print rq_exec('query state=pending')
p rq_status

# Kill rq
rq_exec('shutdown')
status = rq_status()
test_equal(__LINE__,status['jobs']['pending']+status['jobs']['running'],1)
sleep(1)
p rq_status

# Done!
print <<MSG

Congratulations, all tests passed!

This means rq accepts and executes jobs. Note, the tests 
are not exhaustive, but passing them is a good sign.

MSG


