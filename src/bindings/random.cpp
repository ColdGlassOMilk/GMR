#include "gmr/bindings/random.hpp"
#include "gmr/bindings/binding_helpers.hpp"
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <random>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace gmr {
namespace bindings {

// ============================================================================
// RNG State Management
// ============================================================================

// We use xoshiro256** as the generator - fast, high quality, small state
// This is stored globally and managed by the engine (not per-Ruby-object)
// to ensure determinism across hot reloads.

struct RNGState {
    std::mt19937_64 generator;
    uint64_t seed = 0;

    RNGState() {
        // Default seed from random device
        std::random_device rd;
        seed = rd();
        generator.seed(seed);
    }

    void reseed(uint64_t new_seed) {
        seed = new_seed;
        generator.seed(seed);
    }
};

// Global default RNG
static RNGState g_default_rng;

// Named RNG streams
static std::unordered_map<std::string, RNGState> g_named_streams;

// Get or create a named stream
static RNGState& get_stream(const std::string& name) {
    if (name.empty()) {
        return g_default_rng;
    }
    auto it = g_named_streams.find(name);
    if (it == g_named_streams.end()) {
        g_named_streams[name] = RNGState();
    }
    return g_named_streams[name];
}

// ============================================================================
// Random Module Functions
// ============================================================================

/// @module GMR::Random
/// @description Deterministic random number generation with seedable streams.
///   Supports multiple isolated RNG streams for gameplay vs visual effects.
/// @example # Seed for deterministic replay
///   Random.seed(12345)
///   puts Random.int(1, 10)  # Always same sequence with same seed
///
/// @example # Use named streams for isolation
///   Random.seed_stream(:loot, 42)
///   loot_roll = Random.stream(:loot).int(1, 100)
///   particle_dir = Random.stream(:vfx).float(-1, 1)

/// @function seed
/// @description Seed the default random number generator.
///   Call with a specific seed for deterministic sequences (replays, testing).
///   Call without arguments to reseed from system entropy.
/// @param seed [Integer] (optional) The seed value. If omitted, uses system entropy.
/// @returns [Integer] The seed that was set
/// @example Random.seed(12345)  # Deterministic
/// @example Random.seed         # Re-randomize
static mrb_value mrb_random_seed(mrb_state* mrb, mrb_value) {
    mrb_int seed;
    mrb_bool has_seed = false;

    mrb_get_args(mrb, "|i?", &seed, &has_seed);

    if (has_seed) {
        g_default_rng.reseed(static_cast<uint64_t>(seed));
    } else {
        std::random_device rd;
        g_default_rng.reseed(rd());
    }

    return mrb_fixnum_value(static_cast<mrb_int>(g_default_rng.seed));
}

/// @function seed_stream
/// @description Seed a named random stream.
/// @param name [Symbol] The stream name
/// @param seed [Integer] The seed value
/// @returns [Integer] The seed that was set
/// @example Random.seed_stream(:loot, 42)
static mrb_value mrb_random_seed_stream(mrb_state* mrb, mrb_value) {
    mrb_sym name_sym;
    mrb_int seed;

    mrb_get_args(mrb, "ni", &name_sym, &seed);

    std::string name = mrb_sym_name(mrb, name_sym);
    RNGState& rng = get_stream(name);
    rng.reseed(static_cast<uint64_t>(seed));

    return mrb_fixnum_value(seed);
}

/// @function int
/// @description Generate a random integer in the given range (inclusive).
/// @param min [Integer] Minimum value (inclusive)
/// @param max [Integer] Maximum value (inclusive)
/// @returns [Integer] Random integer in [min, max]
/// @example roll = Random.int(1, 6)  # Dice roll
/// @example index = Random.int(0, array.length - 1)
static mrb_value mrb_random_int(mrb_state* mrb, mrb_value) {
    mrb_int min, max;
    mrb_get_args(mrb, "ii", &min, &max);

    if (min > max) {
        std::swap(min, max);
    }

    std::uniform_int_distribution<mrb_int> dist(min, max);
    return mrb_fixnum_value(dist(g_default_rng.generator));
}

/// @function float
/// @description Generate a random float in the given range.
/// @param min [Float] Minimum value (inclusive), default 0.0
/// @param max [Float] Maximum value (exclusive), default 1.0
/// @returns [Float] Random float in [min, max)
/// @example x = Random.float(-1.0, 1.0)
/// @example percent = Random.float  # 0.0 to 1.0
static mrb_value mrb_random_float(mrb_state* mrb, mrb_value) {
    mrb_float min = 0.0, max = 1.0;
    mrb_get_args(mrb, "|ff", &min, &max);

    if (min > max) {
        std::swap(min, max);
    }

    std::uniform_real_distribution<double> dist(min, max);
    return mrb_float_value(mrb, dist(g_default_rng.generator));
}

/// @function bool
/// @description Generate a random boolean (true or false with equal probability).
/// @returns [Boolean] Random true or false
/// @example if Random.bool then attack_left else attack_right end
static mrb_value mrb_random_bool(mrb_state* mrb, mrb_value) {
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(g_default_rng.generator) ? mrb_true_value() : mrb_false_value();
}

/// @function chance
/// @description Return true with the given probability (0.0 to 1.0).
/// @param probability [Float] Probability of returning true (0.0 = never, 1.0 = always)
/// @returns [Boolean] true with the given probability
/// @example if Random.chance(0.25) then critical_hit end
/// @example if Random.chance(0.1) then spawn_rare_enemy end
static mrb_value mrb_random_chance(mrb_state* mrb, mrb_value) {
    mrb_float probability;
    mrb_get_args(mrb, "f", &probability);

    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(g_default_rng.generator) < probability ? mrb_true_value() : mrb_false_value();
}

/// @function choose
/// @description Choose a random element from an array.
/// @param array [Array] The array to choose from
/// @returns [Object] A random element from the array, or nil if empty
/// @example enemy_type = Random.choose([:goblin, :orc, :troll])
/// @example spawn_point = Random.choose(spawn_points)
static mrb_value mrb_random_choose(mrb_state* mrb, mrb_value) {
    mrb_value array;
    mrb_get_args(mrb, "A", &array);

    mrb_int len = RARRAY_LEN(array);
    if (len == 0) {
        return mrb_nil_value();
    }

    std::uniform_int_distribution<mrb_int> dist(0, len - 1);
    mrb_int index = dist(g_default_rng.generator);
    return mrb_ary_ref(mrb, array, index);
}

/// @function shuffle
/// @description Shuffle an array in place using Fisher-Yates algorithm.
/// @param array [Array] The array to shuffle (modified in place)
/// @returns [Array] The same array, now shuffled
/// @example deck = Random.shuffle([1, 2, 3, 4, 5])
static mrb_value mrb_random_shuffle(mrb_state* mrb, mrb_value) {
    mrb_value array;
    mrb_get_args(mrb, "A", &array);

    mrb_int len = RARRAY_LEN(array);
    if (len <= 1) {
        return array;
    }

    // Fisher-Yates shuffle
    for (mrb_int i = len - 1; i > 0; --i) {
        std::uniform_int_distribution<mrb_int> dist(0, i);
        mrb_int j = dist(g_default_rng.generator);
        if (i != j) {
            mrb_value tmp = mrb_ary_ref(mrb, array, i);
            mrb_ary_set(mrb, array, i, mrb_ary_ref(mrb, array, j));
            mrb_ary_set(mrb, array, j, tmp);
        }
    }

    return array;
}

/// @function weighted
/// @description Choose a random key based on weighted probabilities.
///   Weights don't need to sum to 100 - they're relative.
/// @param weights [Hash] A hash mapping keys to their weights (Integer or Float)
/// @returns [Object] One of the keys, chosen by weight
/// @example tier = Random.weighted({ common: 70, rare: 25, legendary: 5 })
/// @example direction = Random.weighted({ north: 1, south: 1, east: 2 })  # East twice as likely
static mrb_value mrb_random_weighted(mrb_state* mrb, mrb_value) {
    mrb_value hash;
    mrb_get_args(mrb, "H", &hash);

    // Calculate total weight
    double total_weight = 0.0;
    mrb_value keys = mrb_hash_keys(mrb, hash);
    mrb_int len = RARRAY_LEN(keys);

    if (len == 0) {
        return mrb_nil_value();
    }

    for (mrb_int i = 0; i < len; ++i) {
        mrb_value key = mrb_ary_ref(mrb, keys, i);
        mrb_value weight_val = mrb_hash_get(mrb, hash, key);
        double weight = mrb_as_float(mrb, weight_val);
        if (weight < 0) weight = 0;
        total_weight += weight;
    }

    if (total_weight <= 0) {
        // All weights zero or negative - return first key
        return mrb_ary_ref(mrb, keys, 0);
    }

    // Generate random value in [0, total_weight)
    std::uniform_real_distribution<double> dist(0.0, total_weight);
    double roll = dist(g_default_rng.generator);

    // Find which key the roll lands on
    double cumulative = 0.0;
    for (mrb_int i = 0; i < len; ++i) {
        mrb_value key = mrb_ary_ref(mrb, keys, i);
        mrb_value weight_val = mrb_hash_get(mrb, hash, key);
        double weight = mrb_as_float(mrb, weight_val);
        if (weight < 0) weight = 0;
        cumulative += weight;
        if (roll < cumulative) {
            return key;
        }
    }

    // Fallback (shouldn't happen due to floating point, but just in case)
    return mrb_ary_ref(mrb, keys, len - 1);
}

// ============================================================================
// Stream Class - for named RNG streams
// ============================================================================

/// @class GMR::Random::Stream
/// @description A named random number stream for isolated RNG.
///   Use Random.stream(:name) to get a stream instance.
/// @example loot_rng = Random.stream(:loot)
///   loot_rng.seed(42)
///   roll = loot_rng.int(1, 100)

struct StreamData {
    std::string name;
};

static void stream_data_free(mrb_state*, void* ptr) {
    delete static_cast<StreamData*>(ptr);
}

static const mrb_data_type stream_data_type = {
    "Random::Stream", stream_data_free
};

static RClass* stream_class_ptr = nullptr;

static StreamData* get_stream_data(mrb_state* mrb, mrb_value self) {
    return static_cast<StreamData*>(mrb_data_get_ptr(mrb, self, &stream_data_type));
}

/// @function stream
/// @description Get a named random stream. Creates it if it doesn't exist.
/// @param name [Symbol] The stream name
/// @returns [Random::Stream] The stream object
/// @example loot_rng = Random.stream(:loot)
static mrb_value mrb_random_stream(mrb_state* mrb, mrb_value) {
    mrb_sym name_sym;
    mrb_get_args(mrb, "n", &name_sym);

    std::string name = mrb_sym_name(mrb, name_sym);

    // Ensure stream exists
    get_stream(name);

    // Create Stream object
    if (!stream_class_ptr) {
        RClass* gmr = get_gmr_module(mrb);
        RClass* random_mod = mrb_module_get_under(mrb, gmr, "Random");
        stream_class_ptr = mrb_class_get_under(mrb, random_mod, "Stream");
    }

    auto* data = new StreamData{name};
    return mrb_obj_value(mrb_data_object_alloc(mrb, stream_class_ptr, data, &stream_data_type));
}

// Stream instance methods - mirror the module functions but use the stream's RNG

static mrb_value mrb_stream_seed(mrb_state* mrb, mrb_value self) {
    StreamData* data = get_stream_data(mrb, self);
    if (!data) return mrb_nil_value();

    mrb_int seed;
    mrb_bool has_seed = false;
    mrb_get_args(mrb, "|i?", &seed, &has_seed);

    RNGState& rng = get_stream(data->name);
    if (has_seed) {
        rng.reseed(static_cast<uint64_t>(seed));
    } else {
        std::random_device rd;
        rng.reseed(rd());
    }

    return mrb_fixnum_value(static_cast<mrb_int>(rng.seed));
}

static mrb_value mrb_stream_int(mrb_state* mrb, mrb_value self) {
    StreamData* data = get_stream_data(mrb, self);
    if (!data) return mrb_fixnum_value(0);

    mrb_int min, max;
    mrb_get_args(mrb, "ii", &min, &max);

    if (min > max) std::swap(min, max);

    RNGState& rng = get_stream(data->name);
    std::uniform_int_distribution<mrb_int> dist(min, max);
    return mrb_fixnum_value(dist(rng.generator));
}

static mrb_value mrb_stream_float(mrb_state* mrb, mrb_value self) {
    StreamData* data = get_stream_data(mrb, self);
    if (!data) return mrb_float_value(mrb, 0.0);

    mrb_float min = 0.0, max = 1.0;
    mrb_get_args(mrb, "|ff", &min, &max);

    if (min > max) std::swap(min, max);

    RNGState& rng = get_stream(data->name);
    std::uniform_real_distribution<double> dist(min, max);
    return mrb_float_value(mrb, dist(rng.generator));
}

static mrb_value mrb_stream_bool(mrb_state* mrb, mrb_value self) {
    StreamData* data = get_stream_data(mrb, self);
    if (!data) return mrb_false_value();

    RNGState& rng = get_stream(data->name);
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(rng.generator) ? mrb_true_value() : mrb_false_value();
}

static mrb_value mrb_stream_chance(mrb_state* mrb, mrb_value self) {
    StreamData* data = get_stream_data(mrb, self);
    if (!data) return mrb_false_value();

    mrb_float probability;
    mrb_get_args(mrb, "f", &probability);

    RNGState& rng = get_stream(data->name);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng.generator) < probability ? mrb_true_value() : mrb_false_value();
}

static mrb_value mrb_stream_choose(mrb_state* mrb, mrb_value self) {
    StreamData* data = get_stream_data(mrb, self);
    if (!data) return mrb_nil_value();

    mrb_value array;
    mrb_get_args(mrb, "A", &array);

    mrb_int len = RARRAY_LEN(array);
    if (len == 0) return mrb_nil_value();

    RNGState& rng = get_stream(data->name);
    std::uniform_int_distribution<mrb_int> dist(0, len - 1);
    return mrb_ary_ref(mrb, array, dist(rng.generator));
}

// ============================================================================
// Registration
// ============================================================================

void register_random(mrb_state* mrb) {
    RClass* gmr = get_gmr_module(mrb);

    // Define Random module under GMR
    RClass* random_mod = mrb_define_module_under(mrb, gmr, "Random");

    // Module functions
    mrb_define_module_function(mrb, random_mod, "seed", mrb_random_seed, MRB_ARGS_OPT(1));
    mrb_define_module_function(mrb, random_mod, "seed_stream", mrb_random_seed_stream, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, random_mod, "stream", mrb_random_stream, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, random_mod, "int", mrb_random_int, MRB_ARGS_REQ(2));
    mrb_define_module_function(mrb, random_mod, "float", mrb_random_float, MRB_ARGS_OPT(2));
    mrb_define_module_function(mrb, random_mod, "bool", mrb_random_bool, MRB_ARGS_NONE());
    mrb_define_module_function(mrb, random_mod, "chance", mrb_random_chance, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, random_mod, "choose", mrb_random_choose, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, random_mod, "shuffle", mrb_random_shuffle, MRB_ARGS_REQ(1));
    mrb_define_module_function(mrb, random_mod, "weighted", mrb_random_weighted, MRB_ARGS_REQ(1));

    // Define Stream class under Random module
    stream_class_ptr = mrb_define_class_under(mrb, random_mod, "Stream", mrb->object_class);
    MRB_SET_INSTANCE_TT(stream_class_ptr, MRB_TT_CDATA);

    // Stream instance methods
    mrb_define_method(mrb, stream_class_ptr, "seed", mrb_stream_seed, MRB_ARGS_OPT(1));
    mrb_define_method(mrb, stream_class_ptr, "int", mrb_stream_int, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, stream_class_ptr, "float", mrb_stream_float, MRB_ARGS_OPT(2));
    mrb_define_method(mrb, stream_class_ptr, "bool", mrb_stream_bool, MRB_ARGS_NONE());
    mrb_define_method(mrb, stream_class_ptr, "chance", mrb_stream_chance, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, stream_class_ptr, "choose", mrb_stream_choose, MRB_ARGS_REQ(1));
}

} // namespace bindings
} // namespace gmr
