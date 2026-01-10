#include "gmr/bindings/animator.hpp"
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
#include <mruby/string.h>
#include <cstring>
#include <unordered_map>

// Error handling macros for fail-loud philosophy (per CONTRIBUTING.md)
#define GMR_REQUIRE_ANIMATOR_DATA(data) \
    if (!data) { \
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid Animator: internal data is null"); \
        return mrb_nil_value(); \
    }

#define GMR_REQUIRE_ANIMATOR_STATE(animator, handle) \
    if (!animator) { \
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Invalid Animator handle %d: animator may have been destroyed", handle); \
        return mrb_nil_value(); \
    }

namespace gmr {
namespace bindings {

/// @class GMR::Animator
/// @description Manages multiple named animations for a sprite with transition rules.
///   Allows defining animations like "idle", "run", "attack" and switching between them.
///   The Animator automatically updates the sprite's source_rect based on the current
///   animation frame, so you just need to call sprite.draw in your draw method.
///
///   Key concepts:
///   - Spritesheet layout: Define columns, frame_width, frame_height once for the whole sheet
///   - Named animations: Each animation has a name (symbol), frame indices, fps, and loop setting
///   - Transition rules: Control which animations can switch to which (optional)
///   - Global transitions: Some animations (like :hurt) can interrupt anything
///
/// @example # Complete platformer character with full animation system
///   include GMR
///
///   class Player
///     def initialize
///       # Load spritesheet texture (8 columns x 4 rows, 32x48 per frame)
///       @texture = Graphics::Texture.load("assets/player_sheet.png")
///       @sprite = Sprite.new(@texture)
///       @sprite.source_rect = Rect.new(0, 0, 32, 48)
///       @sprite.center_origin
///       @sprite.x, @sprite.y = 400, 300
///
///       # Create animator - shares spritesheet settings across all animations
///       @animator = Animator.new(@sprite, columns: 8, frame_width: 32, frame_height: 48)
///
///       # Define animations (frame indices map to spritesheet grid)
///       # Row 0: idle (frames 0-3), Row 1: run (frames 8-13)
///       # Row 2: jump/fall (frames 16-19), Row 3: attack (frames 24-29)
///       @animator.add(:idle, frames: 0..3, fps: 6)
///       @animator.add(:run, frames: 8..13, fps: 12)
///       @animator.add(:jump, frames: 16..17, fps: 8, loop: false)
///       @animator.add(:fall, frames: 18..19, fps: 8)
///       @animator.add(:attack, frames: 24..29, fps: 18, loop: false)
///       @animator.add(:hurt, frames: 30..31, fps: 10, loop: false)
///
///       # Define transition rules - which animations can go to which
///       @animator.allow_transition(:idle, :run)
///       @animator.allow_transition(:idle, :jump)
///       @animator.allow_transition(:idle, :attack)
///       @animator.allow_transition(:run, :idle)
///       @animator.allow_transition(:run, :jump)
///       @animator.allow_transition(:run, :attack)
///       @animator.allow_transition(:jump, :fall)
///       @animator.allow_transition(:fall, :idle)
///       @animator.allow_transition(:fall, :run)
///       @animator.allow_transition(:attack, :idle)
///       @animator.allow_transition(:hurt, :idle)
///
///       # Hurt can interrupt ANY animation (high priority)
///       @animator.allow_from_any(:hurt)
///
///       # Start with idle animation
///       @animator.play(:idle)
///
///       @velocity_y = 0
///       @on_ground = true
///     end
///
///     def update(dt)
///       handle_input(dt)
///       apply_physics(dt)
///       update_animation
///     end
///
///     def handle_input(dt)
///       speed = 200 * dt
///
///       if Input.action_down?(:move_left)
///         @sprite.x -= speed
///         @sprite.flip_x = true
///       elsif Input.action_down?(:move_right)
///         @sprite.x += speed
///         @sprite.flip_x = false
///       end
///
///       if Input.action_pressed?(:jump) && @on_ground
///         @velocity_y = -400
///         @on_ground = false
///       end
///
///       if Input.action_pressed?(:attack)
///         @animator.play(:attack)
///       end
///     end
///
///     def apply_physics(dt)
///       @velocity_y += 800 * dt  # gravity
///       @sprite.y += @velocity_y * dt
///
///       # Simple ground collision
///       if @sprite.y >= 300
///         @sprite.y = 300
///         @velocity_y = 0
///         @on_ground = true
///       end
///     end
///
///     def update_animation
///       # Don't interrupt attack animation
///       return if @animator.current == :attack && @animator.playing?
///
///       if !@on_ground
///         @animator.play(@velocity_y < 0 ? :jump : :fall)
///       elsif Input.action_down?(:move_left) || Input.action_down?(:move_right)
///         @animator.play(:run)
///       else
///         @animator.play(:idle)
///       end
///     end
///
///     def take_damage
///       @animator.play(:hurt)  # Interrupts anything due to allow_from_any
///     end
///
///     def draw
///       @sprite.draw  # Animator automatically updates source_rect
///     end
///   end
///
/// @example # Enemy with simple animation (no transition rules)
///   class Slime
///     def initialize(x, y)
///       @sprite = Sprite.new(Graphics::Texture.load("assets/slime.png"))
///       @sprite.x, @sprite.y = x, y
///
///       # Simple animator without transition rules (all transitions allowed)
///       @animator = Animator.new(@sprite, columns: 4, frame_width: 24, frame_height: 24)
///       @animator.add(:bounce, frames: 0..3, fps: 8)
///       @animator.add(:squish, frames: 4..6, fps: 12, loop: false)
///       @animator.play(:bounce)
///     end
///
///     def squish!
///       @animator.play(:squish)
///       @animator.on_complete { @dead = true }
///     end
///   end
///
/// @example # NPC with queued animations using finish_current
///   class Shopkeeper
///     def initialize
///       @sprite = Sprite.new(Graphics::Texture.load("assets/shopkeeper.png"))
///       @animator = Animator.new(@sprite, columns: 6, frame_width: 32, frame_height: 48)
///
///       @animator.add(:idle, frames: 0..3, fps: 4)
///       @animator.add(:wave, frames: 6..11, fps: 10, loop: false)
///       @animator.add(:nod, frames: 12..15, fps: 8, loop: false)
///
///       @animator.allow_transition(:idle, :wave)
///       @animator.allow_transition(:idle, :nod)
///       @animator.allow_transition(:wave, :idle)
///       @animator.allow_transition(:nod, :idle)
///
///       @animator.play(:idle)
///     end
///
///     def greet_player
///       # Queue wave animation - will play after current animation finishes
///       @animator.play(:wave, transition: :finish_current)
///     end
///
///     def confirm_purchase
///       @animator.play(:nod, transition: :finish_current)
///     end
///   end

// ============================================================================
// Animator Binding Data
// ============================================================================

struct AnimatorData {
    AnimatorHandle handle;
};

static void animator_free(mrb_state* mrb, void* ptr) {
    AnimatorData* data = static_cast<AnimatorData*>(ptr);
    if (data) {
        // LIFECYCLE: Ruby wrapper owns the animator handle.
        // When Ruby GC collects this object, we must clean up the native animator.
        animation::AnimationManager::instance().destroy_animator(data->handle);
        mrb_free(mrb, data);
    }
}

static const mrb_data_type animator_data_type = {
    "Animator", animator_free
};

static AnimatorData* get_animator_data(mrb_state* mrb, mrb_value self) {
    return static_cast<AnimatorData*>(mrb_data_get_ptr(mrb, self, &animator_data_type));
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
// Helper: Convert Ruby symbol to string (with caching for performance)
// ============================================================================

// Cache for symbol-to-string conversions to avoid repeated allocations
static std::unordered_map<mrb_sym, std::string> g_symbol_cache;

static std::string symbol_to_string(mrb_state* mrb, mrb_value sym) {
    if (!mrb_symbol_p(sym)) return "";

    mrb_sym id = mrb_symbol(sym);

    // Check cache first
    auto it = g_symbol_cache.find(id);
    if (it != g_symbol_cache.end()) {
        return it->second;
    }

    // Cache miss - convert and store
    mrb_int len;
    const char* name = mrb_sym2name_len(mrb, id, &len);
    std::string result(name, len);
    g_symbol_cache[id] = result;
    return result;
}

// ============================================================================
// Helper: List available animation names for error messages
// ============================================================================

static std::string list_animation_names(const animation::AnimatorState* animator) {
    if (animator->animations.empty()) {
        return "(none defined)";
    }

    std::string result;
    bool first = true;
    for (const auto& pair : animator->animations) {
        if (!first) result += ", ";
        result += ":";
        result += pair.first;
        first = false;
    }
    return result;
}

// ============================================================================
// GMR::Animator.new(sprite, columns:, frame_width:, frame_height:)
// ============================================================================

/// @method initialize
/// @description Create a new animator for a sprite. The animator manages the sprite's
///   source_rect automatically based on the current animation frame.
/// @param sprite [Sprite] The sprite to animate
/// @param columns: [Integer] Number of columns in spritesheet (default: 1)
/// @param frame_width: [Integer] Width of each frame (default: inferred from source_rect)
/// @param frame_height: [Integer] Height of each frame (default: inferred from source_rect)
/// @example # Basic setup with a 4x4 spritesheet (16 frames total)
///   @texture = Graphics::Texture.load("assets/character.png")
///   @sprite = Sprite.new(@texture)
///   @sprite.source_rect = Rect.new(0, 0, 64, 64)  # First frame size
///   @animator = Animator.new(@sprite, columns: 4, frame_width: 64, frame_height: 64)
/// @example # Infer frame size from sprite's source_rect
///   @sprite.source_rect = Rect.new(0, 0, 32, 48)
///   @animator = Animator.new(@sprite, columns: 8)  # Uses 32x48 from source_rect
static mrb_value mrb_animator_initialize(mrb_state* mrb, mrb_value self) {
    mrb_value sprite_val;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "o|H", &sprite_val, &kwargs);

    // Get sprite handle
    SpriteHandle sprite_handle = get_sprite_handle(mrb, sprite_val);
    if (sprite_handle == INVALID_HANDLE) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Expected Sprite object");
        return mrb_nil_value();
    }

    auto& manager = animation::AnimationManager::instance();

    // Create animator
    AnimatorHandle handle = manager.create_animator();
    animation::AnimatorState* animator = manager.get_animator(handle);

    animator->sprite = sprite_handle;
    animator->sprite_ref = sprite_val;

    // Get sprite to infer frame dimensions if not provided
    SpriteState* sprite = SpriteManager::instance().get(sprite_handle);

    if (!mrb_nil_p(kwargs)) {
        // Parse columns (default: 1)
        mrb_value columns_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "columns")));
        animator->columns = mrb_nil_p(columns_val) ? 1 : static_cast<int>(mrb_fixnum(columns_val));

        // Parse frame_width (default: from source_rect or 0)
        mrb_value frame_width_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "frame_width")));
        if (!mrb_nil_p(frame_width_val)) {
            animator->frame_width = static_cast<int>(mrb_fixnum(frame_width_val));
        } else if (sprite && sprite->use_source_rect) {
            animator->frame_width = static_cast<int>(sprite->source_rect.width);
        }

        // Parse frame_height (default: from source_rect or 0)
        mrb_value frame_height_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "frame_height")));
        if (!mrb_nil_p(frame_height_val)) {
            animator->frame_height = static_cast<int>(mrb_fixnum(frame_height_val));
        } else if (sprite && sprite->use_source_rect) {
            animator->frame_height = static_cast<int>(sprite->source_rect.height);
        }
    } else {
        animator->columns = 1;
        if (sprite && sprite->use_source_rect) {
            animator->frame_width = static_cast<int>(sprite->source_rect.width);
            animator->frame_height = static_cast<int>(sprite->source_rect.height);
        }
    }

    // Attach to Ruby object
    AnimatorData* data = static_cast<AnimatorData*>(
        mrb_malloc(mrb, sizeof(AnimatorData)));
    data->handle = handle;
    mrb_data_init(self, data, &animator_data_type);

    // Store sprite reference to prevent GC
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@sprite"), sprite_val);

    // Store animations hash for Ruby-side access
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@animations"), mrb_hash_new(mrb));

    // Register with GC
    animator->ruby_animator_obj = self;
    mrb_gc_register(mrb, self);

    return self;
}

// ============================================================================
// add(name, frames:, fps:, loop:)
// ============================================================================

/// @method add
/// @description Add a named animation. Frame indices correspond to positions in the
///   spritesheet grid (left-to-right, top-to-bottom). For example, in an 8-column sheet:
///   - Frame 0 = row 0, col 0
///   - Frame 7 = row 0, col 7
///   - Frame 8 = row 1, col 0
/// @param name [Symbol] Animation name (e.g., :idle, :run, :attack)
/// @param frames: [Array<Integer>, Range] Frame indices to cycle through
/// @param fps: [Float] Frames per second (default: 12)
/// @param loop: [Boolean] Whether to loop the animation (default: true)
/// @returns [Animator] self for chaining
/// @example # Typical character animations from a spritesheet
///   # 8-column spritesheet layout:
///   # Row 0 (frames 0-7): idle
///   # Row 1 (frames 8-15): run
///   # Row 2 (frames 16-23): attack
///   @animator.add(:idle, frames: 0..3, fps: 6)           # Slow idle breathing
///   @animator.add(:run, frames: 8..13, fps: 12)          # Fast run cycle
///   @animator.add(:attack, frames: 16..21, fps: 18, loop: false)  # One-shot attack
/// @example # Using explicit frame arrays for non-sequential animations
///   @animator.add(:blink, frames: [0, 1, 0], fps: 4)     # Blink and return
///   @animator.add(:combo, frames: [16, 17, 18, 17, 19, 20], fps: 15, loop: false)
static mrb_value mrb_animator_add(mrb_state* mrb, mrb_value self) {
    mrb_value name_val;
    mrb_value kwargs;

    mrb_get_args(mrb, "oH", &name_val, &kwargs);

    AnimatorData* data = get_animator_data(mrb, self);
    if (!data) return self;

    animation::AnimatorState* animator =
        animation::AnimationManager::instance().get_animator(data->handle);
    if (!animator) return self;

    std::string name = symbol_to_string(mrb, name_val);
    if (name.empty()) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Animation name must be a symbol");
        return self;
    }

    auto& manager = animation::AnimationManager::instance();

    // Create a new SpriteAnimation for this animation
    SpriteAnimationHandle anim_handle = manager.create_animation();
    animation::SpriteAnimationState* anim = manager.get_animation(anim_handle);

    anim->sprite = animator->sprite;
    anim->sprite_ref = animator->sprite_ref;

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
        return self;
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

    // Use animator's shared spritesheet settings
    anim->frame_width = animator->frame_width;
    anim->frame_height = animator->frame_height;
    anim->columns = animator->columns;

    // Store in animator
    animation::AnimationEntry entry;
    entry.animation = anim_handle;
    animator->animations[name] = entry;

    return self;
}

// ============================================================================
// allow_transition(from, to)
// ============================================================================

/// @method allow_transition
/// @description Allow transition from one animation to another. If no transition rules
///   are defined, all transitions are allowed by default. Once you define any rule,
///   only explicitly allowed transitions will work.
/// @param from [Symbol] Source animation name
/// @param to [Symbol] Destination animation name
/// @returns [Animator] self for chaining
/// @example # Platformer character state flow
///   # Ground states can go to each other and to jump
///   @animator.allow_transition(:idle, :run)
///   @animator.allow_transition(:idle, :jump)
///   @animator.allow_transition(:run, :idle)
///   @animator.allow_transition(:run, :jump)
///
///   # Air states flow: jump -> fall -> land
///   @animator.allow_transition(:jump, :fall)
///   @animator.allow_transition(:fall, :idle)
///   @animator.allow_transition(:fall, :run)
///
///   # Attack can be triggered from ground states, returns to idle
///   @animator.allow_transition(:idle, :attack)
///   @animator.allow_transition(:run, :attack)
///   @animator.allow_transition(:attack, :idle)
static mrb_value mrb_animator_allow_transition(mrb_state* mrb, mrb_value self) {
    mrb_value from_val, to_val;
    mrb_get_args(mrb, "oo", &from_val, &to_val);

    AnimatorData* data = get_animator_data(mrb, self);
    if (!data) return self;

    animation::AnimatorState* animator =
        animation::AnimationManager::instance().get_animator(data->handle);
    if (!animator) return self;

    std::string from = symbol_to_string(mrb, from_val);
    std::string to = symbol_to_string(mrb, to_val);

    if (from.empty() || to.empty()) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Animation names must be symbols");
        return self;
    }

    animator->transitions[from].insert(to);

    return self;
}

// ============================================================================
// allow_from_any(name)
// ============================================================================

/// @method allow_from_any
/// @description Allow this animation to be played from any other animation.
///   Useful for high-priority animations like "hurt" or "death" that can interrupt anything.
///   This bypasses all transition rules for the specified animation.
/// @param name [Symbol] Animation name that can interrupt any animation
/// @returns [Animator] self for chaining
/// @example # High-priority animations that can interrupt anything
///   @animator.allow_from_any(:hurt)   # Taking damage interrupts any action
///   @animator.allow_from_any(:death)  # Death animation always plays
///   @animator.allow_from_any(:stun)   # Stun effect interrupts movement/attacks
/// @example # Use case: Enemy interrupts player attack with counter
///   def take_damage
///     # Even if player is mid-attack, hurt animation plays immediately
///     @animator.play(:hurt)
///     @invincible_timer = 0.5
///   end
static mrb_value mrb_animator_allow_from_any(mrb_state* mrb, mrb_value self) {
    mrb_value name_val;
    mrb_get_args(mrb, "o", &name_val);

    AnimatorData* data = get_animator_data(mrb, self);
    if (!data) return self;

    animation::AnimatorState* animator =
        animation::AnimationManager::instance().get_animator(data->handle);
    if (!animator) return self;

    std::string name = symbol_to_string(mrb, name_val);
    if (name.empty()) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Animation name must be a symbol");
        return self;
    }

    animator->global_transitions.insert(name);

    return self;
}

// ============================================================================
// play(name, transition:)
// ============================================================================

/// @method play
/// @description Switch to a named animation. Checks transition rules before switching.
///   If the requested animation is already playing, this does nothing.
///   If transition rules exist and the transition is not allowed, this does nothing.
/// @param name [Symbol] Animation name to play
/// @param transition: [Symbol] :immediate (default) or :finish_current
///   - :immediate - Switch immediately, interrupting current animation
///   - :finish_current - Queue the animation, play after current one completes
/// @returns [Animator] self for chaining
/// @example # Basic animation switching in update loop
///   def update(dt)
///     if Input.action_down?(:move_right)
///       @sprite.flip_x = false
///       @animator.play(:run)
///     elsif Input.action_down?(:move_left)
///       @sprite.flip_x = true
///       @animator.play(:run)
///     else
///       @animator.play(:idle)
///     end
///   end
/// @example # Queue animation to play after current finishes
///   def start_attack_combo
///     @animator.play(:attack1)
///     @animator.play(:attack2, transition: :finish_current)  # Queued
///   end
/// @example # Conditional play with transition check
///   def try_attack
///     if @animator.can_play?(:attack)
///       @animator.play(:attack)
///       @attack_cooldown = 0.5
///     end
///   end
static mrb_value mrb_animator_play(mrb_state* mrb, mrb_value self) {
    mrb_value name_val;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "o|H", &name_val, &kwargs);

    AnimatorData* data = get_animator_data(mrb, self);
    if (!data) return self;

    auto& manager = animation::AnimationManager::instance();
    animation::AnimatorState* animator = manager.get_animator(data->handle);
    if (!animator) return self;

    std::string name = symbol_to_string(mrb, name_val);
    if (name.empty()) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Animation name must be a symbol");
        return self;
    }

    // Check if animation exists - fail loudly per CONTRIBUTING.md
    if (!animator->has_animation(name)) {
        std::string available = list_animation_names(animator);
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "Unknown animation: :%s. Available animations: %s",
            name.c_str(), available.c_str());
        return self;
    }

    // Already playing this animation?
    if (animator->current_animation == name) {
        return self;
    }

    // Check transition rules
    if (!animator->can_transition(name)) {
        return self;  // Transition not allowed
    }

    // Determine transition mode
    animation::AnimationTransition transition = animation::AnimationTransition::Immediate;
    if (!mrb_nil_p(kwargs)) {
        mrb_value transition_val = mrb_hash_get(mrb, kwargs,
            mrb_symbol_value(mrb_intern_lit(mrb, "transition")));
        if (!mrb_nil_p(transition_val)) {
            std::string trans_str = symbol_to_string(mrb, transition_val);
            if (trans_str == "finish_current") {
                transition = animation::AnimationTransition::FinishCurrent;
            }
        }
    }

    // Handle FinishCurrent mode
    if (transition == animation::AnimationTransition::FinishCurrent) {
        SpriteAnimationHandle current = animator->current_handle();
        if (current != INVALID_HANDLE) {
            animation::SpriteAnimationState* current_anim = manager.get_animation(current);
            if (current_anim && current_anim->playing && !current_anim->completed) {
                // Queue for later
                animator->queued_animation = name;
                return self;
            }
        }
    }

    // Immediate switch
    // Stop current animation
    SpriteAnimationHandle current = animator->current_handle();
    if (current != INVALID_HANDLE) {
        animation::SpriteAnimationState* current_anim = manager.get_animation(current);
        if (current_anim) {
            current_anim->playing = false;
        }
    }

    // Start new animation
    animator->current_animation = name;
    animator->queued_animation.clear();

    auto it = animator->animations.find(name);
    if (it != animator->animations.end()) {
        animation::SpriteAnimationState* new_anim = manager.get_animation(it->second.animation);
        if (new_anim) {
            new_anim->current_frame_index = 0;
            new_anim->elapsed = 0.0f;
            new_anim->playing = true;
            new_anim->completed = false;
        }
    }

    return self;
}

// ============================================================================
// stop
// ============================================================================

/// @method stop
/// @description Stop the current animation and clear any queued animation.
///   The sprite will freeze on the current frame.
/// @returns [Animator] self for chaining
/// @example # Pause animation when game is paused
///   def pause_game
///     @animator.stop
///     @paused = true
///   end
/// @example # Stop animation when character dies
///   def die
///     @animator.play(:death)
///     @animator.on_complete { @animator.stop }  # Freeze on last frame
///   end
static mrb_value mrb_animator_stop(mrb_state* mrb, mrb_value self) {
    AnimatorData* data = get_animator_data(mrb, self);
    if (!data) return self;

    auto& manager = animation::AnimationManager::instance();
    animation::AnimatorState* animator = manager.get_animator(data->handle);
    if (!animator) return self;

    SpriteAnimationHandle current = animator->current_handle();
    if (current != INVALID_HANDLE) {
        animation::SpriteAnimationState* anim = manager.get_animation(current);
        if (anim) {
            anim->playing = false;
        }
    }

    animator->queued_animation.clear();

    return self;
}

// ============================================================================
// current
// ============================================================================

/// @method current
/// @description Get the current animation name.
/// @returns [Symbol, nil] Current animation name or nil if no animation is set
/// @example # Check current animation state
///   def update(dt)
///     # Don't allow movement during attack
///     return if @animator.current == :attack
///     handle_movement(dt)
///   end
/// @example # Debug output
///   def draw
///     @sprite.draw
///     Graphics.draw_text("Anim: #{@animator.current}", 10, 10, 16, [255,255,255])
///   end
static mrb_value mrb_animator_current(mrb_state* mrb, mrb_value self) {
    AnimatorData* data = get_animator_data(mrb, self);
    GMR_REQUIRE_ANIMATOR_DATA(data);

    animation::AnimatorState* animator =
        animation::AnimationManager::instance().get_animator(data->handle);
    if (!animator || animator->current_animation.empty()) {
        return mrb_nil_value();
    }

    return mrb_symbol_value(mrb_intern_cstr(mrb, animator->current_animation.c_str()));
}

// ============================================================================
// playing?
// ============================================================================

/// @method playing?
/// @description Check if any animation is currently playing (not stopped/completed).
/// @returns [Boolean] true if an animation is actively playing
/// @example # Wait for attack animation to finish before allowing another
///   def update(dt)
///     if Input.action_pressed?(:attack)
///       unless @animator.current == :attack && @animator.playing?
///         @animator.play(:attack)
///       end
///     end
///   end
/// @example # Check if death animation finished
///   def update(dt)
///     if @animator.current == :death && !@animator.playing?
///       remove_from_game
///     end
///   end
static mrb_value mrb_animator_playing_p(mrb_state* mrb, mrb_value self) {
    AnimatorData* data = get_animator_data(mrb, self);
    GMR_REQUIRE_ANIMATOR_DATA(data);

    auto& manager = animation::AnimationManager::instance();
    animation::AnimatorState* animator = manager.get_animator(data->handle);
    GMR_REQUIRE_ANIMATOR_STATE(animator, data->handle);

    SpriteAnimationHandle current = animator->current_handle();
    if (current == INVALID_HANDLE) return mrb_false_value();

    animation::SpriteAnimationState* anim = manager.get_animation(current);
    if (!anim) return mrb_false_value();

    return mrb_bool_value(anim->playing);
}

// ============================================================================
// can_play?(name)
// ============================================================================

/// @method can_play?
/// @description Check if transition to the given animation is allowed from current.
///   Returns false if the animation doesn't exist or if transition rules block it.
/// @param name [Symbol] Animation name to check
/// @returns [Boolean] true if transition is allowed
/// @example # Only show attack UI prompt when attack is available
///   def draw_ui
///     if @animator.can_play?(:attack)
///       Graphics.draw_text("Press Z to Attack", 10, 50, 16, [255, 255, 255])
///     end
///   end
/// @example # AI decision making based on available animations
///   def ai_update
///     if @animator.can_play?(:attack) && player_in_range?
///       @animator.play(:attack)
///     elsif @animator.can_play?(:run)
///       @animator.play(:run)
///       move_toward_player
///     end
///   end
static mrb_value mrb_animator_can_play_p(mrb_state* mrb, mrb_value self) {
    mrb_value name_val;
    mrb_get_args(mrb, "o", &name_val);

    AnimatorData* data = get_animator_data(mrb, self);
    GMR_REQUIRE_ANIMATOR_DATA(data);

    animation::AnimatorState* animator =
        animation::AnimationManager::instance().get_animator(data->handle);
    GMR_REQUIRE_ANIMATOR_STATE(animator, data->handle);

    std::string name = symbol_to_string(mrb, name_val);
    if (name.empty()) return mrb_false_value();

    // Check if animation exists
    if (!animator->has_animation(name)) return mrb_false_value();

    return mrb_bool_value(animator->can_transition(name));
}

// ============================================================================
// on_complete
// ============================================================================

/// @method on_complete
/// @description Set a callback for when a specific animation completes.
///   The callback is invoked when the named non-looping animation finishes.
/// @param name [Symbol] Animation name to attach callback to
/// @returns [Animator] self for chaining
/// @example # Chain animations with per-animation callbacks
///   @animator.on_complete(:land) { @animator.play(:idle) }
///   @animator.on_complete(:jump) { @animator.play(:fall) }
///   @animator.on_complete(:attack) { @can_attack_again = true }
/// @example # Trigger game events on animation completion
///   @animator.on_complete(:door_open) do
///     @door_fully_open = true
///     Audio::Sound.play("assets/door_click.wav")
///   end
/// @example # Death animation cleanup
///   @animator.on_complete(:death) do
///     spawn_loot
///     remove_from_game
///   end
static mrb_value mrb_animator_on_complete(mrb_state* mrb, mrb_value self) {
    mrb_value name_val;
    mrb_value block;
    mrb_get_args(mrb, "o&", &name_val, &block);

    AnimatorData* data = get_animator_data(mrb, self);
    if (!data) return self;

    animation::AnimatorState* animator =
        animation::AnimationManager::instance().get_animator(data->handle);
    if (!animator) return self;

    std::string name = symbol_to_string(mrb, name_val);
    if (name.empty()) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "Animation name must be a symbol");
        return self;
    }

    // Store per-animation callback
    animator->on_complete_callbacks[name] = block;

    // Store in instance variable for GC protection (use animation name as key)
    mrb_value callbacks_hash = mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, "@on_complete_callbacks"));
    if (mrb_nil_p(callbacks_hash)) {
        callbacks_hash = mrb_hash_new(mrb);
        mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@on_complete_callbacks"), callbacks_hash);
    }
    mrb_hash_set(mrb, callbacks_hash, name_val, block);

    return self;
}

// ============================================================================
// [] (get animation by name)
// ============================================================================

/// @method []
/// @description Get animation info by name. Returns a hash with fps, loop, frame_count, and playing status.
/// @param name [Symbol] Animation name
/// @returns [Hash, nil] Animation info hash or nil if animation doesn't exist
///   - :fps [Float] - Frames per second
///   - :loop [Boolean] - Whether animation loops
///   - :frame_count [Integer] - Number of frames in animation
///   - :playing [Boolean] - Whether this animation is currently playing
/// @example # Debug animation info
///   info = @animator[:attack]
///   puts "Attack: #{info[:frame_count]} frames at #{info[:fps]} fps"
/// @example # Dynamic FPS adjustment based on game state
///   if @speed_boost_active
///     current_info = @animator[@animator.current]
///     # Note: Would need fps= method to modify (not currently implemented)
///   end
static mrb_value mrb_animator_get(mrb_state* mrb, mrb_value self) {
    mrb_value name_val;
    mrb_get_args(mrb, "o", &name_val);

    AnimatorData* data = get_animator_data(mrb, self);
    GMR_REQUIRE_ANIMATOR_DATA(data);

    auto& manager = animation::AnimationManager::instance();
    animation::AnimatorState* animator = manager.get_animator(data->handle);
    GMR_REQUIRE_ANIMATOR_STATE(animator, data->handle);

    std::string name = symbol_to_string(mrb, name_val);
    if (name.empty()) return mrb_nil_value();

    auto it = animator->animations.find(name);
    if (it == animator->animations.end()) return mrb_nil_value();

    animation::SpriteAnimationState* anim = manager.get_animation(it->second.animation);
    if (!anim) return mrb_nil_value();

    // Return a hash with animation info
    mrb_value hash = mrb_hash_new(mrb);
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "fps")),
        mrb_float_value(mrb, anim->fps));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "loop")),
        mrb_bool_value(anim->loop));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "frame_count")),
        mrb_fixnum_value(static_cast<mrb_int>(anim->frames.size())));
    mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "playing")),
        mrb_bool_value(anim->playing));

    return hash;
}

// ============================================================================
// Class Methods
// ============================================================================

/// @method count
/// @description Get the number of active animators in the game. Class method.
/// @returns [Integer] Number of active animators
/// @example # Debug: show animator count
///   Graphics.draw_text("Animators: #{Animator.count}", 10, 10, 16, [255,255,255])
static mrb_value mrb_animator_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(animation::AnimationManager::instance().animator_count());
}

// ============================================================================
// Registration
// ============================================================================

void register_animator(mrb_state* mrb) {
    // Animator class under GMR::Animation
    RClass* gmr = get_gmr_module(mrb);
    RClass* animation = mrb_module_get_under(mrb, gmr, "Animation");
    RClass* animator_class = mrb_define_class_under(mrb, animation, "Animator", mrb->object_class);
    MRB_SET_INSTANCE_TT(animator_class, MRB_TT_CDATA);

    // Constructor
    mrb_define_method(mrb, animator_class, "initialize", mrb_animator_initialize,
        MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));

    // Animation definition
    mrb_define_method(mrb, animator_class, "add", mrb_animator_add,
        MRB_ARGS_REQ(2));

    // Transition rules
    mrb_define_method(mrb, animator_class, "allow_transition", mrb_animator_allow_transition,
        MRB_ARGS_REQ(2));
    mrb_define_method(mrb, animator_class, "allow_from_any", mrb_animator_allow_from_any,
        MRB_ARGS_REQ(1));

    // Playback control
    mrb_define_method(mrb, animator_class, "play", mrb_animator_play,
        MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
    mrb_define_method(mrb, animator_class, "stop", mrb_animator_stop, MRB_ARGS_NONE());

    // State queries
    mrb_define_method(mrb, animator_class, "current", mrb_animator_current, MRB_ARGS_NONE());
    mrb_define_method(mrb, animator_class, "playing?", mrb_animator_playing_p, MRB_ARGS_NONE());
    mrb_define_method(mrb, animator_class, "can_play?", mrb_animator_can_play_p, MRB_ARGS_REQ(1));

    // Callbacks
    mrb_define_method(mrb, animator_class, "on_complete", mrb_animator_on_complete,
        MRB_ARGS_REQ(1) | MRB_ARGS_BLOCK());

    // Animation access
    mrb_define_method(mrb, animator_class, "[]", mrb_animator_get, MRB_ARGS_REQ(1));

    // Class methods
    mrb_define_class_method(mrb, animator_class, "count", mrb_animator_count,
        MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
