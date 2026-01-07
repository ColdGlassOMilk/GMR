#include "gmr/bindings/tween.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/animation/animation_manager.hpp"
#include "gmr/animation/easing.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <cstring>

namespace gmr {
namespace bindings {

/// @class GMR::Tween
/// @description Animates a numeric property on any Ruby object over time.
///   Tweens update automatically each frame - no manual update calls needed.
///   Creating a new tween for the same target+property cancels the previous one.
/// @example # Player death sequence: fade out, disable, trigger camera shake
///   class Player
///     def initialize
///       @sprite = Sprite.new(GMR::Graphics::Texture.load("assets/player.png"))
///       @sprite.alpha = 1.0
///       @alive = true
///     end
///
///     def die
///       return unless @alive
///       @alive = false
///       GMR::Audio::Sound.play("assets/death.wav")
///       Camera2D.current.shake(strength: 8, duration: 0.3)
///       GMR::Tween.to(@sprite, :alpha, 0.0, duration: 0.8, ease: :out_quad)
///         .on_complete { @sprite.visible = false }
///     end
///   end
/// @example # UI slide-in animation with bounce
///   class MenuPanel
///     def initialize
///       @panel_x = -200  # Start off-screen
///     end
///
///     def show
///       GMR::Tween.to(self, :panel_x, 50, duration: 0.4, ease: :out_back)
///     end
///
///     def hide
///       GMR::Tween.to(self, :panel_x, -200, duration: 0.3, ease: :in_quad)
///     end
///
///     def draw
///       GMR::Graphics.draw_rect(@panel_x, 100, 180, 300, [40, 40, 60])
///     end
///   end
/// @example # Collectible bounce and fade when picked up
///   class Coin
///     def initialize(x, y)
///       @sprite = Sprite.new(GMR::Graphics::Texture.load("assets/coin.png"))
///       @sprite.x = x
///       @sprite.y = y
///       @collected = false
///     end
///
///     def collect
///       return if @collected
///       @collected = true
///       GMR::Audio::Sound.play("assets/coin.wav")
///       # Bounce up while fading
///       GMR::Tween.to(@sprite, :y, @sprite.y - 40, duration: 0.3, ease: :out_quad)
///       GMR::Tween.to(@sprite, :alpha, 0.0, duration: 0.3, delay: 0.1)
///         .on_complete { @sprite.visible = false }
///     end
///   end

// ============================================================================
// Tween Binding Data
// ============================================================================

struct TweenData {
    TweenHandle handle;
};

static void tween_free(mrb_state* mrb, void* ptr) {
    TweenData* data = static_cast<TweenData*>(ptr);
    if (data) {
        // LIFECYCLE: AnimationManager owns tween lifecycle, not Ruby.
        //
        // Tweens are automatically cleaned up by the manager when:
        // - They complete naturally (duration elapsed)
        // - cancel() is called explicitly
        // - cancel_all() is called (e.g., scene transitions)
        // - The manager's clear() method is called
        //
        // The Ruby wrapper is just a handle for control (pause/resume/cancel).
        // We do NOT call destroy_tween here because:
        // 1. The tween may have already completed and been removed
        // 2. Multiple Ruby references to same handle would cause double-free
        // 3. Tweens may outlive Ruby references intentionally (fire-and-forget)
        //
        // This design allows patterns like:
        //   Tween.to(@sprite, :alpha, 0, duration: 1.0)  # No variable stored
        //
        mrb_free(mrb, data);
    }
}

static const mrb_data_type tween_data_type = {
    "Tween", tween_free
};

static TweenData* get_tween_data(mrb_state* mrb, mrb_value self) {
    return static_cast<TweenData*>(mrb_data_get_ptr(mrb, self, &tween_data_type));
}

// ============================================================================
// Helper: Create Tween Object
// ============================================================================

static mrb_value create_tween_object(mrb_state* mrb, TweenHandle handle) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* tween_class = mrb_class_get_under(mrb, gmr, "Tween");

    mrb_value tween_obj = mrb_obj_value(mrb_obj_alloc(mrb, MRB_TT_CDATA, tween_class));

    TweenData* data = static_cast<TweenData*>(mrb_malloc(mrb, sizeof(TweenData)));
    data->handle = handle;
    mrb_data_init(tween_obj, data, &tween_data_type);

    return tween_obj;
}

// ============================================================================
// Helper: Parse Tween Options
// ============================================================================

struct TweenOptions {
    float duration{0.0f};
    float delay{0.0f};
    animation::EasingType easing{animation::EasingType::LINEAR};
    bool has_duration{false};
};

static TweenOptions parse_tween_options(mrb_state* mrb, mrb_value kwargs) {
    TweenOptions opts;

    if (mrb_nil_p(kwargs)) {
        return opts;
    }

    // duration (required)
    mrb_value duration_val = mrb_hash_get(mrb, kwargs,
        mrb_symbol_value(mrb_intern_lit(mrb, "duration")));
    if (!mrb_nil_p(duration_val)) {
        opts.duration = static_cast<float>(mrb_as_float(mrb, duration_val));
        opts.has_duration = true;
    }

    // delay (optional, default 0)
    mrb_value delay_val = mrb_hash_get(mrb, kwargs,
        mrb_symbol_value(mrb_intern_lit(mrb, "delay")));
    if (!mrb_nil_p(delay_val)) {
        opts.delay = static_cast<float>(mrb_as_float(mrb, delay_val));
    }

    // ease (optional symbol, default :linear)
    mrb_value ease_val = mrb_hash_get(mrb, kwargs,
        mrb_symbol_value(mrb_intern_lit(mrb, "ease")));
    if (mrb_symbol_p(ease_val)) {
        opts.easing = animation::symbol_to_easing(mrb, mrb_symbol(ease_val));
    }

    return opts;
}

// ============================================================================
// GMR::Tween.to(target, property, end_value, duration:, delay:, ease:)
// ============================================================================

/// @method to
/// @description Create a tween that animates a property TO a target value.
///   The tween starts from the property's current value.
/// @param target [Object] The object to animate (must have property getter and setter)
/// @param property [Symbol] The property to animate (:x, :y, :alpha, :rotation, etc.)
/// @param end_value [Float] The target value to animate to
/// @param duration: [Float] Duration in seconds (required)
/// @param delay: [Float] Delay before starting (default: 0)
/// @param ease: [Symbol] Easing function (default: :linear)
/// @returns [Tween] The tween instance for chaining
/// @example # Enemy knocked back on hit with damage flash
///   class Enemy
///     def take_damage(direction)
///       @health -= 10
///       # Knockback in direction of hit
///       target_x = @sprite.x + direction * 50
///       GMR::Tween.to(@sprite, :x, target_x, duration: 0.15, ease: :out_quad)
///       # Flash red then restore color
///       @sprite.color = [255, 100, 100]
///       GMR::Tween.to(@sprite, :color_r, 255, duration: 0.2, delay: 0.1)
///     end
///   end
/// @example # Smooth camera zoom for scope/aim mode
///   def toggle_aim_mode
///     @aiming = !@aiming
///     target_zoom = @aiming ? 2.0 : 1.0
///     GMR::Tween.to(@camera, :zoom, target_zoom, duration: 0.3, ease: :out_cubic)
///   end
static mrb_value mrb_tween_to(mrb_state* mrb, mrb_value) {
    mrb_value target;
    mrb_sym property_sym;
    mrb_float end_value;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "onf|H", &target, &property_sym, &end_value, &kwargs);

    const char* property = mrb_sym_name(mrb, property_sym);

    // Parse options
    TweenOptions opts = parse_tween_options(mrb, kwargs);
    if (!opts.has_duration) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "duration: is required");
        return mrb_nil_value();
    }

    auto& manager = animation::AnimationManager::instance();

    // Auto-cancel any existing tween for this target+property
    manager.cancel_tweens_for_property(mrb, target, property);

    // Create tween
    TweenHandle handle = manager.create_tween();
    animation::TweenState* tween = manager.get_tween(handle);

    // Get current value as start
    float start_value = manager.get_property_value(mrb, target, property);

    // Initialize tween state
    tween->target = target;
    tween->property = property;
    tween->start_value = start_value;
    tween->end_value = static_cast<float>(end_value);
    tween->duration = opts.duration;
    tween->delay = opts.delay;
    tween->delay_remaining = opts.delay;
    tween->easing = opts.easing;
    tween->active = true;

    // Create Ruby object
    mrb_value tween_obj = create_tween_object(mrb, handle);

    // Store Ruby object reference for GC registration
    tween->ruby_tween_obj = tween_obj;
    mrb_gc_register(mrb, tween_obj);

    // Store target as instance variable to prevent GC
    mrb_iv_set(mrb, tween_obj, mrb_intern_cstr(mrb, "@target"), target);

    return tween_obj;
}

// ============================================================================
// GMR::Tween.from(target, property, start_value, duration:, delay:, ease:)
// ============================================================================

/// @method from
/// @description Create a tween that animates a property FROM a start value to current.
///   Useful for "animate in" effects like fading in from transparent.
/// @param target [Object] The object to animate
/// @param property [Symbol] The property to animate
/// @param start_value [Float] The value to animate from
/// @param duration: [Float] Duration in seconds (required)
/// @param delay: [Float] Delay before starting (default: 0)
/// @param ease: [Symbol] Easing function (default: :linear)
/// @returns [Tween] The tween instance for chaining
/// @example # New scene elements pop in with scale effect
///   class GameScene < GMR::Scene
///     def init
///       @title = Sprite.new(GMR::Graphics::Texture.load("assets/title.png"))
///       @title.x = 400
///       @title.y = 100
///       @title.scale_x = 1.0
///       @title.scale_y = 1.0
///       # Pop in from zero scale with overshoot
///       GMR::Tween.from(@title, :scale_x, 0.0, duration: 0.4, ease: :out_back)
///       GMR::Tween.from(@title, :scale_y, 0.0, duration: 0.4, ease: :out_back)
///       # Fade in slightly delayed
///       GMR::Tween.from(@title, :alpha, 0.0, duration: 0.3, delay: 0.1)
///     end
///   end
/// @example # Toast notification slides in from bottom
///   class Toast
///     def show(message)
///       @message = message
///       @y = GMR::Window.height  # Start below screen
///       @target_y = GMR::Window.height - 60
///       GMR::Tween.from(self, :y, GMR::Window.height, duration: 0.25, ease: :out_cubic)
///       # Auto-hide after 2 seconds
///       GMR::Tween.to(self, :y, GMR::Window.height, duration: 0.2, delay: 2.0, ease: :in_quad)
///     end
///   end
static mrb_value mrb_tween_from(mrb_state* mrb, mrb_value) {
    mrb_value target;
    mrb_sym property_sym;
    mrb_float start_value;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "onf|H", &target, &property_sym, &start_value, &kwargs);

    const char* property = mrb_sym_name(mrb, property_sym);

    // Parse options
    TweenOptions opts = parse_tween_options(mrb, kwargs);
    if (!opts.has_duration) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "duration: is required");
        return mrb_nil_value();
    }

    auto& manager = animation::AnimationManager::instance();

    // Auto-cancel any existing tween for this target+property
    manager.cancel_tweens_for_property(mrb, target, property);

    // Create tween
    TweenHandle handle = manager.create_tween();
    animation::TweenState* tween = manager.get_tween(handle);

    // Get current value as end, use provided start
    float end_value = manager.get_property_value(mrb, target, property);

    // Set property to start value immediately
    manager.set_property_value(mrb, target, property, static_cast<float>(start_value));

    // Initialize tween state
    tween->target = target;
    tween->property = property;
    tween->start_value = static_cast<float>(start_value);
    tween->end_value = end_value;
    tween->duration = opts.duration;
    tween->delay = opts.delay;
    tween->delay_remaining = opts.delay;
    tween->easing = opts.easing;
    tween->active = true;

    // Create Ruby object
    mrb_value tween_obj = create_tween_object(mrb, handle);

    // Store Ruby object reference for GC registration
    tween->ruby_tween_obj = tween_obj;
    mrb_gc_register(mrb, tween_obj);

    // Store target as instance variable to prevent GC
    mrb_iv_set(mrb, tween_obj, mrb_intern_cstr(mrb, "@target"), target);

    return tween_obj;
}

// ============================================================================
// Instance Methods
// ============================================================================

/// @method on_complete
/// @description Set a callback to invoke when the tween completes.
/// @returns [Tween] self for chaining
/// @example # Sequence of tweens using on_complete for chaining
///   class Chest
///     def open
///       # First: lid opens
///       GMR::Tween.to(@lid, :rotation, -1.5, duration: 0.3, ease: :out_quad)
///         .on_complete do
///           # Second: spawn item and make it rise
///           @item.visible = true
///           @item.y = @lid.y
///           GMR::Tween.to(@item, :y, @item.y - 50, duration: 0.5, ease: :out_quad)
///             .on_complete do
///               # Third: play sparkle sound and enable pickup
///               GMR::Audio::Sound.play("assets/sparkle.wav")
///               @item_ready = true
///             end
///         end
///     end
///   end
static mrb_value mrb_tween_on_complete(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    TweenData* data = get_tween_data(mrb, self);
    if (!data) return self;

    animation::TweenState* tween = animation::AnimationManager::instance().get_tween(data->handle);
    if (!tween) return self;

    tween->on_complete = block;

    // Store block as instance variable to prevent GC
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@on_complete"), block);

    return self;
}

/// @method on_update
/// @description Set a callback to invoke each frame during the tween.
///   The callback receives (t, value) where t is normalized progress [0-1]
///   and value is the current interpolated value.
/// @returns [Tween] self for chaining
/// @example # Loading bar with progress callback
///   class LoadingScreen
///     def initialize
///       @progress = 0.0
///       @bar_width = 0
///     end
///
///     def start_loading
///       GMR::Tween.to(self, :progress, 1.0, duration: 2.0, ease: :linear)
///         .on_update { |t, val| @bar_width = (val * 400).to_i }
///         .on_complete { SceneManager.load(GameScene.new) }
///     end
///
///     def draw
///       GMR::Graphics.draw_rect(100, 300, @bar_width, 30, [100, 200, 100])
///       GMR::Graphics.draw_rect_outline(100, 300, 400, 30, [200, 200, 200])
///       GMR::Graphics.draw_text("#{(@progress * 100).to_i}%", 280, 340, 20, [255, 255, 255])
///     end
///   end
/// @example # Color cycling effect on update
///   def start_rainbow_effect
///     @hue = 0.0
///     GMR::Tween.to(self, :hue, 360.0, duration: 3.0, ease: :linear)
///       .on_update do |t, hue|
///         r, g, b = hue_to_rgb(hue)
///         @sprite.color = [r, g, b]
///       end
///   end
static mrb_value mrb_tween_on_update(mrb_state* mrb, mrb_value self) {
    mrb_value block;
    mrb_get_args(mrb, "&", &block);

    TweenData* data = get_tween_data(mrb, self);
    if (!data) return self;

    animation::TweenState* tween = animation::AnimationManager::instance().get_tween(data->handle);
    if (!tween) return self;

    tween->on_update = block;

    // Store block as instance variable to prevent GC
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@on_update"), block);

    return self;
}

/// @method cancel
/// @description Cancel the tween immediately. Does not invoke on_complete.
/// @returns [nil]
/// @example # Cancel movement when player hits wall
///   class Player
///     def move_to(target_x, target_y)
///       @move_tween_x = GMR::Tween.to(@sprite, :x, target_x, duration: 0.5)
///       @move_tween_y = GMR::Tween.to(@sprite, :y, target_y, duration: 0.5)
///     end
///
///     def on_wall_hit
///       @move_tween_x&.cancel
///       @move_tween_y&.cancel
///       Camera2D.current.shake(strength: 3, duration: 0.1)
///     end
///   end
static mrb_value mrb_tween_cancel(mrb_state* mrb, mrb_value self) {
    TweenData* data = get_tween_data(mrb, self);
    if (!data) return mrb_nil_value();

    animation::AnimationManager::instance().cancel_tween(data->handle);
    return mrb_nil_value();
}

/// @method pause
/// @description Pause the tween. Use resume to continue.
/// @returns [Tween] self for chaining
/// @example # Pause all UI animations when game pauses
///   class PauseMenu
///     def show
///       @paused_tweens = []
///       # Store and pause all active UI tweens
///       @ui_elements.each do |element|
///         if element.tween&.active?
///           element.tween.pause
///           @paused_tweens << element.tween
///         end
///       end
///       GMR::Input.push_context(:pause_menu)
///     end
///
///     def hide
///       # Resume all paused tweens
///       @paused_tweens.each(&:resume)
///       @paused_tweens.clear
///       GMR::Input.pop_context
///     end
///   end
static mrb_value mrb_tween_pause(mrb_state* mrb, mrb_value self) {
    TweenData* data = get_tween_data(mrb, self);
    if (!data) return self;

    animation::AnimationManager::instance().pause_tween(data->handle);
    return self;
}

/// @method resume
/// @description Resume a paused tween.
/// @returns [Tween] self for chaining
/// @example tween.resume
static mrb_value mrb_tween_resume(mrb_state* mrb, mrb_value self) {
    TweenData* data = get_tween_data(mrb, self);
    if (!data) return self;

    animation::AnimationManager::instance().resume_tween(data->handle);
    return self;
}

/// @method complete?
/// @description Check if the tween has finished.
/// @returns [Boolean] true if completed
/// @example if tween.complete?
///   puts "Done!"
/// end
static mrb_value mrb_tween_complete_p(mrb_state* mrb, mrb_value self) {
    TweenData* data = get_tween_data(mrb, self);
    if (!data) return mrb_true_value();

    animation::TweenState* tween = animation::AnimationManager::instance().get_tween(data->handle);
    if (!tween) return mrb_true_value();

    return mrb_bool_value(tween->completed);
}

/// @method active?
/// @description Check if the tween is currently running (not paused, cancelled, or complete).
/// @returns [Boolean] true if active
/// @example if tween.active?
///   puts "Still animating..."
/// end
static mrb_value mrb_tween_active_p(mrb_state* mrb, mrb_value self) {
    TweenData* data = get_tween_data(mrb, self);
    if (!data) return mrb_false_value();

    animation::TweenState* tween = animation::AnimationManager::instance().get_tween(data->handle);
    if (!tween) return mrb_false_value();

    return mrb_bool_value(tween->should_update());
}

/// @method paused?
/// @description Check if the tween is paused.
/// @returns [Boolean] true if paused
static mrb_value mrb_tween_paused_p(mrb_state* mrb, mrb_value self) {
    TweenData* data = get_tween_data(mrb, self);
    if (!data) return mrb_false_value();

    animation::TweenState* tween = animation::AnimationManager::instance().get_tween(data->handle);
    if (!tween) return mrb_false_value();

    return mrb_bool_value(tween->paused);
}

/// @method progress
/// @description Get the current progress of the tween (0.0 to 1.0).
/// @returns [Float] Progress value
static mrb_value mrb_tween_progress(mrb_state* mrb, mrb_value self) {
    TweenData* data = get_tween_data(mrb, self);
    if (!data) return mrb_float_value(mrb, 1.0);

    animation::TweenState* tween = animation::AnimationManager::instance().get_tween(data->handle);
    if (!tween) return mrb_float_value(mrb, 1.0);

    float t = tween->duration > 0.0f
        ? tween->elapsed / tween->duration
        : 1.0f;
    return mrb_float_value(mrb, t > 1.0f ? 1.0f : t);
}

// ============================================================================
// Class Methods
// ============================================================================

/// @method cancel_all
/// @description Cancel all active tweens.
/// @returns [nil]
/// @example # Clean slate when transitioning scenes
///   class GameScene < GMR::Scene
///     def unload
///       # Cancel all tweens to prevent callbacks on destroyed objects
///       GMR::Tween.cancel_all
///       # Clean up resources
///       @player = nil
///       @enemies.clear
///     end
///   end
static mrb_value mrb_tween_cancel_all(mrb_state* mrb, mrb_value) {
    animation::AnimationManager::instance().clear(mrb);
    return mrb_nil_value();
}

/// @method count
/// @description Get the number of active tweens.
/// @returns [Integer] Number of active tweens
/// @example puts "Active tweens: #{GMR::Tween.count}"
static mrb_value mrb_tween_count(mrb_state* mrb, mrb_value) {
    return mrb_fixnum_value(animation::AnimationManager::instance().tween_count());
}

// ============================================================================
// Registration
// ============================================================================

void register_tween(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* animation = mrb_module_get_under(mrb, gmr, "Animation");
    RClass* tween_class = mrb_define_class_under(mrb, animation, "Tween", mrb->object_class);
    MRB_SET_INSTANCE_TT(tween_class, MRB_TT_CDATA);

    // Class methods
    mrb_define_class_method(mrb, tween_class, "to", mrb_tween_to, MRB_ARGS_ARG(3, 1));
    mrb_define_class_method(mrb, tween_class, "from", mrb_tween_from, MRB_ARGS_ARG(3, 1));
    mrb_define_class_method(mrb, tween_class, "cancel_all", mrb_tween_cancel_all, MRB_ARGS_NONE());
    mrb_define_class_method(mrb, tween_class, "count", mrb_tween_count, MRB_ARGS_NONE());

    // Instance methods
    mrb_define_method(mrb, tween_class, "on_complete", mrb_tween_on_complete, MRB_ARGS_BLOCK());
    mrb_define_method(mrb, tween_class, "on_update", mrb_tween_on_update, MRB_ARGS_BLOCK());
    mrb_define_method(mrb, tween_class, "cancel", mrb_tween_cancel, MRB_ARGS_NONE());
    mrb_define_method(mrb, tween_class, "pause", mrb_tween_pause, MRB_ARGS_NONE());
    mrb_define_method(mrb, tween_class, "resume", mrb_tween_resume, MRB_ARGS_NONE());
    mrb_define_method(mrb, tween_class, "complete?", mrb_tween_complete_p, MRB_ARGS_NONE());
    mrb_define_method(mrb, tween_class, "active?", mrb_tween_active_p, MRB_ARGS_NONE());
    mrb_define_method(mrb, tween_class, "paused?", mrb_tween_paused_p, MRB_ARGS_NONE());
    mrb_define_method(mrb, tween_class, "progress", mrb_tween_progress, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
