#include "gmr/bindings/audio.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/resources/sound_manager.hpp"

namespace gmr {
namespace bindings {

/// @module GMR::Audio
/// @description Audio playback module. Load and play sound effects and music.

/// @class GMR::Audio::Sound
/// @description A loaded sound that can be played, stopped, and have its volume adjusted.
///   Sounds are loaded once and can be played multiple times. Use for sound effects.
/// @example sound = GMR::Audio::Sound.load("assets/jump.wav")
///   sound.play
/// @example # Adjust volume
///   music = GMR::Audio::Sound.load("assets/bgm.ogg")
///   music.volume = 0.5
///   music.play

// ============================================================================
// GMR::Audio::Sound Class
// ============================================================================

// Sound data structure - holds the handle
struct SoundData {
    SoundHandle handle;
};

// Data type for garbage collection
static void sound_free(mrb_state* mrb, void* ptr) {
    // Don't unload the sound - SoundManager owns it
    // Just free the wrapper struct
    mrb_free(mrb, ptr);
}

static const mrb_data_type sound_data_type = {
    "GMR::Audio::Sound", sound_free
};

// Helper to get SoundData from self
static SoundData* get_sound_data(mrb_state* mrb, mrb_value self) {
    return static_cast<SoundData*>(mrb_data_get_ptr(mrb, self, &sound_data_type));
}

/// @classmethod load
/// @description Load a sound file from disk. Supports WAV, OGG, MP3, and other formats.
/// @param path [String] Path to the audio file
/// @returns [Sound] The loaded sound object
/// @raises [RuntimeError] If the file cannot be loaded
/// @example jump_sound = GMR::Audio::Sound.load("assets/sfx/jump.wav")
static mrb_value mrb_sound_load(mrb_state* mrb, mrb_value klass) {
    const char* path;
    mrb_get_args(mrb, "z", &path);

    SoundHandle handle = SoundManager::instance().load(path);
    if (handle == INVALID_HANDLE) {
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to load sound: %s", path);
        return mrb_nil_value();
    }

    // Create new instance
    RClass* sound_class = mrb_class_ptr(klass);
    mrb_value obj = mrb_obj_new(mrb, sound_class, 0, nullptr);

    // Allocate and set data
    SoundData* data = static_cast<SoundData*>(mrb_malloc(mrb, sizeof(SoundData)));
    data->handle = handle;
    mrb_data_init(obj, data, &sound_data_type);

    return obj;
}

/// @method play
/// @description Play the sound. Can be called multiple times for overlapping playback.
/// @returns [nil]
/// @example sound.play
static mrb_value mrb_sound_play(mrb_state* mrb, mrb_value self) {
    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        SoundManager::instance().play(data->handle);
    }
    return mrb_nil_value();
}

/// @method stop
/// @description Stop the sound if it's currently playing.
/// @returns [nil]
/// @example sound.stop
static mrb_value mrb_sound_stop(mrb_state* mrb, mrb_value self) {
    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        SoundManager::instance().stop(data->handle);
    }
    return mrb_nil_value();
}

/// @method volume=
/// @description Set the playback volume for this sound.
/// @param value [Float] Volume level from 0.0 (silent) to 1.0 (full volume)
/// @returns [Float] The volume that was set
/// @example sound.volume = 0.5  # Half volume
static mrb_value mrb_sound_set_volume(mrb_state* mrb, mrb_value self) {
    mrb_float volume;
    mrb_get_args(mrb, "f", &volume);

    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        SoundManager::instance().set_volume(data->handle, static_cast<float>(volume));
    }
    return mrb_float_value(mrb, volume);
}

// ============================================================================
// Registration
// ============================================================================

void register_audio(mrb_state* mrb) {
    RClass* audio = get_gmr_submodule(mrb, "Audio");

    // Sound class
    RClass* sound_class = mrb_define_class_under(mrb, audio, "Sound", mrb->object_class);
    MRB_SET_INSTANCE_TT(sound_class, MRB_TT_CDATA);

    // Class method
    mrb_define_class_method(mrb, sound_class, "load", mrb_sound_load, MRB_ARGS_REQ(1));

    // Instance methods
    mrb_define_method(mrb, sound_class, "play", mrb_sound_play, MRB_ARGS_NONE());
    mrb_define_method(mrb, sound_class, "stop", mrb_sound_stop, MRB_ARGS_NONE());
    mrb_define_method(mrb, sound_class, "volume=", mrb_sound_set_volume, MRB_ARGS_REQ(1));
}

} // namespace bindings
} // namespace gmr
