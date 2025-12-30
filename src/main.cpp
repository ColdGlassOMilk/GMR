#include "gmr/engine.hpp"
#include "raylib.h"
#include <cstdio>

int main() {
    auto& state = gmr::State::instance();
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(state.screen_width, state.screen_height, "GMR");
    InitAudioDevice();
    SetTargetFPS(60);
    SetExitKey(0);
    
    auto& loader = gmr::scripting::Loader::instance();
    loader.load("scripts");
    
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
            // Console update - returns true if console is consuming input
            mrb_value console_blocking = mrb_false_value();
            mrb_sym console_update_sym = mrb_intern_cstr(mrb, "console_update");
            if (mrb_respond_to(mrb, mrb_top_self(mrb), console_update_sym)) {
                console_blocking = mrb_funcall(mrb, mrb_top_self(mrb), "console_update", 1, 
                                               mrb_float_value(mrb, dt));
                if (mrb->exc) {
                    mrb_print_error(mrb);
                    mrb->exc = nullptr;
                    console_blocking = mrb_false_value();
                }
            }
            
            // Only update game if console is not blocking input
            if (!mrb_test(console_blocking)) {
                gmr::scripting::safe_call(mrb, "update", mrb_float_value(mrb, dt));
            }
            
            if (state.use_virtual_resolution) {
                BeginTextureMode(gmr::bindings::get_render_target());
                gmr::scripting::safe_call(mrb, "draw");
                
                // Draw console on render target
                mrb_sym console_draw_sym = mrb_intern_cstr(mrb, "console_draw");
                if (mrb_respond_to(mrb, mrb_top_self(mrb), console_draw_sym)) {
                    mrb_funcall(mrb, mrb_top_self(mrb), "console_draw", 0);
                    if (mrb->exc) {
                        mrb_print_error(mrb);
                        mrb->exc = nullptr;
                    }
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
                
                // Draw console on top
                mrb_sym console_draw_sym = mrb_intern_cstr(mrb, "console_draw");
                if (mrb_respond_to(mrb, mrb_top_self(mrb), console_draw_sym)) {
                    mrb_funcall(mrb, mrb_top_self(mrb), "console_draw", 0);
                    if (mrb->exc) {
                        mrb_print_error(mrb);
                        mrb->exc = nullptr;
                    }
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
    
    // Cleanup
    gmr::bindings::cleanup_window();
    gmr::SoundManager::instance().clear();
    gmr::TextureManager::instance().clear();
    
    CloseAudioDevice();
    CloseWindow();
    
    return 0;
}