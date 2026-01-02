# frozen_string_literal: true

require "json"

require_relative "gmrcli/version"
require_relative "gmrcli/errors"
require_relative "gmrcli/helpers/platform"
require_relative "gmrcli/helpers/ui"
require_relative "gmrcli/helpers/shell"
require_relative "gmrcli/commands/setup"
require_relative "gmrcli/commands/build"
require_relative "gmrcli/commands/run"
require_relative "gmrcli/commands/new"

module Gmrcli
  class << self
    def root
      File.expand_path("../..", __dir__)
    end
  end
end
