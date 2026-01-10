#include "gmr/engine.hpp"
#include "gmr/camera.hpp"
#include "gmr/draw_queue.hpp"
#include "gmr/sprite.hpp"
#include "gmr/transform.hpp"
#include "gmr/animation/animation_manager.hpp"
#include "gmr/state_machine/state_machine_manager.hpp"
#include "gmr/input/input_manager.hpp"
#include "gmr/event/event_queue.hpp"
#include "gmr/console/console_module.hpp"
#include "gmr/filesystem/paths.hpp"
#include "gmr/resources/music_manager.hpp"
#include "raylib.h"
#include <cstdio>
#include <cstring>
#include <filesystem>

#if defined(GMR_DEBUG_ENABLED)
#include "gmr/debug/debug_server.hpp"
#endif

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

// Global state for the main loop (needed for Emscripten callback).
// NOTE: This is a platform-specific workaround for Emscripten's emscripten_set_main_loop.
// The Emscripten callback cannot capture local state, so we must use a global.
// This global is NOT reset on hot reload and does not hold Ruby references.
// See CONTRIBUTING.md section on known issues regarding global state.
struct GameContext {
    double last_time;
};

static GameContext g_ctx;

// Global flag for script-initiated shutdown
// Allows GMR::System.quit to trigger clean shutdown through main loop exit
bool g_should_quit = false;

// Global delta time for consistent timing across C++ and Ruby
// This ensures Time.delta in Ruby matches the actual frame delta used by C++
// Note: Not static because it's accessed from window.cpp
float g_frame_delta = 0.0f;

// Forward declaration
void game_loop(void* arg);

#ifdef PLATFORM_WEB
// Emscripten main loop callback
void game_loop(void* arg) {
    (void)arg;  // Unused in callback version
    
    auto& state = gmr::State::instance();
    auto& loader = gmr::scripting::Loader::instance();
    
    double current_time = GetTime();
    double dt = current_time - g_ctx.last_time;
    g_ctx.last_time = current_time;

    // Cap delta time to prevent lag spikes (max 20fps minimum frame rate)
    // This only triggers during severe lag - normal frame variance is fine
    const double MAX_DELTA = 0.05;  // ~20 fps minimum
    if (dt > MAX_DELTA) dt = MAX_DELTA;

    // Store for Ruby bindings
    g_frame_delta = static_cast<float>(dt);
    
    // Update window size tracking - always check on web since browser can resize canvas
    gmr::bindings::update_web_screen_size();

    // Note: Hot reload is disabled for web builds (no filesystem write access)
    
    if (auto* mrb = loader.mrb()) {
        // Reset draw queue for new frame
        gmr::DrawQueue::instance().begin_frame();

        // Poll input and enqueue events
        gmr::input::InputManager::instance().poll_and_dispatch(mrb);

        // Dispatch queued events to all subscribers
        gmr::event::EventQueue::instance().dispatch(mrb);

        // Update state machine system (receives input events via subscription)
        gmr::state_machine::StateMachineManager::instance().update(mrb, static_cast<float>(dt));

        // Update music streaming (must be called every frame)
        gmr::MusicManager::instance().update();

        // Built-in console update (handles its own input)
        auto& console = gmr::console::ConsoleModule::instance();
        bool console_consumed = console.update(static_cast<float>(dt));

        // Update game (skip if console consumed input)
        if (!console_consumed || !console.is_open()) {
            gmr::scripting::safe_call(mrb, "update", mrb_float_value(mrb, dt));
        }

        // Update camera system (after Ruby update to apply bounds immediately)
        gmr::CameraManager::instance().update(mrb, static_cast<float>(dt));

        // Update animation system AFTER game logic (tweens and sprite animations)
        // This ensures animations triggered during update() are advanced immediately
        gmr::animation::AnimationManager::instance().update(mrb, static_cast<float>(dt));

        if (state.use_virtual_resolution) {
            BeginTextureMode(gmr::bindings::get_render_target());
            gmr::scripting::safe_call(mrb, "draw");
            // Flush queued sprite draws (z-sorted)
            gmr::DrawQueue::instance().flush();

            // Draw built-in console on render target
            console.draw();

            EndTextureMode();

            BeginDrawing();
            ClearBackground(::Color{0, 0, 0, 255});

            float scale_x = static_cast<float>(GetScreenWidth()) / state.virtual_width;
            float scale_y = static_cast<float>(GetScreenHeight()) / state.virtual_height;
            float scale = (scale_x < scale_y) ? scale_x : scale_y;

            int scaled_width = static_cast<int>(state.virtual_width * scale);
            int scaled_height = static_cast<int>(state.virtual_height * scale);
            int offset_x = (GetScreenWidth() - scaled_width) / 2;
            int offset_y = (GetScreenHeight() - scaled_height) / 2;

            Rectangle source = {0, 0, static_cast<float>(state.virtual_width),
                               -static_cast<float>(state.virtual_height)};
            Rectangle dest = {static_cast<float>(offset_x), static_cast<float>(offset_y),
                              static_cast<float>(scaled_width), static_cast<float>(scaled_height)};

            DrawTexturePro(gmr::bindings::get_render_target().texture, source, dest,
                          Vector2{0, 0}, 0.0f, ::Color{255, 255, 255, 255});
            EndDrawing();
        } else {
            BeginDrawing();
            gmr::scripting::safe_call(mrb, "draw");
            // Flush queued sprite draws (z-sorted)
            gmr::DrawQueue::instance().flush();

            // Draw built-in console on top
            console.draw();

            EndDrawing();
        }
    } else {
        BeginDrawing();
        ClearBackground(::Color{0, 0, 0, 255});
        DrawText("Script error - check console", 10, 10, 20, ::Color{255, 0, 0, 255});
        EndDrawing();
    }
}
#endif

int main(int argc, char* argv[]) {
    auto& state = gmr::State::instance();

    // Parse command-line arguments
    bool window_topmost = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--topmost") == 0) {
            window_topmost = true;
        }
    }

#ifdef PLATFORM_WEB
    (void)window_topmost; // Unused on web
    // Web-specific initialization
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(state.screen_width, state.screen_height, "GMR");
    InitAudioDevice();

    // Initialize Emscripten FS for writable /data directory backed by IndexedDB
    // Note: /assets is preloaded as read-only, so we use a separate /data directory
    EM_ASM({
        try {
            // Create writable /data directory (separate from preloaded /assets)
            try {
                FS.mkdir('/data');
            } catch (e) {
                if (e.code !== 'EEXIST') {
                    console.error('Failed to create /data:', e);
                    return;
                }
            }

            // Mount IDBFS for persistence
            FS.mount(IDBFS, {}, '/data');

            // Load existing data from IndexedDB
            FS.syncfs(true, function(err) {
                if (err) {
                    console.error('IDBFS sync failed:', err);
                    console.warn('Save data will not persist between sessions');
                } else {
                    console.log('IDBFS initialized');
                }
            });
        } catch (e) {
            console.error('IDBFS init failed:', e);
            console.warn('Storage persistence disabled');
        }
    });

    auto& loader = gmr::scripting::Loader::instance();
    loader.load("game/scripts");

    g_ctx.last_time = GetTime();
    
    // Use emscripten_set_main_loop for web
    // 0 = use requestAnimationFrame (recommended)
    // 1 = simulate infinite loop (required)
    emscripten_set_main_loop_arg(game_loop, nullptr, 0, 1);
    
#else
    // Native platform initialization
    unsigned int config_flags = FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT;
    if (window_topmost) {
        config_flags |= FLAG_WINDOW_TOPMOST;
    }
    SetConfigFlags(config_flags);
    InitWindow(state.screen_width, state.screen_height, "GMR");
    InitAudioDevice();
    SetTargetFPS(60);
    SetExitKey(0);

    // Ensure game/data directory exists for writable storage
    // This matches the path resolution in filesystem/paths.cpp
    try {
        std::filesystem::create_directories("game/data");
    } catch (...) {
        fprintf(stderr, "Warning: Failed to create game/data directory\n");
    }

    auto& loader = gmr::scripting::Loader::instance();
    loader.load("game/scripts");

#if defined(GMR_DEBUG_ENABLED)
    // Start the Ruby debug server
    if (gmr::debug::DebugServer::instance().start(5678)) {
        printf("Ruby debugger listening on port 5678\n");
    }
#endif

    double last_time = GetTime();

    while (!WindowShouldClose() && !g_should_quit) {
        double current_time = GetTime();
        double dt = current_time - last_time;
        last_time = current_time;

        // Cap delta time to prevent lag spikes (max 20fps minimum frame rate)
        // This only triggers during severe lag - normal frame variance is fine
        const double MAX_DELTA = 0.05;  // ~20 fps minimum
        if (dt > MAX_DELTA) dt = MAX_DELTA;

        // Store for Ruby bindings
        g_frame_delta = static_cast<float>(dt);
        
        // Update window size tracking
        if (IsWindowResized() && !state.is_fullscreen) {
            state.windowed_width = GetScreenWidth();
            state.windowed_height = GetScreenHeight();
            if (!state.use_virtual_resolution) {
                state.screen_width = state.windowed_width;
                state.screen_height = state.windowed_height;
            }
        }
        
        // Hot reload
        loader.reload_if_changed();

#if defined(GMR_DEBUG_ENABLED)
        // Poll debug server for commands
        gmr::debug::DebugServer::instance().poll();
#endif

        if (auto* mrb = loader.mrb()) {
            // Reset draw queue for new frame
            gmr::DrawQueue::instance().begin_frame();

            // Poll input and enqueue events
            gmr::input::InputManager::instance().poll_and_dispatch(mrb);

            // Dispatch queued events to all subscribers
            gmr::event::EventQueue::instance().dispatch(mrb);

            // Update state machine system (receives input events via subscription)
            gmr::state_machine::StateMachineManager::instance().update(mrb, static_cast<float>(dt));

            // Update music streaming (must be called every frame)
            gmr::MusicManager::instance().update();

            // Built-in console update (handles its own input)
            auto& console = gmr::console::ConsoleModule::instance();
            bool console_consumed = console.update(static_cast<float>(dt));

            // Update game (skip if console consumed input)
            if (!console_consumed || !console.is_open()) {
                gmr::scripting::safe_call(mrb, "update", mrb_float_value(mrb, dt));
            }

            // Update camera system (after Ruby update to apply bounds immediately)
            gmr::CameraManager::instance().update(mrb, static_cast<float>(dt));

            // Update animation system AFTER game logic (tweens and sprite animations)
            // This ensures animations triggered during update() are advanced immediately
            gmr::animation::AnimationManager::instance().update(mrb, static_cast<float>(dt));

            if (state.use_virtual_resolution) {
                BeginTextureMode(gmr::bindings::get_render_target());
                gmr::scripting::safe_call(mrb, "draw");
                // Flush queued sprite draws (z-sorted)
                gmr::DrawQueue::instance().flush();

                // Draw built-in console on render target
                console.draw();

                EndTextureMode();

                BeginDrawing();
                ClearBackground(::Color{0, 0, 0, 255});

                float scale_x = static_cast<float>(GetScreenWidth()) / state.virtual_width;
                float scale_y = static_cast<float>(GetScreenHeight()) / state.virtual_height;
                float scale = (scale_x < scale_y) ? scale_x : scale_y;

                int scaled_width = static_cast<int>(state.virtual_width * scale);
                int scaled_height = static_cast<int>(state.virtual_height * scale);
                int offset_x = (GetScreenWidth() - scaled_width) / 2;
                int offset_y = (GetScreenHeight() - scaled_height) / 2;

                Rectangle source = {0, 0, static_cast<float>(state.virtual_width),
                                   -static_cast<float>(state.virtual_height)};
                Rectangle dest = {static_cast<float>(offset_x), static_cast<float>(offset_y),
                                  static_cast<float>(scaled_width), static_cast<float>(scaled_height)};

                DrawTexturePro(gmr::bindings::get_render_target().texture, source, dest,
                              Vector2{0, 0}, 0.0f, ::Color{255, 255, 255, 255});
                EndDrawing();
            } else {
                BeginDrawing();
                gmr::scripting::safe_call(mrb, "draw");
                // Flush queued sprite draws (z-sorted)
                gmr::DrawQueue::instance().flush();

                // Draw built-in console on top
                console.draw();

                EndDrawing();
            }
        } else {
            BeginDrawing();
            ClearBackground(::Color{0, 0, 0, 255});
            DrawText("Script error - check console", 10, 10, 20, ::Color{255, 0, 0, 255});
            EndDrawing();
        }
    }

#if defined(GMR_DEBUG_ENABLED)
    // Stop the debug server
    gmr::debug::DebugServer::instance().stop();
#endif

    // Shutdown console module
    gmr::console::ConsoleModule::instance().shutdown();

    // Cleanup (only for native - web doesn't reach here)
    // Clear managers with Ruby references while mruby is still available
    // Order matches hot reload in loader.cpp - managers cleared before event queue
    if (auto* mrb = loader.mrb()) {
        gmr::CameraManager::instance().clear(mrb);
        gmr::animation::AnimationManager::instance().clear(mrb);
        gmr::state_machine::StateMachineManager::instance().clear(mrb);
        gmr::input::InputManager::instance().clear(mrb);
        gmr::event::EventQueue::instance().clear(mrb);
    }
    gmr::bindings::cleanup_window();
    gmr::SpriteManager::instance().clear();
    gmr::TransformManager::instance().clear();
    gmr::DrawQueue::instance().clear();
    gmr::SoundManager::instance().clear();
    gmr::MusicManager::instance().clear();
    gmr::TextureManager::instance().clear();

    CloseAudioDevice();
    CloseWindow();
#endif
    
    return 0;
}