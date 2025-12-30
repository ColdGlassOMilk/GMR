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
    void set_volume(SoundHandle handle, float volume);
    
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
