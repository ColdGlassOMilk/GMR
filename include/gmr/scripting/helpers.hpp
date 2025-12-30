#ifndef GMR_SCRIPTING_HELPERS_HPP
#define GMR_SCRIPTING_HELPERS_HPP

#include <mruby.h>
#include <string>
#include <vector>

namespace gmr {
namespace scripting {

// Safe function call with error handling
void safe_call(mrb_state* mrb, const std::string& func);
void safe_call(mrb_state* mrb, const std::string& func, mrb_value arg);
void safe_call(mrb_state* mrb, const std::string& func, const std::vector<mrb_value>& args);

} // namespace scripting
} // namespace gmr

#endif
