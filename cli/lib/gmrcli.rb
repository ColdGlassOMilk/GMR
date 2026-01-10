# frozen_string_literal: true

require "json"

# Core modules
require_relative "gmrcli/version"
require_relative "gmrcli/protocol"
require_relative "gmrcli/error_codes"
require_relative "gmrcli/errors"

# Output layer (machine-first architecture)
require_relative "gmrcli/output/envelope"
require_relative "gmrcli/output/emitter"
require_relative "gmrcli/output/human"

# Helpers
require_relative "gmrcli/helpers/platform"
require_relative "gmrcli/helpers/ui"
require_relative "gmrcli/helpers/shell"

# Commands
require_relative "gmrcli/commands/setup"
require_relative "gmrcli/commands/build"
require_relative "gmrcli/commands/run"
require_relative "gmrcli/commands/new"
require_relative "gmrcli/commands/docs"
require_relative "gmrcli/commands/bump"

module Gmrcli
  class << self
    def root
      File.expand_path("../..", __dir__)
    end
  end
end
