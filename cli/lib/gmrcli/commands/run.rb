# frozen_string_literal: true

module Gmrcli
  module Commands
    # Run command - runs GMR games
    #
    # The run command looks for executables in the project's release directory,
    # not the engine directory. This matches the build command behavior where
    # build output goes to the project directory.
    class Run
      attr_reader :options, :project_dir

      def initialize(options = {})
        @options = {
          project: nil,
          port: 8080
        }.merge(options)

        # Determine project directory early
        @project_dir = resolve_project_dir
      end

      # Run the native executable
      def native
        exe_path = find_executable

        UI.info "Running: #{exe_path}"
        UI.info "Project: #{project_dir}"
        UI.blank

        # Run gmr.exe with the project directory as working directory
        Dir.chdir(project_dir) do
          if Platform.windows?
            exec("\"#{exe_path}\"")
          else
            exec(exe_path)
          end
        end
      end

      # Start a local web server for the web build
      def web
        # Look in project's release/web directory
        web_dir = File.join(project_dir, "release", "web")
        html_file = File.join(web_dir, "gmr.html")

        unless File.exist?(html_file)
          raise MissingFileError.new(
            "Web build not found",
            details: html_file,
            suggestions: ["Run 'gmrcli build web' first"]
          )
        end

        port = options[:port]
        UI.info "Starting web server on http://localhost:#{port}"
        UI.info "Open http://localhost:#{port}/gmr.html in your browser"
        UI.blank
        UI.warn "Press Ctrl+C to stop"
        UI.blank

        Dir.chdir(web_dir) do
          start_http_server(port)
        end
      end

      private

      def resolve_project_dir
        # Use explicit project option, or current directory
        project_dir = options[:project] || Dir.pwd
        project_dir = File.expand_path(project_dir)

        # Validate it's a GMR project
        scripts_dir = File.join(project_dir, "game", "scripts")
        main_rb = File.join(scripts_dir, "main.rb")

        unless File.exist?(main_rb)
          raise NotAProjectError.new(
            "Not a GMR project: #{project_dir}",
            details: "Missing game/scripts/main.rb",
            suggestions: [
              "Run 'gmrcli new <name>' to create a new project",
              "Make sure you're in a GMR project directory"
            ]
          )
        end

        project_dir
      end

      def find_executable
        exe_name = Platform.gmr_executable

        # Search order:
        # 1. GMR_EXE environment variable
        # 2. Project's release directory (where builds output to)
        # 3. Engine's release directory (legacy/fallback)
        # 4. System PATH

        # Check environment variable
        if ENV["GMR_EXE"]
          path = ENV["GMR_EXE"]
          return path if File.exist?(path)

          UI.warn "GMR_EXE set but file not found: #{path}"
        end

        # Check project's release directory (primary location)
        project_exe = File.join(project_dir, "release", exe_name)
        return project_exe if File.exist?(project_exe)

        # Check engine's release directory (fallback for legacy builds)
        engine_exe = File.join(Platform.gmr_root, "release", exe_name)
        return engine_exe if File.exist?(engine_exe)

        # Check system PATH
        if Platform.command_exists?(exe_name)
          path = Platform.command_path(exe_name)
          return path if path && File.exist?(path)
        end

        # Not found
        raise MissingFileError.new(
          "GMR executable not found",
          suggestions: [
            "Run 'gmrcli build debug' to build the project first",
            "Set GMR_EXE environment variable to the executable path",
            "Add GMR to your system PATH"
          ]
        )
      end

      def start_http_server(port)
        require "webrick"

        server = WEBrick::HTTPServer.new(
          Port: port,
          DocumentRoot: Dir.pwd,
          Logger: WEBrick::Log.new($stderr, WEBrick::Log::INFO),
          AccessLog: [[File.open(File::NULL, "w"), WEBrick::AccessLog::COMMON_LOG_FORMAT]]
        )

        trap("INT") { server.shutdown }
        trap("TERM") { server.shutdown }

        server.start
      end
    end
  end
end
