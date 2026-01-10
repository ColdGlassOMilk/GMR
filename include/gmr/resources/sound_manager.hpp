#ifndef GMR_SOUND_MANAGER_HPP
#define GMR_SOUND_MANAGER_HPP

#include "gmr/resources/resource_manager.hpp"
#include "raylib.h"

namespace gmr {

class SoundManager : public ResourceManager<SoundHandle, Sound> {
public:
    static SoundManager& instance();
    
    void play(SoundHandle handle);
    void stop(SoundHandle handle);
    void pause(SoundHandle handle);
    void resume(SoundHandle handle);
    bool is_playing(SoundHandle handle);

    void set_volume(SoundHandle handle, float volume);
    void set_pitch(SoundHandle handle, float pitch);
    void set_pan(SoundHandle handle, float pan);

    float get_volume(SoundHandle handle);
    float get_pitch(SoundHandle handle);
    float get_pan(SoundHandle handle);
    
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    
protected:
    std::optional<Sound> load_resource(const std::string& path) override;
    void unload_resource(Sound& sound) override;
    
private:
    SoundManager() = default;
};

} // namespace gmr

#endif
