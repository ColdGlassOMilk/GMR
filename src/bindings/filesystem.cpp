#include "gmr/bindings/filesystem.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include "gmr/filesystem/operations.hpp"

#include <mruby/string.h>
#include <mruby/hash.h>
#include <mruby/array.h>
#include <cstring>

namespace gmr {
namespace bindings {

// Helper: Parse root from kwargs hash
static filesystem::Root parse_root_from_kwargs(mrb_state* mrb, mrb_value kwargs, const char* method_name) {
    // Default to :data
    if (mrb_nil_p(kwargs)) {
        return filesystem::Root::Data;
    }

    // Extract :root from kwargs hash
    mrb_value root_key = mrb_symbol_value(mrb_intern_cstr(mrb, "root"));
    mrb_value root_val = mrb_hash_get(mrb, kwargs, root_key);

    // If :root key not present, default to :data
    if (mrb_nil_p(root_val)) {
        return filesystem::Root::Data;
    }

    if (!mrb_symbol_p(root_val)) {
        mrb_raisef(mrb, E_TYPE_ERROR,
            "[File.%s] root must be a Symbol",
            method_name);
    }

    mrb_sym sym = mrb_symbol(root_val);
    const char* root_str = mrb_sym_name(mrb, sym);

    if (strcmp(root_str, "assets") == 0) {
        return filesystem::Root::Assets;
    } else if (strcmp(root_str, "data") == 0) {
        return filesystem::Root::Data;
    } else {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "[File.%s] Unknown root: :%s. Valid: :assets, :data",
            method_name, root_str);
    }

    return filesystem::Root::Data;  // Unreachable
}

// Helper: Get root string for error messages
static const char* root_to_string(filesystem::Root root) {
    return (root == filesystem::Root::Assets) ? "assets" : "data";
}

// ============================================================================
// GMR::FileSystem Module Functions
// ============================================================================

/// @module GMR::File
/// @description Cross-platform file I/O system for reading and writing files.
///   Provides access to read-only assets and writable data storage with identical
///   behavior on native and web builds.

/// @function read_text
/// @param path [String] Relative path to file (e.g., "save.txt" or "config/settings.txt")
/// @param root [Symbol] Logical root directory - :assets (read-only) or :data (writable). Default: :data
/// @return [String] File contents as text
/// @raise [LoadError] if file doesn't exist or read fails
/// @raise [ArgumentError] if path is invalid (directory traversal, absolute path)
/// @example # Read from data storage
///   text = GMR::File.read_text("save.txt", root: :data)
/// @example # Read from assets
///   config = GMR::File.read_text("config.txt", root: :assets)
static mrb_value mrb_fs_read_text(mrb_state* mrb, mrb_value self) {
    const char* path_cstr;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "z|H", &path_cstr, &kwargs);

    // Parse root argument from kwargs
    filesystem::Root root = parse_root_from_kwargs(mrb, kwargs, "read_text");

    // Validate path
    std::string path(path_cstr);
    if (!filesystem::is_valid_path(path)) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "[File.read_text] Invalid path: %s (no .. or absolute paths allowed)",
            path_cstr);
    }

    // Read file
    auto result = filesystem::read_text(path, root);
    if (!result) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[File.read_text] Failed to read file: %s/%s",
            root_to_string(root), path_cstr);
    }

    return mrb_str_new_cstr(mrb, result->c_str());
}

/// @function read_bytes
/// @param path [String] Relative path to file
/// @param root [Symbol] Logical root directory - :assets or :data. Default: :data
/// @return [String] File contents as binary string
/// @raise [LoadError] if file doesn't exist or read fails
/// @raise [ArgumentError] if path is invalid
/// @example # Read binary data
///   bytes = GMR::File.read_bytes("level.dat", root: :data)
static mrb_value mrb_fs_read_bytes(mrb_state* mrb, mrb_value self) {
    const char* path_cstr;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "z|H", &path_cstr, &kwargs);

    // Parse root argument from kwargs
    filesystem::Root root = parse_root_from_kwargs(mrb, kwargs, "read_bytes");

    // Validate path
    std::string path(path_cstr);
    if (!filesystem::is_valid_path(path)) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "[File.read_bytes] Invalid path: %s (no .. or absolute paths allowed)",
            path_cstr);
    }

    // Read file
    auto result = filesystem::read_bytes(path, root);
    if (!result) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[File.read_bytes] Failed to read file: %s/%s",
            root_to_string(root), path_cstr);
    }

    // Return as binary string
    return mrb_str_new(mrb, reinterpret_cast<const char*>(result->data()), result->size());
}

/// @function read_json
/// @param path [String] Relative path to JSON file
/// @param root [Symbol] Logical root directory - :assets or :data. Default: :data
/// @return [Hash, Array] Parsed JSON data
/// @raise [LoadError] if file doesn't exist, read fails, or JSON is invalid
/// @raise [ArgumentError] if path is invalid
/// @example # Read JSON save file
///   data = GMR::File.read_json("save.json", root: :data)
///   puts data["level"]  # Access parsed data
static mrb_value mrb_fs_read_json(mrb_state* mrb, mrb_value self) {
    const char* path_cstr;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "z|H", &path_cstr, &kwargs);

    // Parse root argument from kwargs
    filesystem::Root root = parse_root_from_kwargs(mrb, kwargs, "read_json");

    // Validate path
    std::string path(path_cstr);
    if (!filesystem::is_valid_path(path)) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "[File.read_json] Invalid path: %s (no .. or absolute paths allowed)",
            path_cstr);
    }

    // Read file
    auto result = filesystem::read_text(path, root);
    if (!result) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[File.read_json] Failed to read file: %s/%s",
            root_to_string(root), path_cstr);
    }

    // Parse JSON using mruby's built-in JSON module
    RClass* json_mod = mrb_module_get(mrb, "JSON");
    if (!json_mod) {
        mrb_raise(mrb, E_RUNTIME_ERROR,
            "[File.read_json] JSON module not available");
    }

    mrb_value json_str = mrb_str_new_cstr(mrb, result->c_str());
    mrb_value parsed = mrb_funcall(mrb, mrb_obj_value(json_mod), "parse", 1, json_str);

    // Check if JSON.parse raised an exception
    if (mrb->exc) {
        mrb_value exc = mrb_obj_value(mrb->exc);
        mrb->exc = nullptr;  // Clear exception

        // Get exception message
        mrb_value exc_msg = mrb_funcall(mrb, exc, "to_s", 0);
        const char* exc_str = mrb_string_p(exc_msg) ? RSTRING_PTR(exc_msg) : "unknown error";

        // Re-raise with context
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[File.read_json] Failed to parse JSON from %s/%s: %s",
            root_to_string(root), path_cstr, exc_str);
    }

    return parsed;
}

/// @function write_text
/// @param path [String] Relative path to file
/// @param content [String] Text content to write
/// @param root [Symbol] Optional. Only :data is allowed (writes to :assets are rejected). Default: :data
/// @return [Boolean] true on success
/// @raise [RuntimeError] if write fails
/// @raise [ArgumentError] if path is invalid or root is :assets
/// @example # Write log file (root: :data is implied)
///   GMR::File.write_text("log.txt", "Game started\n")
static mrb_value mrb_fs_write_text(mrb_state* mrb, mrb_value self) {
    const char* path_cstr;
    const char* content_cstr;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "zz|H", &path_cstr, &content_cstr, &kwargs);

    // Parse root argument from kwargs
    filesystem::Root root = parse_root_from_kwargs(mrb, kwargs, "write_text");

    // Reject writes to assets
    if (root == filesystem::Root::Assets) {
        mrb_raise(mrb, E_ARGUMENT_ERROR,
            "[File.write_text] Cannot write to :assets root (read-only). Use root: :data");
    }

    // Validate path
    std::string path(path_cstr);
    if (!filesystem::is_valid_path(path)) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "[File.write_text] Invalid path: %s (no .. or absolute paths allowed)",
            path_cstr);
    }

    // Write file
    std::string content(content_cstr);
    bool success = filesystem::write_text(path, content, root);
    if (!success) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[File.write_text] Failed to write file: %s/%s",
            root_to_string(root), path_cstr);
    }

    return mrb_true_value();
}

/// @function write_bytes
/// @param path [String] Relative path to file
/// @param data [String] Binary data to write
/// @param root [Symbol] Optional. Only :data is allowed. Default: :data
/// @return [Boolean] true on success
/// @raise [RuntimeError] if write fails
/// @raise [ArgumentError] if path is invalid or root is :assets
/// @example # Write binary data (root: :data is implied)
///   GMR::File.write_bytes("level.dat", binary_data)
static mrb_value mrb_fs_write_bytes(mrb_state* mrb, mrb_value self) {
    const char* path_cstr;
    mrb_value data_val;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "zo|H", &path_cstr, &data_val, &kwargs);

    // Parse root argument from kwargs
    filesystem::Root root = parse_root_from_kwargs(mrb, kwargs, "write_bytes");

    // Reject writes to assets
    if (root == filesystem::Root::Assets) {
        mrb_raise(mrb, E_ARGUMENT_ERROR,
            "[File.write_bytes] Cannot write to :assets root (read-only). Use root: :data");
    }

    // Validate path
    std::string path(path_cstr);
    if (!filesystem::is_valid_path(path)) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "[File.write_bytes] Invalid path: %s (no .. or absolute paths allowed)",
            path_cstr);
    }

    // Convert Ruby string to byte vector
    if (!mrb_string_p(data_val)) {
        mrb_raise(mrb, E_TYPE_ERROR,
            "[File.write_bytes] data must be a String");
    }

    const char* data_ptr = RSTRING_PTR(data_val);
    mrb_int data_len = RSTRING_LEN(data_val);
    std::vector<uint8_t> data(data_ptr, data_ptr + data_len);

    // Write file
    bool success = filesystem::write_bytes(path, data, root);
    if (!success) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[File.write_bytes] Failed to write file: %s/%s",
            root_to_string(root), path_cstr);
    }

    return mrb_true_value();
}

/// @function write_json
/// @param path [String] Relative path to file
/// @param data [Hash, Array] Ruby object to serialize as JSON
/// @param root [Symbol] Optional. Only :data is allowed. Default: :data
/// @param pretty [Boolean] Format JSON with indentation. Default: false (minified)
/// @return [Boolean] true on success
/// @raise [RuntimeError] if write fails or JSON serialization fails
/// @raise [ArgumentError] if path is invalid or root is :assets
/// @example # Write JSON save file (minified, root: :data implied)
///   GMR::File.write_json("save.json", { level: 5, score: 1000 })
/// @example # Write JSON with pretty formatting
///   GMR::File.write_json("config.json", data, pretty: true)
static mrb_value mrb_fs_write_json(mrb_state* mrb, mrb_value self) {
    const char* path_cstr;
    mrb_value data_obj;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "zo|H", &path_cstr, &data_obj, &kwargs);

    // Parse root argument from kwargs
    filesystem::Root root = parse_root_from_kwargs(mrb, kwargs, "write_json");

    // Reject writes to assets
    if (root == filesystem::Root::Assets) {
        mrb_raise(mrb, E_ARGUMENT_ERROR,
            "[File.write_json] Cannot write to :assets root (read-only). Use root: :data");
    }

    // Validate path
    std::string path(path_cstr);
    if (!filesystem::is_valid_path(path)) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "[File.write_json] Invalid path: %s (no .. or absolute paths allowed)",
            path_cstr);
    }

    // Check if pretty: true was passed
    bool pretty = false;
    if (!mrb_nil_p(kwargs) && mrb_hash_p(kwargs)) {
        mrb_value pretty_key = mrb_symbol_value(mrb_intern_cstr(mrb, "pretty"));
        mrb_value pretty_val = mrb_hash_get(mrb, kwargs, pretty_key);
        if (!mrb_nil_p(pretty_val)) {
            pretty = mrb_bool(pretty_val);
        }
    }

    // Serialize to JSON using mruby's built-in JSON module
    RClass* json_mod = mrb_module_get(mrb, "JSON");
    if (!json_mod) {
        mrb_raise(mrb, E_RUNTIME_ERROR,
            "[File.write_json] JSON module not available");
    }

    const char* method = pretty ? "pretty_generate" : "generate";
    mrb_value json_str = mrb_funcall(mrb, mrb_obj_value(json_mod), method, 1, data_obj);

    // Check if JSON generation raised an exception
    if (mrb->exc) {
        mrb_value exc = mrb_obj_value(mrb->exc);
        mrb->exc = nullptr;  // Clear exception

        mrb_value exc_msg = mrb_funcall(mrb, exc, "to_s", 0);
        const char* exc_str = mrb_string_p(exc_msg) ? RSTRING_PTR(exc_msg) : "unknown error";

        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[File.write_json] Failed to generate JSON: %s",
            exc_str);
    }

    // Write JSON string to file
    const char* json_cstr = RSTRING_PTR(json_str);
    std::string json_content(json_cstr);

    bool success = filesystem::write_text(path, json_content, root);
    if (!success) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
            "[File.write_json] Failed to write file: %s/%s",
            root_to_string(root), path_cstr);
    }

    return mrb_true_value();
}

/// @function exists?
/// @param path [String] Relative path to file
/// @param root [Symbol] Logical root directory - :assets or :data. Default: :data
/// @return [Boolean] true if file exists, false otherwise
/// @example # Check if save file exists
///   if GMR::File.exists?("save.json", root: :data)
///     data = GMR::File.read_json("save.json", root: :data)
///   end
static mrb_value mrb_fs_exists(mrb_state* mrb, mrb_value self) {
    const char* path_cstr;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "z|H", &path_cstr, &kwargs);

    // Parse root argument from kwargs
    filesystem::Root root = parse_root_from_kwargs(mrb, kwargs, "exists?");

    // Validate path
    std::string path(path_cstr);
    if (!filesystem::is_valid_path(path)) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "[File.exists?] Invalid path: %s (no .. or absolute paths allowed)",
            path_cstr);
    }

    // Check existence
    bool exists = filesystem::exists(path, root);
    return mrb_bool_value(exists);
}

/// @function list_files
/// @param directory [String] Relative path to directory
/// @param root [Symbol] Logical root directory - :assets or :data. Default: :data
/// @return [Array<String>] List of filenames in directory (not recursive). Returns empty array on web builds.
/// @example # List save files
///   files = GMR::File.list_files("saves", root: :data)
///   files.each { |f| puts f }  # "save1.json", "save2.json", ...
static mrb_value mrb_fs_list_files(mrb_state* mrb, mrb_value self) {
    const char* path_cstr;
    mrb_value kwargs = mrb_nil_value();

    mrb_get_args(mrb, "z|H", &path_cstr, &kwargs);

    // Parse root argument from kwargs
    filesystem::Root root = parse_root_from_kwargs(mrb, kwargs, "list_files");

    // Validate path
    std::string path(path_cstr);
    if (!filesystem::is_valid_path(path)) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
            "[File.list_files] Invalid path: %s (no .. or absolute paths allowed)",
            path_cstr);
    }

    // List files
    auto files = filesystem::list_files(path, root);

    // Convert to Ruby array
    mrb_value arr = mrb_ary_new_capa(mrb, files.size());
    for (const auto& filename : files) {
        mrb_ary_push(mrb, arr, mrb_str_new_cstr(mrb, filename.c_str()));
    }

    return arr;
}

// ============================================================================
// Module Registration
// ============================================================================

void register_file(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);
    RClass* fs = mrb_define_module_under(mrb, gmr, "File");

    // Read operations
    mrb_define_module_function(mrb, fs, "read_text", mrb_fs_read_text, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
    mrb_define_module_function(mrb, fs, "read_json", mrb_fs_read_json, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
    mrb_define_module_function(mrb, fs, "read_bytes", mrb_fs_read_bytes, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));

    // Write operations
    mrb_define_module_function(mrb, fs, "write_text", mrb_fs_write_text, MRB_ARGS_REQ(2) | MRB_ARGS_OPT(1));
    mrb_define_module_function(mrb, fs, "write_json", mrb_fs_write_json, MRB_ARGS_ARG(2, 2));  // 2 required, 2 optional (root + kwargs)
    mrb_define_module_function(mrb, fs, "write_bytes", mrb_fs_write_bytes, MRB_ARGS_REQ(2) | MRB_ARGS_OPT(1));

    // Query operations
    mrb_define_module_function(mrb, fs, "exists?", mrb_fs_exists, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
    mrb_define_module_function(mrb, fs, "list_files", mrb_fs_list_files, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
}

}  // namespace bindings
}  // namespace gmr
