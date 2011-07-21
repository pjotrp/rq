require 'rake'
require 'rake/packagetask'

task :default => [:package]

Rake::PackageTask.new("rq-ruby1.8", File.new('VERSION').read.strip) do |p|
  p.need_tar = true
  p.package_files.include("Rakefile")
  p.package_files.include(["bin/*","lib/**/*.rb","white_box/*","example/*"])
  p.package_files.include(["contrib/*","INSTALL","Rakefile","README","TODO","TUTORIAL","VERSION"])
end

