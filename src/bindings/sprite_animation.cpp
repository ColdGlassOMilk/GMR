#include "gmr/bindings/sprite_animation.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/bindings/sprite.hpp"
#include "gmr/animation/animation_manager.hpp"
#include "gmr/scripting/helpers.hpp"
#include "gmr/sprite.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/array.h>
#include <mruby/variable.h>
#include <cstring>

namespace gmr {
namespace bindings {

/// @class GMR::SpriteAnimation
/// @description Frame-based animation for sprites. Automatically updates sprite's
///   source_rect each frame based on the frame indices and grid layout.
/// @example # Platformer character with multiple animations managed by state machine
///   class Player
///     def initialize
///       @texture = GMR::Graphics::Texture.load("assets/player_sheet.png")
///       @sprite = Sprite.new(@texture)
///       @sprite.source_rect = Rect.new(0, 0, 32, 32)
///
///       # Define all animations (8 columns in spritesheet)
///       @animations = {
///         idle: GMR::SpriteAnimation.new(@sprite, frames: 0..3, fps: 6, columns: 8),
///         run: GMR::SpriteAnimation.new(@sprite, frames: 8..13, fps: 12, columns: 8),
///         jump: GMR::SpriteAnimation.new(@sprite, frames: 16..18, fps: 8, loop: false, columns: 8),
///         fall: GMR::SpriteAnimation.new(@sprite, frames: 19..21, fps: 8, columns: 8),
///         attack: GMR::SpriteAnimation.new(@sprite, frames: 24..29, fps: 15, loop: false, columns: 8)
///       }
///       @current_anim = nil
///       play_animation(:idle)
///     end
///
///     def play_animation(name)
///       return if @current_anim == @animations[name]
///       @current_anim&.stop
///       @current_anim = @animations[name]
///       @current_anim.play
///     end
///   end
/// @example # Enemy with death animation that triggers cleanup
///   class Enemy
///     def initialize(x, y)
///       @sprite = Sprite.new(GMR::Graphics::Texture.load("assets/enemy.png"))
///       @sprite.x = x
///       @sprite.y = y
///       @health = 50
///       @alive = true
///
///       @walk_anim = GMR::SpriteAnimation.new(@sprite, frames: 0..5, fps: 10, columns: 6)
///       @death_anim = GMR::SpriteAnimation.new(@sprite, frames: 6..11, fps: 12, loop: false, columns: 6)
///       @death_anim.on_complete { @sprite.visible = false; @can_remove = true }
///
///       @walk_anim.play
///     end
///
///     def take_damage(amount)
///       @health -= amount
///       if @health <= 0 && @alive
///         @alive = false
///         @walk_anim.stop
///         @death_anim.play
///         GMR::Audio::Sound.play("assets/enemy_death.wav")
///       end
///     end
///   end
/// @example # Animated UI element with frame-based events
///   class TreasureChest
///     def initialize
///       @sprite = Sprite.new(GMR::Graphics::Texture.load("assets/chest.png"))
///       @open_anim = GMR::SpriteAnimation.new(@sprite, frames: 0..4, fps: 8, loop: false, columns: 5)
///       @open_anim.on_frame_change do |frame|
///         # Play sound on specific frames
///         GMR::Audio::Sound.play("assets/creak.wav") if frame == 2
///         spawn_particles if frame == 4
///       end
///       @open_anim.on_complete { @ready_to_loot = true }
///     end
///
///     def open
///       @open_anim.play unless @opened
///       @opened = true
///     end
///   end

// ============================================================================
// SpriteAnimation Binding Data
// ============================================================================

struct SpriteAnimationData {
    SpriteAnimationHandle handle;
};

static void sprite_animation_free(mrb_state* mrb, void* ptr) {
    SpriteAnimationData* data = static_cast<SpriteAnimationData*>(ptr);
    if (data) {
        // LIFECYCLE: Ruby wrapper owns the animation handle.
        // When Ruby GC collects this object, we must clean up the native animation.
        animation::AnimationManager::instance().destroy_animation(data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type sprite_animation_data_type = {
    "SpriteAnimation", sprite_animation_free
};

static SpriteAnimationData* get_animation_data(mrb_state* mrb, mrb_value self) {
    return static_cast<SpriteAnimationData*>(mrb_data_get_ptr(mrb, self, &sprite_animation_data_type));
}

// ============================================================================
// Helper: Get SpriteHandle from Ruby Sprite object
// ============================================================================

// Forward declare sprite data type from sprite.cpp for type-safe access
extern const mrb_data_type sprite_data_type;

struct SpriteBindingData {
    SpriteHandle handle;
};

static SpriteHandle get_sprite_handle(mrb_state* mrb, mrb_value sprite_val) {
    // TYPE-SAFE: Use &sprite_data_type instead of nullptr to verify type
    void* ptr = mrb_data_get_ptr(mrb, sprite_val, &sprite_data_type);
    if (ptr) {
        SpriteBindingData* data = static_cast<SpriteBindingData*>(ptr);
        return data->handle;
    }
    return INVALID_HANDLE;
}

// ============================================================================
// GMR::SpriteAnimation.new(sprite, frames:, fps:, loop:, ...)
// ============================================================================

/// @method initialize
/// @description Create a new sprite animation.
/// @param sprite [Sprite] The sprite to animate
/// @param frames: [Array<Integer>, Range] Frame indices to cycle through
/// @param fps: [Float] Frames per second (default: 12)
/// @param loop: [Boolean] Whether to loop the animation (default: true)
/// @param frame_width: [Integer] Width of each frame (default: inferred from source_rect)
/// @param frame_height: [Integer] Height of each frame (default: inferred from source_rect)
/// @param columns: [Integer] Number of columns in spritesheet (default: 1)
/// @example # Basic spritesheet setup (4x4 grid, 64x64 frames)
///   @sprite = Sprite.new(GMR::Graphics::Texture.load("assets/hero.png"))
///   @sprite.source_rect = Rect.new(0, 0, 64, 64)  # First frame
///
///   # Row 0: idle (frames 0-3), Row 1: walk (frames 4-7)
///   @idle_anim = GMR::SpriteAnimation.new(@sprite,
///     frames: 0..3,
///     fps: 8,
///     loop: true,
///     frame_width: 64,
///     frame_height: 64,
///     columns: 4
///   )
/// @example # Attack combo with non-looping animation
///   @attack_anim = GMR::SpriteAnimation.new(@sprite,
///     frames: [8, 9, 10, 11, 12],  # Explicit frame list
///     fps: 18,  # Fast attack animation
///     loop: false
///   )
///   @attack_anim.on_complete { @can_attack_again = true }
static mrb_value mrb_sprite_animation_initialize(mrb_state* mrb, mrb_value self) {
    mrb_value sprite_val;
    mrb_value kwargs;

    mrb_get_args(mrb, "oH", &sprite_val, &kwargs);

    // Get sprite handle
    SpriteHandle sprite_handle = get_sprite_handle(mrb, sprite_val);
    if (sprite_handle == INVALID_HANDLE) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected Sprite object");
        return mrb_nil_value();
    }

    auto& manager = animation::AnimationManager::instance();

    // Create animation
    SpriteAnimationHandle handle = manager.create_animation();
    animation::SpriteAnimationState* anim = manager.get_animation(handle);

    anim->sprite = sprite_handle;
    anim->sprite_ref = sprite_val;

    // Parse frames (Array or Range)
    mrb_value frames_val = mrb_hash_get(mrb, kwargs,
        mrb_symbol_value(mrb_intern_lit(mrb, "frames")));

    if (mrb_array_p(frames_val)) {
        mrb_int len = RARRAY_LEN(frames_val);
        for (mrb_int i = 0; i < len; i++) {
            mrb_value frame = mrb_ary_ref(mrb, frames_val, i);
            anim->frames.push_back(static_cast<int>(mrb_fixnum(frame)));
        }
    } else if (mrb_range_p(frames_val)) {
        // Convert Range to array (protected)
        mrb_value arr = scripting::safe_method_call(mrb, frames_val, "to_a");
        if (mrb_array_p(arr)) {
            mrb_int len = RARRAY_LEN(arr);
            for (mrb_int i = 0; i < len; i++) {
                mrb_value frame = mrb_ary_ref(mrb, arr, i);
                anim->frames.push_back(static_cast<int>(mrb_fixnum(frame)));
            }
        }
    } else if (!mrb_nil_p(frames_val)) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "frames: must be an Array or Range");
        return mrb_nil_value();
    }

    // Parse fps (default: 12)
    mrb_value fps_val = mrb_hash_get(mrb, kwargs,
        mrb_symbol_value(mrb_intern_lit(mrb, "fps")));
    anim->fps = mrb_nil_p(fps_val) ? 12.0f : static_cast<float>(mrb_as_float(mrb, fps_val));
    anim->update_frame_duration();

    // Parse loop (default: true)
    mrb_value loop_val = mrb_hash_get(mrb, kwargs,
        mrb_symbol_value(mrb_intern_lit(mrb, "loop")));
    anim->loop = mrb_nil_p(loop_val) ? true : mrb_test(loop_val);

    // Get sprite to infer frame dimensions if not provided
    SpriteState* sprite = SpriteManager::instance().get(sprite_handle);

    // Parse frame_width (default: from source_rect or 0)
    mrb_value frame_width_val = mrb_hash_get(mrb, kwargs,
        mrb_symbol_value(mrb_intern_lit(mrb, "frame_width")));
    if (!mrb_nil_p(frame_width_val)) {
        anim->frame_width = static_cast<int>(mrb_fixnum(frame_width_val));
    } else if (sprite && sprite->use_source_rect) {
        anim->frame_width = static_cast<int>(sprite->source_rect.width);
    }

    // Parse frame_height (default: from source_rect or 0)
    mrb_value frame_height_val = mrb_hash_get(mrb, kwargs,
        mrb_symbol_value(mrb_intern_lit(mrb, "frame_height")));
    if (!mrb_nil_p(frame_height_val)) {
        anim->frame_height = static_cast<int>(mrb_fixnum(frame_height_val));
    } else if (sprite && sprite->use_source_rect) {
        anim->frame_height = static_cast<int>(sprite->source_rect.height);
    }

    // Parse columns (default: 1)
    mrb_value columns_val = mrb_hash_get(mrb, kwargs,
        mrb_symbol_value(mrb_intern_lit(mrb, "columns")));
    anim->columns = mrb_nil_p(columns_val) ? 1 : static_cast<int>(mrb_fixnum(columns_val));

    // Attach to Ruby object
    SpriteAnimationData* data = static_cast<SpriteAnimationData*>(
        mrb_malloc(mrb, sizeof(SpriteAnimationData)));
    data->handle = handle;
    mrb_data_init(self, data, &sprite_animation_data_type);

    // Store sprite reference to prevent GC
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@sprite"), sprite_val);

    // Register with GC
    anim->ruby_anim_obj = self;
    mrb_gc_register(mrb, self);

    return self;
}

// ============================================================================
// Instance Methods
// ============================================================================

/// @method play
/// @description Start or resume the animation.
/// @returns [SpriteAnimation] self for chaining
/// @example # State machine integration - play animation when entering run state
///   state :run do
///     enter { @animations[:run].play }
///     exit { @animations[:run].stop }
///     on :stop, :idle
///     on :jump, :jumping
///   end
static mrb_value mrb_sprite_animation_play(mrb_state* mrb, mrb_value self) {
    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return self;

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return self;

    anim->playing = true;
    anim->completed = false;

    return self;
}

/// @method pause
/// @description Pause the animation at the current frame.
/// @returns [SpriteAnimation] self for chaining
/// @example anim.pause
static mrb_value mrb_sprite_animation_pause(mrb_state* mrb, mrb_value self) {
    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return self;

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return self;

    anim->playing = false;

    return self;
}

/// @method stop
/// @description Stop the animation and reset to the first frame.
/// @returns [SpriteAnimation] self for chaining
/// @example anim.stop
static mrb_value mrb_sprite_animation_stop(mrb_state* mrb, mrb_value self) {
    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return self;

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return self;

    anim->playing = false;
    anim->current_frame_index = 0;
    anim->elapsed = 0.0f;
    anim->completed = false;

    return self;
}

/// @method on_complete
/// @description Set a callback for when the animation finishes (non-looping only).
/// @returns [SpriteAnimation] self for chaining
/// @example # Chain attack animation into recovery state
///   def start_attack
///     @attack_anim.stop
///     @attack_anim.play
///       .on_complete do
///         @state_machine.trigger(:attack_finished)
///         @can_be_hit = true
///       end
///     @can_be_hit = false  # Invincible during attack
///   end
static mrb_value mrb_sprite_animation_on_complete(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return self;

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return self;

    anim->on_complete = block;
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@on_complete"), block);

    return self;
}

/// @method on_frame_change
/// @description Set a callback for each frame change. Receives frame index.
/// @returns [SpriteAnimation] self for chaining
/// @example # Play footstep sounds on specific walk cycle frames
///   @walk_anim.on_frame_change do |frame|
///     if frame == 2 || frame == 6  # Foot contact frames
///       GMR::Audio::Sound.play("assets/footstep.wav", volume: 0.3)
///     end
///   end
/// @example # Spawn attack hitbox on specific frame
///   @attack_anim.on_frame_change do |frame|
///     if frame == 3  # Sword swing frame
///       spawn_attack_hitbox
///       GMR::Audio::Sound.play("assets/whoosh.wav")
///     elsif frame == 5  # End of swing
///       destroy_attack_hitbox
///     end
///   end
static mrb_value mrb_sprite_animation_on_frame_change(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return self;

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return self;

    anim->on_frame_change = block;
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@on_frame_change"), block);

    return self;
}

/// @method playing?
/// @description Check if the animation is currently playing.
/// @returns [Boolean] true if playing
static mrb_value mrb_sprite_animation_playing_p(mrb_state* mrb, mrb_value self) {
    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return mrb_false_value();

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return mrb_false_value();

    return mrb_bool_value(anim->playing);
}

/// @method complete?
/// @description Check if the animation has completed (non-looping only).
/// @returns [Boolean] true if completed
static mrb_value mrb_sprite_animation_complete_p(mrb_state* mrb, mrb_value self) {
    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return mrb_false_value();

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return mrb_false_value();

    return mrb_bool_value(anim->completed);
}

/// @method frame
/// @description Get the current frame index (from the frames array).
/// @returns [Integer] Current frame index
static mrb_value mrb_sprite_animation_frame(mrb_state* mrb, mrb_value self) {
    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return mrb_fixnum_value(0);

    return mrb_fixnum_value(anim->current_frame());
}

/// @method frame=
/// @description Set the current frame index directly.
/// @param index [Integer] Frame index (into frames array)
/// @returns [Integer] The frame index
static mrb_value mrb_sprite_animation_set_frame(mrb_state* mrb, mrb_value self) {
    mrb_int index;
    mrb_get_args(mrb, "i", &index);

    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return mrb_fixnum_value(index);

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return mrb_fixnum_value(index);

    // Clamp to valid range
    if (index < 0) index = 0;
    if (index >= static_cast<mrb_int>(anim->frames.size())) {
        index = static_cast<mrb_int>(anim->frames.size()) - 1;
    }

    anim->current_frame_index = static_cast<int>(index);
    anim->elapsed = 0.0f;

    return mrb_fixnum_value(index);
}

/// @method fps
/// @description Get the frames per second.
/// @returns [Float] FPS
static mrb_value mrb_sprite_animation_fps(mrb_state* mrb, mrb_value self) {
    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return mrb_float_value(mrb, 12.0);

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return mrb_float_value(mrb, 12.0);

    return mrb_float_value(mrb, anim->fps);
}

/// @method fps=
/// @description Set the frames per second.
/// @param value [Float] New FPS
/// @returns [Float] The FPS value
/// @example # Speed up animation when player is running fast
///   def update(dt)
///     speed = calculate_movement_speed
///     # Scale animation FPS with movement speed (8-16 fps range)
///     @run_anim.fps = 8 + (speed / @max_speed) * 8
///   end
static mrb_value mrb_sprite_animation_set_fps(mrb_state* mrb, mrb_value self) {
    mrb_float fps;
    mrb_get_args(mrb, "f", &fps);

    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return mrb_float_value(mrb, fps);

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return mrb_float_value(mrb, fps);

    anim->fps = static_cast<float>(fps);
    anim->update_frame_duration();

    return mrb_float_value(mrb, fps);
}

/// @method loop?
/// @description Check if the animation loops.
/// @returns [Boolean] true if looping
static mrb_value mrb_sprite_animation_loop_p(mrb_state* mrb, mrb_value self) {
    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return mrb_false_value();

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return mrb_false_value();

    return mrb_bool_value(anim->loop);
}

/// @method loop=
/// @description Set whether the animation loops.
/// @param value [Boolean] true to loop
/// @returns [Boolean] The loop value
static mrb_value mrb_sprite_animation_set_loop(mrb_state* mrb, mrb_value self) {
    mrb_bool loop;
    mrb_get_args(mrb, "b", &loop);

    SpriteAnimationData* data = get_animation_data(mrb, self);
    if (!data) return mrb_bool_value(loop);

    animation::SpriteAnimationState* anim =
        animation::AnimationManager::instance().get_animation(data->handle);
    if (!anim) return mrb_bool_value(loop);

    anim->loop = loop;

    return mrb_bool_value(loop);
}

// ============================================================================
// Class Methods
// ============================================================================

/// @method count
/// @description Get the number of active sprite animations.
/// @returns [Integer] Number of active animations
static mrb_value mrb_sprite_animation_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(animation::AnimationManager::instance().animation_count());
}

// ============================================================================
// Registration
// ============================================================================

void register_sprite_animation(mrb_state* mrb) {
    // SpriteAnimation class under GMR::Animation
    RClass* gmr = get_gmr_module(mrb);
    RClass* animation = mrb_module_get_under(mrb, gmr, "Animation");
    RClass* anim_class = mrb_define_class_under(mrb, animation, "SpriteAnimation", mrb->object_class);
    MRB_SET_INSTANCE_TT(anim_class, MRB_TT_CDATA);

    // Constructor
    mrb_define_method(mrb, anim_class, "initialize", mrb_sprite_animation_initialize,
        MRB_ARGS_REQ(2));

    // Control
    mrb_define_method(mrb, anim_class, "play", mrb_sprite_animation_play, MRB_ARGS_NONE());
    mrb_define_method(mrb, anim_class, "pause", mrb_sprite_animation_pause, MRB_ARGS_NONE());
    mrb_define_method(mrb, anim_class, "stop", mrb_sprite_animation_stop, MRB_ARGS_NONE());

    // Callbacks
    mrb_define_method(mrb, anim_class, "on_complete", mrb_sprite_animation_on_complete,
        MRB_ARGS_BLOCK());
    mrb_define_method(mrb, anim_class, "on_frame_change", mrb_sprite_animation_on_frame_change,
        MRB_ARGS_BLOCK());

    // State
    mrb_define_method(mrb, anim_class, "playing?", mrb_sprite_animation_playing_p,
        MRB_ARGS_NONE());
    mrb_define_method(mrb, anim_class, "complete?", mrb_sprite_animation_complete_p,
        MRB_ARGS_NONE());
    mrb_define_method(mrb, anim_class, "loop?", mrb_sprite_animation_loop_p, MRB_ARGS_NONE());
    mrb_define_method(mrb, anim_class, "loop=", mrb_sprite_animation_set_loop, MRB_ARGS_REQ(1));

    // Frame access
    mrb_define_method(mrb, anim_class, "frame", mrb_sprite_animation_frame, MRB_ARGS_NONE());
    mrb_define_method(mrb, anim_class, "frame=", mrb_sprite_animation_set_frame, MRB_ARGS_REQ(1));

    // FPS
    mrb_define_method(mrb, anim_class, "fps", mrb_sprite_animation_fps, MRB_ARGS_NONE());
    mrb_define_method(mrb, anim_class, "fps=", mrb_sprite_animation_set_fps, MRB_ARGS_REQ(1));

    // Class methods
    mrb_define_class_method(mrb, anim_class, "count", mrb_sprite_animation_count,
        MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
