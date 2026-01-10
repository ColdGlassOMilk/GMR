#include "gmr/resources/sound_manager.hpp"
#include "gmr/paths.hpp"

namespace gmr {

SoundManager& SoundManager::instance() {
    static SoundManager instance;
    return instance;
}

std::optional<Sound> SoundManager::load_resource(const std::string& path) {
    std::string resolved = resolve_asset_path(path);
    Sound sound = LoadSound(resolved.c_str());
    if (sound.frameCount == 0) {
        return std::nullopt;
    }
    return sound;
}

void SoundManager::unload_resource(Sound& sound) {
    UnloadSound(sound);
}

void SoundManager::play(SoundHandle handle) {
    if (auto* sound = get(handle)) {
        PlaySound(*sound);
    }
}

void SoundManager::stop(SoundHandle handle) {
    if (auto* sound = get(handle)) {
        StopSound(*sound);
    }
}

void SoundManager::set_volume(SoundHandle handle, float volume) {
    if (auto* sound = get(handle)) {
        SetSoundVolume(*sound, volume);
    }
}

void SoundManager::pause(SoundHandle handle) {
    if (auto* sound = get(handle)) {
        PauseSound(*sound);
    }
}

void SoundManager::resume(SoundHandle handle) {
    if (auto* sound = get(handle)) {
        ResumeSound(*sound);
    }
}

bool SoundManager::is_playing(SoundHandle handle) {
    if (auto* sound = get(handle)) {
        return IsSoundPlaying(*sound);
    }
    return false;
}

void SoundManager::set_pitch(SoundHandle handle, float pitch) {
    if (auto* sound = get(handle)) {
        SetSoundPitch(*sound, pitch);
    }
}

void SoundManager::set_pan(SoundHandle handle, float pan) {
    if (auto* sound = get(handle)) {
        SetSoundPan(*sound, pan);
    }
}

float SoundManager::get_volume(SoundHandle handle) {
    // Raylib doesn't provide a GetSoundVolume function
    // Volume would need to be tracked separately if needed
    // For now, return 1.0 as default
    return 1.0f;
}

float SoundManager::get_pitch(SoundHandle handle) {
    // Raylib doesn't provide a GetSoundPitch function
    // Pitch would need to be tracked separately if needed
    return 1.0f;
}

float SoundManager::get_pan(SoundHandle handle) {
    // Raylib doesn't provide a GetSoundPan function
    // Pan would need to be tracked separately if needed
    return 0.5f;
}

} // namespace gmr
