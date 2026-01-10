#ifndef GMR_MUSIC_MANAGER_HPP
#define GMR_MUSIC_MANAGER_HPP

#include "raylib.h"
#include <string>

namespace gmr {

/// MusicManager provides singleton music streaming support
/// Unlike sounds, music is streamed from disk and requires per-frame updates
/// Only one music stream can be active at a time (Raylib design)
class MusicManager {
public:
    static MusicManager& instance();

    // Core lifecycle
    bool load(const std::string& path);
    void unload();
    bool is_loaded() const { return state_.is_valid; }

    // Playback control
    void play();
    void stop();
    void pause();
    void resume();
    bool is_playing() const { return state_.is_playing; }

    // Properties - setters
    void set_volume(float volume);
    void set_pitch(float pitch);
    void set_pan(float pan);
    void set_looping(bool loop);

    // Properties - getters
    float get_volume() const { return state_.volume; }
    float get_pitch() const { return state_.pitch; }
    float get_pan() const { return state_.pan; }
    bool is_looping() const { return state_.loop; }

    // Time control (music-specific features)
    void seek(float position_seconds);
    float get_time_length() const;
    float get_time_played() const;

    // Per-frame update - CRITICAL: must be called every frame for streaming
    void update();

    // Cleanup
    void clear();

    MusicManager(const MusicManager&) = delete;
    MusicManager& operator=(const MusicManager&) = delete;

private:
    MusicManager() = default;

    struct State {
        Music stream;           // Raylib Music stream
        std::string path;       // Path for debugging/reference
        bool is_valid = false;  // Whether stream is loaded
        bool is_playing = false;// Playing state
        float volume = 1.0f;    // Current volume (0.0-1.0)
        float pitch = 1.0f;     // Current pitch (1.0 = normal)
        float pan = 0.5f;       // Stereo pan (0.0 = left, 0.5 = center, 1.0 = right)
        bool loop = false;      // Loop enabled
    } state_;
};

} // namespace gmr

#endif
