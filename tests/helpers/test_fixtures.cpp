#include "test_fixtures.hpp"

// Include manager headers for reset functionality
#include "gmr/transform.hpp"
#include "gmr/node.hpp"
#include "gmr/draw_queue.hpp"
#include "gmr/sprite.hpp"
#include "gmr/camera.hpp"
#include "gmr/resources/texture_manager.hpp"
#include "gmr/resources/sound_manager.hpp"
#include "gmr/resources/music_manager.hpp"
#include "gmr/resources/font_manager.hpp"
#include "gmr/resources/shader_manager.hpp"
#include "gmr/particle/particle_manager.hpp"
#include "gmr/input/input_manager.hpp"
#include "gmr/animation/animation_manager.hpp"
#include "gmr/timer/timer_manager.hpp"
#include "gmr/sequence/sequence_manager.hpp"
#include "gmr/spatial/spatial_hash.hpp"
#include "gmr/state_machine/state_machine_manager.hpp"
#include "gmr/event/event_queue.hpp"
#include "gmr/ui/ui_manager.hpp"

EngineTestFixture::EngineTestFixture() {
    reset_all_managers();
}

EngineTestFixture::~EngineTestFixture() {
    // Clean up after each test to ensure isolation
    reset_all_managers();
}

void EngineTestFixture::reset_all_managers() {
    // Reset managers in reverse dependency order
    // Managers that take mrb_state* in clear() are passed nullptr
    // This is safe because they guard against null mrb_state

    // UI and event systems
    gmr::ui::UIManager::instance().clear(nullptr);
    gmr::event::EventQueue::instance().clear(nullptr);

    // Gameplay systems
    gmr::state_machine::StateMachineManager::instance().clear(nullptr);
    gmr::spatial::SpatialHash::instance().clear(nullptr);
    gmr::sequence::SequenceManager::instance().clear(nullptr);
    gmr::timer::TimerManager::instance().clear(nullptr);
    gmr::animation::AnimationManager::instance().clear(nullptr);
    gmr::particle::ParticleManager::instance().clear(nullptr);
    gmr::input::InputManager::instance().clear(nullptr);

    // Camera system
    gmr::CameraManager::instance().clear(nullptr);

    // Scene graph
    gmr::SpriteManager::instance().clear();
    gmr::NodeManager::instance().clear();
    gmr::TransformManager::instance().clear();

    // Rendering
    gmr::DrawQueue::instance().clear();

    // Resources - don't clear in tests as they may require raylib context
    // Tests should use mock resource managers instead
}
