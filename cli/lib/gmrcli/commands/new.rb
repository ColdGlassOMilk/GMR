# frozen_string_literal: true

require "fileutils"

module Gmrcli
  module Commands
    # New command - scaffolds new GMR projects
    class New
      TEMPLATES = %w[basic empty].freeze

      attr_reader :options

      def initialize(options = {})
        @options = {
          template: "basic"
        }.merge(options)
      end

      def project(name)
        project_dir = File.expand_path(name)
        @files_created = []

        # Check if directory already exists and has content
        if Dir.exist?(project_dir) && !Dir.empty?(project_dir)
          raise Error.new(
            "Directory already exists and is not empty",
            code: "PROJECT.ALREADY_EXISTS",
            details: project_dir,
            suggestions: [
              "Choose a different name",
              "Delete the existing directory first"
            ]
          )
        end

        UI.step "Creating new GMR project: #{name}"

        # Create directory structure
        create_directories(project_dir)

        # Create files based on template
        create_main_rb(project_dir, name)
        create_project_config(project_dir, name)
        create_gitignore(project_dir)

        # Emit JSON result
        JsonEmitter.emit_success_envelope(
          command: "new",
          result: {
            project_name: name,
            project_path: project_dir,
            template: options[:template],
            files_created: @files_created
          }
        )

        UI.success "Project created: #{project_dir}"
        UI.blank

        show_next_steps(name)
      end

      private

      def create_directories(project_dir)
        dirs = [
          File.join(project_dir, "game", "scripts"),
          File.join(project_dir, "game", "assets", "sprites"),
          File.join(project_dir, "game", "assets", "sounds"),
          File.join(project_dir, "game", "assets", "fonts"),
          File.join(project_dir, "game", "assets", "music")
        ]

        dirs.each do |dir|
          FileUtils.mkdir_p(dir)
          UI.info "Created #{dir.sub(project_dir + '/', '')}"
        end
      end

      def create_main_rb(project_dir, name)
        main_rb = File.join(project_dir, "game", "scripts", "main.rb")

        content = case options[:template]
                  when "empty"
                    empty_template(name)
                  else
                    basic_template(name)
                  end

        File.write(main_rb, content)
        @files_created << "game/scripts/main.rb"
        UI.info "Created game/scripts/main.rb"
      end

      def empty_template(name)
        <<~RUBY
          # #{name}
          # Created with GMR (Games Made with Ruby)

          def init
            # Called once at startup
          end

          def update
            # Called every frame
          end

          def draw
            # Called every frame after update
          end
        RUBY
      end

      def basic_template(name)
        <<~RUBY
          # #{name}
          # Created with GMR (Games Made with Ruby)

          def init
            @x = 400
            @y = 300
            @speed = 200
            @size = 50
            @color = [255, 100, 100]
          end

          def update
            dt = Window.delta_time

            # Movement with arrow keys or WASD
            @x -= @speed * dt if Input.key_down?(:left) || Input.key_down?(:a)
            @x += @speed * dt if Input.key_down?(:right) || Input.key_down?(:d)
            @y -= @speed * dt if Input.key_down?(:up) || Input.key_down?(:w)
            @y += @speed * dt if Input.key_down?(:down) || Input.key_down?(:s)

            # Keep on screen
            @x = @x.clamp(@size / 2, 800 - @size / 2)
            @y = @y.clamp(@size / 2, 600 - @size / 2)

            # Change color with space
            if Input.key_pressed?(:space)
              @color = [rand(100..255), rand(100..255), rand(100..255)]
            end

            # Exit with escape
            Window.close if Input.key_pressed?(:escape)
          end

          def draw
            Graphics.clear(30, 30, 40)

            # Draw player
            half = @size / 2
            Graphics.fill_rect(@x - half, @y - half, @size, @size, *@color)

            # Draw instructions
            Graphics.draw_text("Arrow keys or WASD to move", 10, 10, 20, 200, 200, 200)
            Graphics.draw_text("Space to change color", 10, 35, 20, 200, 200, 200)
            Graphics.draw_text("Escape to exit", 10, 60, 20, 200, 200, 200)

            # Show FPS
            Graphics.draw_text("FPS: \#{Window.fps}", 10, 560, 20, 100, 100, 100)
          end
        RUBY
      end

      def create_project_config(project_dir, name)
        config = {
          name: name,
          version: "0.1.0",
          window: {
            title: name,
            width: 800,
            height: 600
          }
        }

        config_path = File.join(project_dir, "gmr.json")
        File.write(config_path, JSON.pretty_generate(config))
        @files_created << "gmr.json"
        UI.info "Created gmr.json"
      end

      def create_gitignore(project_dir)
        content = <<~GITIGNORE
          # Build artifacts
          /build/
          /release/
          *.exe

          # Editor files
          .vscode/
          *.swp
          *~

          # OS files
          .DS_Store
          Thumbs.db
        GITIGNORE

        File.write(File.join(project_dir, ".gitignore"), content)
        @files_created << ".gitignore"
        UI.info "Created .gitignore"
      end

      def show_next_steps(name)
        UI.next_steps([
          "cd #{name}",
          "gmrcli run                    # Run your game",
          "Edit game/scripts/main.rb     # Start coding!"
        ])
      end
    end
  end
end
