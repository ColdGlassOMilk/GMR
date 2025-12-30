#include "gmr/bindings/audio.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/resources/sound_manager.hpp"

namespace gmr {
namespace bindings {

static mrb_value mrb_load_sound(mrb_state* mrb, mrb_value) {
    const char* path;
    mrb_get_args(mrb, "z", &path);
    
    SoundHandle handle = SoundManager::instance().load(path);
    if (handle == INVALID_HANDLE) {
        mrb_raisef(mrb, E_RUNTIME_ERROR, "Failed to load sound: %s", path);
    }
    return mrb_fixnum_value(handle);
}

static mrb_value mrb_play_sound(mrb_state* mrb, mrb_value) {
    mrb_int handle;
    mrb_get_args(mrb, "i", &handle);
    SoundManager::instance().play(static_cast<SoundHandle>(handle));
    return mrb_nil_value();
}

static mrb_value mrb_stop_sound(mrb_state* mrb, mrb_value) {
    mrb_int handle;
    mrb_get_args(mrb, "i", &handle);
    SoundManager::instance().stop(static_cast<SoundHandle>(handle));
    return mrb_nil_value();
}

static mrb_value mrb_set_sound_volume(mrb_state* mrb, mrb_value) {
    mrb_int handle;
    mrb_float volume;
    mrb_get_args(mrb, "if", &handle, &volume);
    SoundManager::instance().set_volume(static_cast<SoundHandle>(handle), static_cast<float>(volume));
    return mrb_nil_value();
}

void register_audio(mrb_state* mrb) {
    define_method(mrb, "load_sound", mrb_load_sound, MRB_ARGS_REQ(1));
    define_method(mrb, "play_sound", mrb_play_sound, MRB_ARGS_REQ(1));
    define_method(mrb, "stop_sound", mrb_stop_sound, MRB_ARGS_REQ(1));
    define_method(mrb, "set_sound_volume", mrb_set_sound_volume, MRB_ARGS_REQ(2));
}

} // namespace bindings
} // namespace gmr
