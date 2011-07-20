#!/usr/bin/ruby
#
# Test frame work for rq
#
# Copyright (C) 2011 Pjotr Prins <pjotr.prins@thebird.nl> 

src = File.join('..',File.dirname(__FILE__))
$rq = File.join(src,'bin','rq');
queue = './test_queue'

def rq_exec(args)
  cmd = $rq + ' ' + args.join(' ')
  print cmd
  system(cmd)
end

# In the first step we set up the queue
print `rm -rf #{queue}`
rq_exec([queue,'create'])
