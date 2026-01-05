#ifndef GMR_DEBUG_SERVER_HPP
#define GMR_DEBUG_SERVER_HPP

#if defined(GMR_DEBUG_ENABLED)

#include <mruby.h>
#include <string>
#include <vector>
#include <cstdint>

// Platform-agnostic socket handle (avoid including windows.h in header)
// On Windows SOCKET is unsigned __int64, on POSIX it's int
#ifdef _WIN32
using socket_t = unsigned long long;
constexpr socket_t INVALID_SOCKET_VALUE = ~static_cast<socket_t>(0);
#else
using socket_t = int;
constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

namespace gmr {
namespace debug {

// Forward declarations
struct DebugCommand;
enum class PauseReason;

// Debug server - listens for IDE connections and handles debug protocol
class DebugServer {
public:
    static DebugServer& instance();

    // Start the debug server on the specified port
    bool start(uint16_t port = 5678);

    // Stop the debug server
    void stop();

    // Poll for incoming connections and messages (non-blocking)
    void poll();

    // Check if the server is running
    bool is_running() const { return running_; }

    // Check if a client is connected
    bool is_connected() const { return client_socket_ != INVALID_SOCKET_VALUE; }

    // Check if debugging is currently active (connected and should check hooks)
    bool is_active() const;

    // Enter the pause loop - blocks until continue/step command received
    void enter_pause_loop(mrb_state* mrb, const char* file, int32_t line, PauseReason reason);

    // Pause on exception - called when an unhandled exception occurs
    void pause_on_exception(mrb_state* mrb, const char* exception_class,
                            const char* message, const char* file, int32_t line);

    // Send a message to the connected client
    void send_message(const std::string& json);

    // Notify that the debugger should pause (thread-safe)
    void request_pause();

    DebugServer(const DebugServer&) = delete;
    DebugServer& operator=(const DebugServer&) = delete;

private:
    DebugServer();
    ~DebugServer();

    // Initialize platform-specific socket library
    bool init_sockets();
    void cleanup_sockets();

    // Accept a new client connection
    void accept_client();

    // Read available data from client (non-blocking)
    void read_from_client();

    // Process a complete message
    void process_message(const std::string& message, mrb_state* mrb);

    // Handle specific commands
    void handle_command(const DebugCommand& cmd, mrb_state* mrb);

    // Handle REPL commands
    void handle_repl_command(const DebugCommand& cmd, mrb_state* mrb);

    // Close client connection
    void close_client();

    bool running_ = false;
    bool sockets_initialized_ = false;
    socket_t server_socket_ = INVALID_SOCKET_VALUE;
    socket_t client_socket_ = INVALID_SOCKET_VALUE;

    std::string receive_buffer_;
    std::vector<std::string> pending_messages_;

    // Current mruby state for exception handling
    mrb_state* current_mrb_ = nullptr;

    // Pause loop exit flag
    bool should_resume_ = false;
};

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
#endif // GMR_DEBUG_SERVER_HPP
