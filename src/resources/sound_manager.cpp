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

} // namespace gmr
