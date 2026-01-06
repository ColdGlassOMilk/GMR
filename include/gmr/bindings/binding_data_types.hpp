#ifndef GMR_BINDINGS_BINDING_DATA_TYPES_HPP
#define GMR_BINDINGS_BINDING_DATA_TYPES_HPP

// =============================================================================
// GMR Binding Data Types
// =============================================================================
//
// This header provides shared struct definitions for mruby binding data.
// These structs wrap native handles and are stored in Ruby CDATA objects.
//
// USAGE:
// - Include this header when you need to access another binding's data
// - The corresponding mrb_data_type must be declared extern where needed
//
// OWNERSHIP RULES:
// - The Ruby wrapper generally owns the handle lifecycle
// - The free function should call the manager's destroy method
// - Exceptions: Tweens (manager owns lifecycle, Ruby wrapper is just a handle)
//
// TYPE SAFETY:
// - Always use mrb_data_get_ptr with the correct &xxx_data_type
// - Never pass nullptr as the data type (bypasses type checking)
//
// =============================================================================

#include "gmr/types.hpp"

namespace gmr {
namespace bindings {

// Sprite wrapper data
// Used by: sprite.cpp, animator.cpp, sprite_animation.cpp
struct SpriteBindingData {
    SpriteHandle handle;
};

// Node wrapper data
// Used by: node.cpp
struct NodeBindingData {
    NodeHandle handle;
};

// Transform wrapper data
// Used by: transform.cpp, sprite.cpp
struct TransformBindingData {
    TransformHandle handle;
};

// Camera wrapper data
// Used by: camera.cpp
struct CameraBindingData {
    CameraHandle handle;
};

// Animator wrapper data
// Used by: animator.cpp
struct AnimatorBindingData {
    AnimatorHandle handle;
};

// SpriteAnimation wrapper data
// Used by: sprite_animation.cpp
struct SpriteAnimationBindingData {
    SpriteAnimationHandle handle;
};

// Tween wrapper data
// Note: Manager owns tween lifecycle, Ruby wrapper is just a handle
// Used by: tween.cpp
struct TweenBindingData {
    TweenHandle handle;
};

// StateMachine wrapper data
// Used by: state_machine.cpp
struct StateMachineBindingData {
    StateMachineHandle handle;
};

} // namespace bindings
} // namespace gmr

#endif // GMR_BINDINGS_BINDING_DATA_TYPES_HPP
