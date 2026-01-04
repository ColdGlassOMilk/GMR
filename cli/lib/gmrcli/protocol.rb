# frozen_string_literal: true

module Gmrcli
  # Protocol versioning for machine-readable output
  # Increment major version for breaking changes to JSON schema
  module Protocol
    # Current protocol version
    VERSION = "v1"

    # All supported protocol versions (for negotiation)
    SUPPORTED = ["v1"].freeze

    class << self
      # Returns the current protocol version
      def current
        VERSION
      end

      # Check if a protocol version is supported
      def supported?(version)
        SUPPORTED.include?(version)
      end

      # Validate requested protocol version
      # Returns the version to use, or raises if unsupported
      def negotiate(requested)
        return VERSION if requested.nil?

        if supported?(requested)
          requested
        else
          raise UnsupportedProtocolError.new(
            "Unsupported protocol version: #{requested}",
            requested: requested,
            supported: SUPPORTED
          )
        end
      end
    end
  end

  # Error for unsupported protocol versions
  class UnsupportedProtocolError < StandardError
    attr_reader :requested, :supported

    def initialize(message, requested:, supported:)
      super(message)
      @requested = requested
      @supported = supported
    end
  end
end
