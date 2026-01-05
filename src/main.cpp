#include "gmr/engine.hpp"
#include "gmr/camera.hpp"
#include "gmr/draw_queue.hpp"
#include "gmr/sprite.hpp"
#include "gmr/transform.hpp"
#include "raylib.h"
#include <cstdio>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

// Global state for the main loop (needed for Emscripten callback)
struct GameContext {
    double last_time;
};

static GameContext g_ctx;

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
    
    // Update window size tracking - always check on web since browser can resize canvas
    gmr::bindings::update_web_screen_size();

    // Note: Hot reload is disabled for web builds (no filesystem write access)
    
    if (auto* mrb = loader.mrb()) {
        // Reset draw queue for new frame
        gmr::DrawQueue::instance().begin_frame();

        // Update camera system (before Ruby update)
        gmr::CameraManager::instance().update(mrb, static_cast<float>(dt));

        // Console update (handles its own input)
        mrb_sym console_update_sym = mrb_intern_cstr(mrb, "console_update");
        if (mrb_respond_to(mrb, mrb_top_self(mrb), console_update_sym)) {
            mrb_funcall(mrb, mrb_top_self(mrb), "console_update", 1,
                        mrb_float_value(mrb, dt));
            gmr::scripting::check_error(mrb, "console_update");
        }

        // Update game
        gmr::scripting::safe_call(mrb, "update", mrb_float_value(mrb, dt));

        if (state.use_virtual_resolution) {
            BeginTextureMode(gmr::bindings::get_render_target());
            gmr::scripting::safe_call(mrb, "draw");
            // Flush queued sprite draws (z-sorted)
            gmr::DrawQueue::instance().flush();

            // Draw console on render target
            mrb_sym console_draw_sym = mrb_intern_cstr(mrb, "console_draw");
            if (mrb_respond_to(mrb, mrb_top_self(mrb), console_draw_sym)) {
                mrb_funcall(mrb, mrb_top_self(mrb), "console_draw", 0);
                gmr::scripting::check_error(mrb, "console_draw");
            }
            
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

            // Draw console on top
            mrb_sym console_draw_sym = mrb_intern_cstr(mrb, "console_draw");
            if (mrb_respond_to(mrb, mrb_top_self(mrb), console_draw_sym)) {
                mrb_funcall(mrb, mrb_top_self(mrb), "console_draw", 0);
                gmr::scripting::check_error(mrb, "console_draw");
            }

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

int main() {
    auto& state = gmr::State::instance();
    
#ifdef PLATFORM_WEB
    // Web-specific initialization
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(state.screen_width, state.screen_height, "GMR");
    InitAudioDevice();
    
    auto& loader = gmr::scripting::Loader::instance();
    loader.load("game/scripts");

    g_ctx.last_time = GetTime();
    
    // Use emscripten_set_main_loop for web
    // 0 = use requestAnimationFrame (recommended)
    // 1 = simulate infinite loop (required)
    emscripten_set_main_loop_arg(game_loop, nullptr, 0, 1);
    
#else
    // Native platform initialization
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(state.screen_width, state.screen_height, "GMR");
    InitAudioDevice();
    SetTargetFPS(60);
    SetExitKey(0);
    
    auto& loader = gmr::scripting::Loader::instance();
    loader.load("game/scripts");

    double last_time = GetTime();
    
    while (!WindowShouldClose()) {
        double current_time = GetTime();
        double dt = current_time - last_time;
        last_time = current_time;
        
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
        
        if (auto* mrb = loader.mrb()) {
            // Reset draw queue for new frame
            gmr::DrawQueue::instance().begin_frame();

            // Update camera system (before Ruby update)
            gmr::CameraManager::instance().update(mrb, static_cast<float>(dt));

            // Console update (handles its own input)
            mrb_sym console_update_sym = mrb_intern_cstr(mrb, "console_update");
            if (mrb_respond_to(mrb, mrb_top_self(mrb), console_update_sym)) {
                mrb_funcall(mrb, mrb_top_self(mrb), "console_update", 1,
                            mrb_float_value(mrb, dt));
                gmr::scripting::check_error(mrb, "console_update");
            }

            // Update game
            gmr::scripting::safe_call(mrb, "update", mrb_float_value(mrb, dt));

            if (state.use_virtual_resolution) {
                BeginTextureMode(gmr::bindings::get_render_target());
                gmr::scripting::safe_call(mrb, "draw");
                // Flush queued sprite draws (z-sorted)
                gmr::DrawQueue::instance().flush();

                // Draw console on render target
                mrb_sym console_draw_sym = mrb_intern_cstr(mrb, "console_draw");
                if (mrb_respond_to(mrb, mrb_top_self(mrb), console_draw_sym)) {
                    mrb_funcall(mrb, mrb_top_self(mrb), "console_draw", 0);
                    gmr::scripting::check_error(mrb, "console_draw");
                }

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

                // Draw console on top
                mrb_sym console_draw_sym = mrb_intern_cstr(mrb, "console_draw");
                if (mrb_respond_to(mrb, mrb_top_self(mrb), console_draw_sym)) {
                    mrb_funcall(mrb, mrb_top_self(mrb), "console_draw", 0);
                    gmr::scripting::check_error(mrb, "console_draw");
                }

                EndDrawing();
            }
        } else {
            BeginDrawing();
            ClearBackground(::Color{0, 0, 0, 255});
            DrawText("Script error - check console", 10, 10, 20, ::Color{255, 0, 0, 255});
            EndDrawing();
        }
    }

    // Cleanup (only for native - web doesn't reach here)
    gmr::bindings::cleanup_window();
    gmr::SpriteManager::instance().clear();
    gmr::TransformManager::instance().clear();
    gmr::DrawQueue::instance().clear();
    gmr::SoundManager::instance().clear();
    gmr::TextureManager::instance().clear();
    
    CloseAudioDevice();
    CloseWindow();
#endif
    
    return 0;
}