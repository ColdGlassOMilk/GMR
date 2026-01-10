#include "gmr/bindings/audio.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/resources/sound_manager.hpp"
#include "gmr/resources/music_manager.hpp"

namespace gmr {
namespace bindings {

/// @module GMR::Audio
/// @description Audio playback module. Load and play sound effects and music.
/// @example # Complete audio management for a game
///   class AudioManager
///     def initialize
///       # Preload commonly used sound effects
///       @sounds = {
///         jump: GMR::Audio::Sound.load("assets/sfx/jump.wav"),
///         land: GMR::Audio::Sound.load("assets/sfx/land.wav"),
///         coin: GMR::Audio::Sound.load("assets/sfx/coin.wav"),
///         hurt: GMR::Audio::Sound.load("assets/sfx/hurt.wav"),
///         explosion: GMR::Audio::Sound.load("assets/sfx/explosion.wav")
///       }
///       @music_volume = 0.7
///       @sfx_volume = 1.0
///     end
///
///     def play_sfx(name, volume: nil)
///       sound = @sounds[name]
///       return unless sound
///       sound.volume = (volume || @sfx_volume)
///       sound.play
///     end
///
///     def play_music(path, loop: true)
///       GMR::Audio.stop_music
///       GMR::Audio.play_music(path, volume: @music_volume, loop: loop)
///     end
///
///     def fade_out_music(duration)
///       # Gradually reduce music volume
///       GMR::Tween.to(self, :music_volume, 0.0, duration: duration)
///         .on_update { |t, vol| GMR::Audio.set_music_volume(vol) }
///         .on_complete { GMR::Audio.stop_music }
///     end
///   end
///
///   # Usage in game
///   def init
///     $audio = AudioManager.new
///     $audio.play_music("assets/music/level1.ogg")
///   end
///
///   def player_jump
///     $audio.play_sfx(:jump)
///   end

/// @class GMR::Audio::Sound
/// @description A loaded sound that can be played, stopped, and have its volume adjusted.
///   Sounds are loaded once and can be played multiple times. Use for sound effects.
/// @example # Player with contextual footstep sounds
///   class Player
///     def initialize
///       @footstep_sounds = {
///         grass: GMR::Audio::Sound.load("assets/sfx/step_grass.wav"),
///         stone: GMR::Audio::Sound.load("assets/sfx/step_stone.wav"),
///         wood: GMR::Audio::Sound.load("assets/sfx/step_wood.wav")
///       }
///       @footstep_timer = 0
///     end
///
///     def update(dt)
///       if moving? && @on_ground
///         @footstep_timer -= dt
///         if @footstep_timer <= 0
///           play_footstep
///           @footstep_timer = @running ? 0.25 : 0.4
///         end
///       end
///     end
///
///     def play_footstep
///       surface = detect_surface_type
///       sound = @footstep_sounds[surface] || @footstep_sounds[:stone]
///       sound.volume = GMR::Math.random(0.3, 0.5)  # Vary volume slightly
///       sound.play
///     end
///   end
/// @example # Enemy with death sound and state machine integration
///   class Enemy
///     def initialize
///       @sounds = {
///         alert: GMR::Audio::Sound.load("assets/sfx/enemy_alert.wav"),
///         attack: GMR::Audio::Sound.load("assets/sfx/enemy_attack.wav"),
///         death: GMR::Audio::Sound.load("assets/sfx/enemy_death.wav")
///       }
///
///       state_machine do
///         state :patrol do
///           on :see_player, :chase
///         end
///         state :chase do
///           enter { @sounds[:alert].play }
///           on :in_range, :attack
///         end
///         state :attack do
///           enter { @sounds[:attack].play }
///         end
///         state :dead do
///           enter { @sounds[:death].play }
///         end
///       end
///     end
///   end

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
/// @param options [Hash] Optional configuration
/// @option options :volume [Float] Initial volume (0.0-1.0, default: 1.0)
/// @option options :pitch [Float] Initial pitch (default: 1.0)
/// @option options :pan [Float] Initial stereo pan (0.0-1.0, default: 0.5)
/// @returns [Sound] The loaded sound object
/// @raises [RuntimeError] If the file cannot be loaded
/// @example jump_sound = GMR::Audio::Sound.load("assets/sfx/jump.wav", volume: 0.7, pitch: 1.2)
static mrb_value mrb_sound_load(mrb_state* mrb, mrb_value klass) {
    const char* path;
    mrb_value opts = mrb_nil_value();
    mrb_get_args(mrb, "z|H", &path, &opts);

    SoundHandle handle = SoundManager::instance().load(path);
    if (handle == INVALID_HANDLE) {
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to load sound: %s", path);
        return mrb_nil_value();
    }

    // Apply optional settings
    if (!mrb_nil_p(opts)) {
        mrb_value vol_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "volume")));
        if (!mrb_nil_p(vol_val)) {
            mrb_float volume = mrb_float(mrb_to_float(mrb, vol_val));
            SoundManager::instance().set_volume(handle, static_cast<float>(volume));
        }

        mrb_value pitch_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "pitch")));
        if (!mrb_nil_p(pitch_val)) {
            mrb_float pitch = mrb_float(mrb_to_float(mrb, pitch_val));
            SoundManager::instance().set_pitch(handle, static_cast<float>(pitch));
        }

        mrb_value pan_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "pan")));
        if (!mrb_nil_p(pan_val)) {
            mrb_float pan = mrb_float(mrb_to_float(mrb, pan_val));
            SoundManager::instance().set_pan(handle, static_cast<float>(pan));
        }
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

/// @method pause
/// @description Pause the sound if it's currently playing.
/// @returns [nil]
/// @example sound.pause
static mrb_value mrb_sound_pause(mrb_state* mrb, mrb_value self) {
    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        SoundManager::instance().pause(data->handle);
    }
    return mrb_nil_value();
}

/// @method resume
/// @description Resume the sound if it was paused.
/// @returns [nil]
/// @example sound.resume
static mrb_value mrb_sound_resume(mrb_state* mrb, mrb_value self) {
    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        SoundManager::instance().resume(data->handle);
    }
    return mrb_nil_value();
}

/// @method playing?
/// @description Check if the sound is currently playing.
/// @returns [Boolean] true if the sound is playing, false otherwise
/// @example if sound.playing? then puts "Playing!" end
static mrb_value mrb_sound_is_playing(mrb_state* mrb, mrb_value self) {
    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        bool playing = SoundManager::instance().is_playing(data->handle);
        return mrb_bool_value(playing);
    }
    return mrb_false_value();
}

/// @method pitch=
/// @description Set the playback pitch for this sound.
/// @param value [Float] Pitch multiplier (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
/// @returns [Float] The pitch that was set
/// @example sound.pitch = 1.5  # Play 50% faster
static mrb_value mrb_sound_set_pitch(mrb_state* mrb, mrb_value self) {
    mrb_float pitch;
    mrb_get_args(mrb, "f", &pitch);

    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        SoundManager::instance().set_pitch(data->handle, static_cast<float>(pitch));
    }
    return mrb_float_value(mrb, pitch);
}

/// @method pan=
/// @description Set the stereo pan for this sound.
/// @param value [Float] Pan value from 0.0 (left) to 1.0 (right), 0.5 is center
/// @returns [Float] The pan value that was set
/// @example sound.pan = 0.0  # Full left
static mrb_value mrb_sound_set_pan(mrb_state* mrb, mrb_value self) {
    mrb_float pan;
    mrb_get_args(mrb, "f", &pan);

    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        SoundManager::instance().set_pan(data->handle, static_cast<float>(pan));
    }
    return mrb_float_value(mrb, pan);
}

/// @method volume
/// @description Get the current volume level for this sound.
/// @returns [Float] Current volume (0.0-1.0)
/// @example vol = sound.volume
static mrb_value mrb_sound_get_volume(mrb_state* mrb, mrb_value self) {
    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        float volume = SoundManager::instance().get_volume(data->handle);
        return mrb_float_value(mrb, volume);
    }
    return mrb_float_value(mrb, 1.0);
}

/// @method pitch
/// @description Get the current pitch for this sound.
/// @returns [Float] Current pitch multiplier
/// @example p = sound.pitch
static mrb_value mrb_sound_get_pitch(mrb_state* mrb, mrb_value self) {
    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        float pitch = SoundManager::instance().get_pitch(data->handle);
        return mrb_float_value(mrb, pitch);
    }
    return mrb_float_value(mrb, 1.0);
}

/// @method pan
/// @description Get the current stereo pan for this sound.
/// @returns [Float] Current pan value (0.0-1.0)
/// @example p = sound.pan
static mrb_value mrb_sound_get_pan(mrb_state* mrb, mrb_value self) {
    SoundData* data = get_sound_data(mrb, self);
    if (data) {
        float pan = SoundManager::instance().get_pan(data->handle);
        return mrb_float_value(mrb, pan);
    }
    return mrb_float_value(mrb, 0.5);
}

// ============================================================================
// GMR::Audio::Music Class
// ============================================================================

/// @class GMR::Audio::Music
/// @description A streaming music player for longer audio tracks. Music is streamed
///   from disk rather than loaded entirely into memory, making it suitable for
///   background music and longer audio files.
/// @example # Basic music playback
///   class MenuScene
///     def init
///       @music = GMR::Audio::Music.load("assets/music/menu.ogg")
///       @music.loop = true
///       @music.volume = 0.6
///       @music.play
///     end
///
///     def cleanup
///       @music.stop
///     end
///   end
/// @example # Dynamic music transitions
///   class GameMusic
///     def initialize
///       @explore = GMR::Audio::Music.load("assets/music/explore.ogg")
///       @combat = GMR::Audio::Music.load("assets/music/combat.ogg")
///       @current = nil
///     end
///
///     def switch_to(track)
///       @current&.stop
///       @current = instance_variable_get("@#{track}")
///       @current.loop = true
///       @current.play
///     end
///   end

// Music data structure - just a marker since MusicManager is singleton
struct MusicData {
    // Empty - MusicManager handles all state
    // This is just to mark Ruby objects as Music instances
};

static void music_free(mrb_state* mrb, void* ptr) {
    // MusicManager is singleton - nothing to free here
    // Just free the wrapper struct
    mrb_free(mrb, ptr);
}

static const mrb_data_type music_data_type = {
    "GMR::Audio::Music", music_free
};

/// @classmethod load
/// @description Load a music file for streaming. Supports OGG, MP3, WAV, and other formats.
/// @param path [String] Path to the music file
/// @param opts [Hash] Optional hash with volume:, pitch:, pan:, and/or loop: keys
/// @returns [Music] The loaded music object
/// @raises [RuntimeError] If the file cannot be loaded
/// @example music = GMR::Audio::Music.load("assets/music/level1.ogg")
/// @example music = GMR::Audio::Music.load("assets/music/level1.ogg", volume: 0.7, loop: true)
static mrb_value mrb_music_load(mrb_state* mrb, mrb_value klass) {
    const char* path;
    mrb_value opts = mrb_nil_value();
    mrb_get_args(mrb, "z|H", &path, &opts);

    if (!MusicManager::instance().load(path)) {
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to load music: %s", path);
        return mrb_nil_value();
    }

    // Apply optional settings from hash
    if (!mrb_nil_p(opts)) {
        // Volume
        mrb_value vol_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "volume")));
        if (!mrb_nil_p(vol_val)) {
            mrb_float volume = mrb_float(mrb_to_float(mrb, vol_val));
            MusicManager::instance().set_volume(static_cast<float>(volume));
        }

        // Pitch
        mrb_value pitch_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "pitch")));
        if (!mrb_nil_p(pitch_val)) {
            mrb_float pitch = mrb_float(mrb_to_float(mrb, pitch_val));
            MusicManager::instance().set_pitch(static_cast<float>(pitch));
        }

        // Pan
        mrb_value pan_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "pan")));
        if (!mrb_nil_p(pan_val)) {
            mrb_float pan = mrb_float(mrb_to_float(mrb, pan_val));
            MusicManager::instance().set_pan(static_cast<float>(pan));
        }

        // Loop
        mrb_value loop_val = mrb_hash_get(mrb, opts, mrb_symbol_value(mrb_intern_cstr(mrb, "loop")));
        if (!mrb_nil_p(loop_val)) {
            bool loop = mrb_test(loop_val);
            MusicManager::instance().set_looping(loop);
        }
    }

    // Create new Music instance
    RClass* music_class = mrb_class_ptr(klass);
    mrb_value obj = mrb_obj_new(mrb, music_class, 0, nullptr);

    // Allocate and set data (empty marker)
    MusicData* data = static_cast<MusicData*>(mrb_malloc(mrb, sizeof(MusicData)));
    mrb_data_init(obj, data, &music_data_type);

    return obj;
}

/// @method play
/// @description Start playing the music from the current position.
/// @returns [nil]
/// @example music.play
static mrb_value mrb_music_play(mrb_state* mrb, mrb_value self) {
    MusicManager::instance().play();
    return mrb_nil_value();
}

/// @method stop
/// @description Stop playing the music and reset position to beginning.
/// @returns [nil]
/// @example music.stop
static mrb_value mrb_music_stop(mrb_state* mrb, mrb_value self) {
    MusicManager::instance().stop();
    return mrb_nil_value();
}

/// @method pause
/// @description Pause the music at the current position.
/// @returns [nil]
/// @example music.pause
static mrb_value mrb_music_pause(mrb_state* mrb, mrb_value self) {
    MusicManager::instance().pause();
    return mrb_nil_value();
}

/// @method resume
/// @description Resume playing paused music.
/// @returns [nil]
/// @example music.resume
static mrb_value mrb_music_resume(mrb_state* mrb, mrb_value self) {
    MusicManager::instance().resume();
    return mrb_nil_value();
}

/// @method playing?
/// @description Check if the music is currently playing.
/// @returns [Boolean] true if playing, false otherwise
/// @example if music.playing? then puts "Music is playing!" end
static mrb_value mrb_music_is_playing(mrb_state* mrb, mrb_value self) {
    bool playing = MusicManager::instance().is_playing();
    return mrb_bool_value(playing);
}

/// @method loaded?
/// @description Check if valid music is loaded.
/// @returns [Boolean] true if music is loaded, false otherwise
/// @example if music.loaded? then music.play end
static mrb_value mrb_music_is_loaded(mrb_state* mrb, mrb_value self) {
    bool loaded = MusicManager::instance().is_loaded();
    return mrb_bool_value(loaded);
}

/// @method volume=
/// @description Set the playback volume for the music.
/// @param value [Float] Volume level from 0.0 (silent) to 1.0 (full volume)
/// @returns [Float] The volume that was set
/// @example music.volume = 0.7
static mrb_value mrb_music_set_volume(mrb_state* mrb, mrb_value self) {
    mrb_float volume;
    mrb_get_args(mrb, "f", &volume);

    MusicManager::instance().set_volume(static_cast<float>(volume));
    return mrb_float_value(mrb, volume);
}

/// @method volume
/// @description Get the current volume level.
/// @returns [Float] Current volume (0.0-1.0)
/// @example vol = music.volume
static mrb_value mrb_music_get_volume(mrb_state* mrb, mrb_value self) {
    float volume = MusicManager::instance().get_volume();
    return mrb_float_value(mrb, volume);
}

/// @method pitch=
/// @description Set the playback pitch for the music.
/// @param value [Float] Pitch multiplier (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
/// @returns [Float] The pitch that was set
/// @example music.pitch = 0.8  # Slower playback
static mrb_value mrb_music_set_pitch(mrb_state* mrb, mrb_value self) {
    mrb_float pitch;
    mrb_get_args(mrb, "f", &pitch);

    MusicManager::instance().set_pitch(static_cast<float>(pitch));
    return mrb_float_value(mrb, pitch);
}

/// @method pitch
/// @description Get the current pitch.
/// @returns [Float] Current pitch multiplier
/// @example p = music.pitch
static mrb_value mrb_music_get_pitch(mrb_state* mrb, mrb_value self) {
    float pitch = MusicManager::instance().get_pitch();
    return mrb_float_value(mrb, pitch);
}

/// @method pan=
/// @description Set the stereo pan for the music.
/// @param value [Float] Pan value from 0.0 (left) to 1.0 (right), 0.5 is center
/// @returns [Float] The pan value that was set
/// @example music.pan = 0.3  # Slightly left
static mrb_value mrb_music_set_pan(mrb_state* mrb, mrb_value self) {
    mrb_float pan;
    mrb_get_args(mrb, "f", &pan);

    MusicManager::instance().set_pan(static_cast<float>(pan));
    return mrb_float_value(mrb, pan);
}

/// @method pan
/// @description Get the current stereo pan.
/// @returns [Float] Current pan value (0.0-1.0)
/// @example p = music.pan
static mrb_value mrb_music_get_pan(mrb_state* mrb, mrb_value self) {
    float pan = MusicManager::instance().get_pan();
    return mrb_float_value(mrb, pan);
}

/// @method loop=
/// @description Enable or disable music looping.
/// @param value [Boolean] true to enable looping, false to disable
/// @returns [Boolean] The loop value that was set
/// @example music.loop = true
static mrb_value mrb_music_set_loop(mrb_state* mrb, mrb_value self) {
    mrb_bool loop;
    mrb_get_args(mrb, "b", &loop);

    MusicManager::instance().set_looping(loop);
    return mrb_bool_value(loop);
}

/// @method loop
/// @description Check if looping is enabled.
/// @returns [Boolean] true if looping, false otherwise
/// @example if music.loop then puts "Looping!" end
static mrb_value mrb_music_get_loop(mrb_state* mrb, mrb_value self) {
    bool loop = MusicManager::instance().is_looping();
    return mrb_bool_value(loop);
}

/// @method seek
/// @description Seek to a specific position in the music.
/// @param position [Float] Position in seconds
/// @returns [nil]
/// @example music.seek(30.0)  # Jump to 30 seconds
static mrb_value mrb_music_seek(mrb_state* mrb, mrb_value self) {
    mrb_float position;
    mrb_get_args(mrb, "f", &position);

    MusicManager::instance().seek(static_cast<float>(position));
    return mrb_nil_value();
}

/// @method length
/// @description Get the total length of the music in seconds.
/// @returns [Float] Total length in seconds
/// @example puts "Music is #{music.length} seconds long"
static mrb_value mrb_music_get_length(mrb_state* mrb, mrb_value self) {
    float length = MusicManager::instance().get_time_length();
    return mrb_float_value(mrb, length);
}

/// @method position
/// @description Get the current playback position in seconds.
/// @returns [Float] Current position in seconds
/// @example puts "At #{music.position} / #{music.length}"
static mrb_value mrb_music_get_position(mrb_state* mrb, mrb_value self) {
    float position = MusicManager::instance().get_time_played();
    return mrb_float_value(mrb, position);
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

    // Instance methods - playback control
    mrb_define_method(mrb, sound_class, "play", mrb_sound_play, MRB_ARGS_NONE());
    mrb_define_method(mrb, sound_class, "stop", mrb_sound_stop, MRB_ARGS_NONE());
    mrb_define_method(mrb, sound_class, "pause", mrb_sound_pause, MRB_ARGS_NONE());
    mrb_define_method(mrb, sound_class, "resume", mrb_sound_resume, MRB_ARGS_NONE());

    // State queries
    mrb_define_method(mrb, sound_class, "playing?", mrb_sound_is_playing, MRB_ARGS_NONE());

    // Property setters
    mrb_define_method(mrb, sound_class, "volume=", mrb_sound_set_volume, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sound_class, "pitch=", mrb_sound_set_pitch, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, sound_class, "pan=", mrb_sound_set_pan, MRB_ARGS_REQ(1));

    // Property getters
    mrb_define_method(mrb, sound_class, "volume", mrb_sound_get_volume, MRB_ARGS_NONE());
    mrb_define_method(mrb, sound_class, "pitch", mrb_sound_get_pitch, MRB_ARGS_NONE());
    mrb_define_method(mrb, sound_class, "pan", mrb_sound_get_pan, MRB_ARGS_NONE());

    // Music class
    RClass* music_class = mrb_define_class_under(mrb, audio, "Music", mrb->object_class);
    MRB_SET_INSTANCE_TT(music_class, MRB_TT_CDATA);

    // Class method
    mrb_define_class_method(mrb, music_class, "load", mrb_music_load, MRB_ARGS_REQ(1));

    // Instance methods - playback control
    mrb_define_method(mrb, music_class, "play", mrb_music_play, MRB_ARGS_NONE());
    mrb_define_method(mrb, music_class, "stop", mrb_music_stop, MRB_ARGS_NONE());
    mrb_define_method(mrb, music_class, "pause", mrb_music_pause, MRB_ARGS_NONE());
    mrb_define_method(mrb, music_class, "resume", mrb_music_resume, MRB_ARGS_NONE());

    // State queries
    mrb_define_method(mrb, music_class, "playing?", mrb_music_is_playing, MRB_ARGS_NONE());
    mrb_define_method(mrb, music_class, "loaded?", mrb_music_is_loaded, MRB_ARGS_NONE());

    // Property setters
    mrb_define_method(mrb, music_class, "volume=", mrb_music_set_volume, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, music_class, "pitch=", mrb_music_set_pitch, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, music_class, "pan=", mrb_music_set_pan, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, music_class, "loop=", mrb_music_set_loop, MRB_ARGS_REQ(1));

    // Property getters
    mrb_define_method(mrb, music_class, "volume", mrb_music_get_volume, MRB_ARGS_NONE());
    mrb_define_method(mrb, music_class, "pitch", mrb_music_get_pitch, MRB_ARGS_NONE());
    mrb_define_method(mrb, music_class, "pan", mrb_music_get_pan, MRB_ARGS_NONE());
    mrb_define_method(mrb, music_class, "loop", mrb_music_get_loop, MRB_ARGS_NONE());
    // Ruby-idiomatic predicate alias (boolean method should have ? suffix)
    mrb_define_method(mrb, music_class, "loop?", mrb_music_get_loop, MRB_ARGS_NONE());

    // Time control
    mrb_define_method(mrb, music_class, "seek", mrb_music_seek, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, music_class, "length", mrb_music_get_length, MRB_ARGS_NONE());
    mrb_define_method(mrb, music_class, "position", mrb_music_get_position, MRB_ARGS_NONE());
}

} // namespace bindings
} // namespace gmr
