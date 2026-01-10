#include "gmr/resources/music_manager.hpp"
#include "gmr/paths.hpp"

namespace gmr {

MusicManager& MusicManager::instance() {
    static MusicManager instance;
    return instance;
}

bool MusicManager::load(const std::string& path) {
    // Stop and unload existing music if any
    if (state_.is_valid) {
        stop();
        UnloadMusicStream(state_.stream);
        state_.is_valid = false;
    }

    // Load new music stream
    std::string resolved = resolve_asset_path(path);
    Music music = LoadMusicStream(resolved.c_str());

    // Validate: Raylib sets ctxType to non-zero if music loaded successfully
    if (music.ctxType == 0) {
        state_.is_valid = false;
        return false;
    }

    // Store new music stream
    state_.stream = music;
    state_.path = path;
    state_.is_valid = true;
    state_.is_playing = false;

    // Apply stored properties to new stream
    SetMusicVolume(state_.stream, state_.volume);
    SetMusicPitch(state_.stream, state_.pitch);
    SetMusicPan(state_.stream, state_.pan);

    return true;
}

void MusicManager::unload() {
    if (state_.is_valid) {
        stop();
        UnloadMusicStream(state_.stream);
        state_.is_valid = false;
        state_.is_playing = false;
        state_.path.clear();
    }
}

void MusicManager::play() {
    if (!state_.is_valid) return;

    PlayMusicStream(state_.stream);
    state_.is_playing = true;
}

void MusicManager::stop() {
    if (!state_.is_valid) return;

    StopMusicStream(state_.stream);
    state_.is_playing = false;
}

void MusicManager::pause() {
    if (!state_.is_valid) return;

    PauseMusicStream(state_.stream);
    state_.is_playing = false;
}

void MusicManager::resume() {
    if (!state_.is_valid) return;

    ResumeMusicStream(state_.stream);
    state_.is_playing = true;
}

void MusicManager::set_volume(float volume) {
    state_.volume = volume;
    if (state_.is_valid) {
        SetMusicVolume(state_.stream, volume);
    }
}

void MusicManager::set_pitch(float pitch) {
    state_.pitch = pitch;
    if (state_.is_valid) {
        SetMusicPitch(state_.stream, pitch);
    }
}

void MusicManager::set_pan(float pan) {
    state_.pan = pan;
    if (state_.is_valid) {
        SetMusicPan(state_.stream, pan);
    }
}

void MusicManager::set_looping(bool loop) {
    state_.loop = loop;
    // Note: Raylib's music.looping field is read-only from our perspective
    // We handle looping manually in update()
}

void MusicManager::seek(float position_seconds) {
    if (!state_.is_valid) return;

    SeekMusicStream(state_.stream, position_seconds);
}

float MusicManager::get_time_length() const {
    if (!state_.is_valid) return 0.0f;

    return GetMusicTimeLength(state_.stream);
}

float MusicManager::get_time_played() const {
    if (!state_.is_valid) return 0.0f;

    return GetMusicTimePlayed(state_.stream);
}

void MusicManager::update() {
    if (!state_.is_valid || !state_.is_playing) {
        return;
    }

    // Update music streaming - CRITICAL for playback
    UpdateMusicStream(state_.stream);

    // Check if music has finished playing
    if (!IsMusicStreamPlaying(state_.stream)) {
        state_.is_playing = false;

        // Handle looping manually
        if (state_.loop) {
            PlayMusicStream(state_.stream);
            state_.is_playing = true;
        }
    }
}

void MusicManager::clear() {
    if (state_.is_valid) {
        stop();
        UnloadMusicStream(state_.stream);
    }

    // Reset state to defaults
    state_ = State{};
    state_.volume = 1.0f;
    state_.pitch = 1.0f;
    state_.pan = 0.5f;
    state_.loop = false;
}

} // namespace gmr
