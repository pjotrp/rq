require 'rubygems'
require 'bundler'
begin
  Bundler.setup(:default, :development)
rescue Bundler::BundlerError => e
  $stderr.puts e.message
  $stderr.puts "Run `bundle install` to install missing gems"
  exit e.status_code
end
require 'rake'

require 'jeweler'
Jeweler::Tasks.new do |gem|
  # gem is a Gem::Specification... see http://docs.rubygems.org/read/chapter/20 for more options
  gem.name = "rq"
  gem.homepage = "http://github.com/pjotrp/rq"
  gem.license = "BSD"
  gem.summary = %Q{Ruby Queue scheduler}
  gem.description = %Q{Zero configuration job scheduler for multi-core and computer clusters}
  gem.email = "pjotr.public01@thebird.nl"
  gem.authors = ["Pjotr Prins"]
  gem.rubyforge_project = "nowarning"
  gem.files += Dir['lib/**/*'] + Dir['ext/**/*']
  # Include your dependencies below. Runtime dependencies are required when using your gem,
  # and development dependencies are only needed for development (ie running rake tasks, tests, etc)
  #  gem.add_runtime_dependency 'jabber4r', '> 0.1'
  #  gem.add_development_dependency 'rspec', '> 1.2.3'
end
Jeweler::RubygemsDotOrgTasks.new

task :default => :spec

task :test do
  Dir.chdir('test') do 
    sh '../bin/test_rq.rb'
  end
end


