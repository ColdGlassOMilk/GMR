# frozen_string_literal: true

require_relative "lib/gmrcli/version"

Gem::Specification.new do |spec|
  spec.name = "gmrcli"
  spec.version = Gmrcli::VERSION
  spec.authors = ["GMR Contributors"]
  spec.email = [""]

  spec.summary = "CLI utility for GMR (Game Middleware for Ruby)"
  spec.description = "Build, run, and deploy GMR games from the command line"
  spec.homepage = "https://github.com/ColdGlassOMilk/gmr"
  spec.license = "MIT"
  spec.required_ruby_version = ">= 3.0.0"

  spec.files = Dir["lib/**/*", "bin/*", "README.md", "LICENSE"]
  spec.bindir = "bin"
  spec.executables = ["gmrcli"]
  spec.require_paths = ["lib"]

  spec.add_dependency "thor", "~> 1.3"
  spec.add_dependency "tty-spinner", "~> 0.9"
  spec.add_dependency "tty-prompt", "~> 0.23"
  spec.add_dependency "pastel", "~> 0.8"
  spec.add_dependency "webrick", "~> 1.8"
  spec.add_dependency "coderay", "~> 1.1"
end
