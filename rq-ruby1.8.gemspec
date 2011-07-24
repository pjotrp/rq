# Generated by jeweler
# DO NOT EDIT THIS FILE DIRECTLY
# Instead, edit Jeweler::Tasks in Rakefile, and run 'rake gemspec'
# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{rq-ruby1.8}
  s.version = "3.4.6"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Pjotr Prins"]
  s.date = %q{2011-07-24}
  s.description = %q{Zero configuration job scheduler for computer clusters}
  s.email = %q{pjotr.public01@thebird.nl}
  s.executables = ["rq", "rqmailer", "test_rq.rb"]
  s.extensions = ["ext/extconf.rb"]
  s.extra_rdoc_files = [
    "LICENSE",
    "README",
    "TODO"
  ]
  s.files = [
    "Gemfile",
    "Gemfile.lock",
    "INSTALL",
    "LICENSE",
    "Makefile",
    "README",
    "Rakefile",
    "TODO",
    "TUTORIAL",
    "VERSION",
    "bin/rq",
    "bin/rqmailer",
    "bin/test_rq.rb",
    "example/a.rb",
    "ext/extconf.rb",
    "ext/sqlite.c",
    "extconf.rb",
    "gemspec.rb",
    "install.rb",
    "lib/rq.rb",
    "lib/rq/arrayfields.rb",
    "lib/rq/backer.rb",
    "lib/rq/configfile.rb",
    "lib/rq/configurator.rb",
    "lib/rq/creator.rb",
    "lib/rq/cron.rb",
    "lib/rq/defaultconfig.txt",
    "lib/rq/deleter.rb",
    "lib/rq/executor.rb",
    "lib/rq/feeder.rb",
    "lib/rq/ioviewer.rb",
    "lib/rq/job.rb",
    "lib/rq/jobqueue.rb",
    "lib/rq/jobrunner.rb",
    "lib/rq/jobrunnerdaemon.rb",
    "lib/rq/lister.rb",
    "lib/rq/locker.rb",
    "lib/rq/lockfile.rb",
    "lib/rq/logging.rb",
    "lib/rq/mainhelper.rb",
    "lib/rq/orderedautohash.rb",
    "lib/rq/orderedhash.rb",
    "lib/rq/qdb.rb",
    "lib/rq/querier.rb",
    "lib/rq/rails.rb",
    "lib/rq/recoverer.rb",
    "lib/rq/refresher.rb",
    "lib/rq/relayer.rb",
    "lib/rq/resource.rb",
    "lib/rq/resourcemanager.rb",
    "lib/rq/resubmitter.rb",
    "lib/rq/rotater.rb",
    "lib/rq/sleepcycle.rb",
    "lib/rq/snapshotter.rb",
    "lib/rq/sqlite.rb",
    "lib/rq/statuslister.rb",
    "lib/rq/submitter.rb",
    "lib/rq/toucher.rb",
    "lib/rq/updater.rb",
    "lib/rq/usage.rb",
    "lib/rq/util.rb",
    "rdoc.sh",
    "rq-ruby1.8.gemspec",
    "test/.gitignore",
    "white_box/crontab",
    "white_box/joblist",
    "white_box/killrq",
    "white_box/rq_killer"
  ]
  s.homepage = %q{http://github.com/pjotrp/rq}
  s.licenses = ["BSD"]
  s.require_paths = ["lib"]
  s.rubygems_version = %q{1.3.7}
  s.summary = %q{Ruby Queue scheduler}

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<posixlock>, [">= 0"])
      s.add_runtime_dependency(%q<arrayfields>, [">= 0"])
      s.add_runtime_dependency(%q<lockfile>, [">= 0"])
      s.add_development_dependency(%q<bundler>, ["~> 1.0.15"])
      s.add_development_dependency(%q<jeweler>, ["~> 1.6.4"])
    else
      s.add_dependency(%q<posixlock>, [">= 0"])
      s.add_dependency(%q<arrayfields>, [">= 0"])
      s.add_dependency(%q<lockfile>, [">= 0"])
      s.add_dependency(%q<bundler>, ["~> 1.0.15"])
      s.add_dependency(%q<jeweler>, ["~> 1.6.4"])
    end
  else
    s.add_dependency(%q<posixlock>, [">= 0"])
    s.add_dependency(%q<arrayfields>, [">= 0"])
    s.add_dependency(%q<lockfile>, [">= 0"])
    s.add_dependency(%q<bundler>, ["~> 1.0.15"])
    s.add_dependency(%q<jeweler>, ["~> 1.6.4"])
  end
end

