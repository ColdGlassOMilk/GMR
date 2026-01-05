#if defined(GMR_DEBUG_ENABLED)

// Include Windows socket headers BEFORE our headers to avoid conflicts
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#endif

#include "gmr/debug/debug_server.hpp"
#include "gmr/debug/debug_state.hpp"
#include "gmr/debug/debug_hooks.hpp"
#include "gmr/debug/breakpoint_manager.hpp"
#include "gmr/debug/variable_inspector.hpp"
#include "gmr/debug/json_protocol.hpp"
#include <mruby/proc.h>
#include <mruby/debug.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>

namespace gmr {
namespace debug {

DebugServer::DebugServer() = default;

DebugServer::~DebugServer() {
    stop();
}

DebugServer& DebugServer::instance() {
    static DebugServer instance;
    return instance;
}

bool DebugServer::init_sockets() {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "[Debug] WSAStartup failed\n");
        return false;
    }
#endif
    sockets_initialized_ = true;
    return true;
}

void DebugServer::cleanup_sockets() {
#ifdef _WIN32
    if (sockets_initialized_) {
        WSACleanup();
    }
#endif
    sockets_initialized_ = false;
}

bool DebugServer::start(uint16_t port) {
    if (running_) return true;

    if (!init_sockets()) {
        return false;
    }

    // Create server socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket_ == INVALID_SOCKET_VALUE) {
        fprintf(stderr, "[Debug] Failed to create socket\n");
        cleanup_sockets();
        return false;
    }

    // Allow address reuse
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    // Set non-blocking
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(server_socket_, FIONBIO, &mode);
#else
    int flags = fcntl(server_socket_, F_GETFL, 0);
    fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);
#endif

    // Bind to localhost
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1
    addr.sin_port = htons(port);

    if (bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[Debug] Failed to bind to port %d\n", port);
#ifdef _WIN32
        closesocket(server_socket_);
#else
        close(server_socket_);
#endif
        server_socket_ = INVALID_SOCKET_VALUE;
        cleanup_sockets();
        return false;
    }

    if (listen(server_socket_, 1) < 0) {
        fprintf(stderr, "[Debug] Failed to listen\n");
#ifdef _WIN32
        closesocket(server_socket_);
#else
        close(server_socket_);
#endif
        server_socket_ = INVALID_SOCKET_VALUE;
        cleanup_sockets();
        return false;
    }

    running_ = true;
    printf("[Debug] Server listening on 127.0.0.1:%d\n", port);
    return true;
}

void DebugServer::stop() {
    close_client();

    if (server_socket_ != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(server_socket_);
#else
        close(server_socket_);
#endif
        server_socket_ = INVALID_SOCKET_VALUE;
    }

    cleanup_sockets();
    running_ = false;
}

void DebugServer::close_client() {
    if (client_socket_ != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(client_socket_);
#else
        close(client_socket_);
#endif
        client_socket_ = INVALID_SOCKET_VALUE;
        printf("[Debug] Client disconnected\n");
    }
    receive_buffer_.clear();
    DebugStateManager::instance().reset();
}

void DebugServer::accept_client() {
    if (client_socket_ != INVALID_SOCKET_VALUE) {
        return;  // Already have a client
    }

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    socket_t new_client = accept(server_socket_, (struct sockaddr*)&client_addr, &addr_len);
    if (new_client == INVALID_SOCKET_VALUE) {
        return;  // No pending connection
    }

    // Set non-blocking
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(new_client, FIONBIO, &mode);
#else
    int flags = fcntl(new_client, F_GETFL, 0);
    fcntl(new_client, F_SETFL, flags | O_NONBLOCK);
#endif

    client_socket_ = new_client;
    receive_buffer_.clear();
    printf("[Debug] Client connected\n");
}

void DebugServer::read_from_client() {
    if (client_socket_ == INVALID_SOCKET_VALUE) return;

    char buf[4096];
#ifdef _WIN32
    int n = recv(client_socket_, buf, sizeof(buf) - 1, 0);
    if (n == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            close_client();
        }
        return;
    }
#else
    ssize_t n = recv(client_socket_, buf, sizeof(buf) - 1, 0);
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            close_client();
        }
        return;
    }
#endif

    if (n == 0) {
        close_client();
        return;
    }

    buf[n] = '\0';
    receive_buffer_ += buf;

    // Process complete messages (newline-delimited)
    size_t pos;
    while ((pos = receive_buffer_.find('\n')) != std::string::npos) {
        std::string message = receive_buffer_.substr(0, pos);
        receive_buffer_.erase(0, pos + 1);

        if (!message.empty()) {
            pending_messages_.push_back(message);
        }
    }
}

void DebugServer::poll() {
    if (!running_) return;

    accept_client();
    read_from_client();

    // Process messages outside of pause loop (breakpoints, etc.)
    for (const auto& msg : pending_messages_) {
        DebugCommand cmd = parse_command(msg);
        if (cmd.type == CommandType::SET_BREAKPOINT) {
            BreakpointManager::instance().add(cmd.file, cmd.line);
            printf("[Debug] Breakpoint set: %s:%d\n", cmd.file.c_str(), cmd.line);
        } else if (cmd.type == CommandType::CLEAR_BREAKPOINT) {
            BreakpointManager::instance().remove(cmd.file, cmd.line);
            printf("[Debug] Breakpoint cleared: %s:%d\n", cmd.file.c_str(), cmd.line);
        } else if (cmd.type == CommandType::PAUSE) {
            DebugStateManager::instance().request_pause();
        }
    }
    pending_messages_.clear();
}

bool DebugServer::is_active() const {
    if (!running_ || client_socket_ == INVALID_SOCKET_VALUE) {
        return false;
    }

    auto& state = DebugStateManager::instance();
    return state.is_stepping() ||
           state.pause_requested() ||
           !BreakpointManager::instance().empty();
}

void DebugServer::send_message(const std::string& json) {
    if (client_socket_ == INVALID_SOCKET_VALUE) return;

    std::string msg = json + "\n";
#ifdef _WIN32
    send(client_socket_, msg.c_str(), static_cast<int>(msg.length()), 0);
#else
    send(client_socket_, msg.c_str(), msg.length(), 0);
#endif
}

void DebugServer::enter_pause_loop(mrb_state* mrb, const char* file, int32_t line, PauseReason reason) {
    auto& state = DebugStateManager::instance();
    state.set_paused(reason);

    current_mrb_ = mrb;
    should_resume_ = false;

    // Determine reason string
    const char* reason_str = "unknown";
    switch (reason) {
        case PauseReason::BREAKPOINT: reason_str = "breakpoint"; break;
        case PauseReason::STEP: reason_str = "step"; break;
        case PauseReason::PAUSE_COMMAND: reason_str = "pause"; break;
        case PauseReason::EXCEPTION: reason_str = "exception"; break;
        default: break;
    }

    // Get stack and locals
    std::string stack_json = get_stack_trace_json(mrb);
    std::string locals_json = get_locals_json(mrb, 0);

    // Send paused event
    std::string event = make_paused_event(reason_str, file, line, stack_json, locals_json);
    send_message(event);

    printf("[Debug] Paused at %s:%d (%s)\n", file ? file : "(unknown)", line, reason_str);

    // Pause loop - poll for commands
    while (!should_resume_ && running_) {
        read_from_client();

        // Process pending messages
        for (const auto& msg : pending_messages_) {
            process_message(msg, mrb);
        }
        pending_messages_.clear();

        // Check if client disconnected
        if (client_socket_ == INVALID_SOCKET_VALUE) {
            should_resume_ = true;
            state.set_running();
            break;
        }

        // Brief sleep to avoid spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    current_mrb_ = nullptr;
}

void DebugServer::process_message(const std::string& message, mrb_state* mrb) {
    DebugCommand cmd = parse_command(message);
    handle_command(cmd, mrb);
}

void DebugServer::handle_command(const DebugCommand& cmd, mrb_state* mrb) {
    auto& state = DebugStateManager::instance();

    switch (cmd.type) {
        case CommandType::CONTINUE:
            printf("[Debug] Continuing...\n");
            state.set_running();
            should_resume_ = true;
            break;

        case CommandType::STEP_OVER:
            printf("[Debug] Step over\n");
            if (mrb && mrb->c && mrb->c->ci) {
                int depth = static_cast<int>(mrb->c->ci - mrb->c->cibase);
                // Get current file/line
                const char* file = nullptr;
                int32_t line = 0;
                if (mrb->c->ci->proc && !MRB_PROC_CFUNC_P(mrb->c->ci->proc)) {
                    const mrb_irep* irep = mrb->c->ci->proc->body.irep;
                    if (irep && mrb->c->ci->pc) {
                        uint32_t pc_offset = static_cast<uint32_t>(mrb->c->ci->pc - irep->iseq);
                        mrb_debug_get_position(mrb, irep, pc_offset, &line, &file);
                    }
                }
                state.set_stepping_over(depth, file, line);
            }
            should_resume_ = true;
            break;

        case CommandType::STEP_INTO:
            printf("[Debug] Step into\n");
            if (mrb && mrb->c && mrb->c->ci) {
                const char* file = nullptr;
                int32_t line = 0;
                if (mrb->c->ci->proc && !MRB_PROC_CFUNC_P(mrb->c->ci->proc)) {
                    const mrb_irep* irep = mrb->c->ci->proc->body.irep;
                    if (irep && mrb->c->ci->pc) {
                        uint32_t pc_offset = static_cast<uint32_t>(mrb->c->ci->pc - irep->iseq);
                        mrb_debug_get_position(mrb, irep, pc_offset, &line, &file);
                    }
                }
                state.set_stepping_into(file, line);
            }
            should_resume_ = true;
            break;

        case CommandType::STEP_OUT:
            printf("[Debug] Step out\n");
            if (mrb && mrb->c && mrb->c->ci) {
                int depth = static_cast<int>(mrb->c->ci - mrb->c->cibase);
                state.set_stepping_out(depth);
            }
            should_resume_ = true;
            break;

        case CommandType::SET_BREAKPOINT:
            BreakpointManager::instance().add(cmd.file, cmd.line);
            printf("[Debug] Breakpoint set: %s:%d\n", cmd.file.c_str(), cmd.line);
            break;

        case CommandType::CLEAR_BREAKPOINT:
            BreakpointManager::instance().remove(cmd.file, cmd.line);
            printf("[Debug] Breakpoint cleared: %s:%d\n", cmd.file.c_str(), cmd.line);
            break;

        case CommandType::EVALUATE:
            {
                std::string result = evaluate_expression(mrb, cmd.expression, cmd.frame_id);
                std::string response = make_evaluate_response(true, result);
                send_message(response);
            }
            break;

        default:
            break;
    }
}

void DebugServer::pause_on_exception(mrb_state* mrb, const char* exception_class,
                                      const char* message, const char* file, int32_t line) {
    if (!is_connected()) return;

    std::string stack_json = get_stack_trace_json(mrb);
    std::string event = make_exception_event(exception_class, message, file, line, stack_json);
    send_message(event);

    // Enter pause loop for exception
    enter_pause_loop(mrb, file, line, PauseReason::EXCEPTION);
}

void DebugServer::request_pause() {
    DebugStateManager::instance().request_pause();
}

} // namespace debug
} // namespace gmr

#endif // GMR_DEBUG_ENABLED
