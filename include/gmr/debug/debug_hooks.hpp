#ifndef GMR_DEBUG_HOOKS_HPP
#define GMR_DEBUG_HOOKS_HPP

#if defined(GMR_DEBUG_ENABLED)

#include <mruby.h>
#include <mruby/irep.h>

namespace gmr {
namespace debug {

// mruby debug hook - called before each bytecode instruction
// This is the main entry point for breakpoint detection and stepping
void code_fetch_hook(mrb_state* mrb, const mrb_irep* irep,
                     const mrb_code* pc, mrb_value* regs);

// Install debug hooks on an mruby state
void install_hooks(mrb_state* mrb);

// Remove debug hooks from an mruby state
void remove_hooks(mrb_state* mrb);

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
#endif // GMR_DEBUG_HOOKS_HPP
