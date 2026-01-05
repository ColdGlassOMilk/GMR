#include "gmr/animation/easing.hpp"
#include <cmath>
#include <cstring>

namespace gmr {
namespace animation {

// Constants
static constexpr float PI = 3.14159265358979323846f;

// Helper for bounce easing
static float bounce_out(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

float apply_easing(EasingType type, float t) {
    // Clamp input to valid range
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    switch (type) {
        case EasingType::LINEAR:
            return t;

        // Quadratic
        case EasingType::IN_QUAD:
            return t * t;

        case EasingType::OUT_QUAD:
            return 1.0f - (1.0f - t) * (1.0f - t);

        case EasingType::IN_OUT_QUAD:
            return t < 0.5f
                ? 2.0f * t * t
                : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

        // Cubic
        case EasingType::IN_CUBIC:
            return t * t * t;

        case EasingType::OUT_CUBIC:
            return 1.0f - std::pow(1.0f - t, 3.0f);

        case EasingType::IN_OUT_CUBIC:
            return t < 0.5f
                ? 4.0f * t * t * t
                : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;

        // Quartic
        case EasingType::IN_QUART:
            return t * t * t * t;

        case EasingType::OUT_QUART:
            return 1.0f - std::pow(1.0f - t, 4.0f);

        case EasingType::IN_OUT_QUART:
            return t < 0.5f
                ? 8.0f * t * t * t * t
                : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;

        // Quintic
        case EasingType::IN_QUINT:
            return t * t * t * t * t;

        case EasingType::OUT_QUINT:
            return 1.0f - std::pow(1.0f - t, 5.0f);

        case EasingType::IN_OUT_QUINT:
            return t < 0.5f
                ? 16.0f * t * t * t * t * t
                : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;

        // Sine
        case EasingType::IN_SINE:
            return 1.0f - std::cos((t * PI) / 2.0f);

        case EasingType::OUT_SINE:
            return std::sin((t * PI) / 2.0f);

        case EasingType::IN_OUT_SINE:
            return -(std::cos(PI * t) - 1.0f) / 2.0f;

        // Exponential
        case EasingType::IN_EXPO:
            return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);

        case EasingType::OUT_EXPO:
            return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);

        case EasingType::IN_OUT_EXPO:
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return t < 0.5f
                ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;

        // Circular
        case EasingType::IN_CIRC:
            return 1.0f - std::sqrt(1.0f - std::pow(t, 2.0f));

        case EasingType::OUT_CIRC:
            return std::sqrt(1.0f - std::pow(t - 1.0f, 2.0f));

        case EasingType::IN_OUT_CIRC:
            return t < 0.5f
                ? (1.0f - std::sqrt(1.0f - std::pow(2.0f * t, 2.0f))) / 2.0f
                : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;

        // Back (overshoot)
        case EasingType::IN_BACK: {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1.0f;
            return c3 * t * t * t - c1 * t * t;
        }

        case EasingType::OUT_BACK: {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1.0f;
            return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
        }

        case EasingType::IN_OUT_BACK: {
            const float c1 = 1.70158f;
            const float c2 = c1 * 1.525f;
            return t < 0.5f
                ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
                : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
        }

        // Elastic
        case EasingType::IN_ELASTIC: {
            const float c4 = (2.0f * PI) / 3.0f;
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
        }

        case EasingType::OUT_ELASTIC: {
            const float c4 = (2.0f * PI) / 3.0f;
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
        }

        case EasingType::IN_OUT_ELASTIC: {
            const float c5 = (2.0f * PI) / 4.5f;
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return t < 0.5f
                ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f
                : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
        }

        // Bounce
        case EasingType::IN_BOUNCE:
            return 1.0f - bounce_out(1.0f - t);

        case EasingType::OUT_BOUNCE:
            return bounce_out(t);

        case EasingType::IN_OUT_BOUNCE:
            return t < 0.5f
                ? (1.0f - bounce_out(1.0f - 2.0f * t)) / 2.0f
                : (1.0f + bounce_out(2.0f * t - 1.0f)) / 2.0f;

        default:
            return t;
    }
}

EasingType symbol_to_easing(mrb_state* mrb, mrb_sym sym) {
    const char* name = mrb_sym_name(mrb, sym);

    // Linear
    if (strcmp(name, "linear") == 0) return EasingType::LINEAR;

    // Quadratic
    if (strcmp(name, "in_quad") == 0) return EasingType::IN_QUAD;
    if (strcmp(name, "out_quad") == 0) return EasingType::OUT_QUAD;
    if (strcmp(name, "in_out_quad") == 0) return EasingType::IN_OUT_QUAD;

    // Cubic
    if (strcmp(name, "in_cubic") == 0) return EasingType::IN_CUBIC;
    if (strcmp(name, "out_cubic") == 0) return EasingType::OUT_CUBIC;
    if (strcmp(name, "in_out_cubic") == 0) return EasingType::IN_OUT_CUBIC;

    // Quartic
    if (strcmp(name, "in_quart") == 0) return EasingType::IN_QUART;
    if (strcmp(name, "out_quart") == 0) return EasingType::OUT_QUART;
    if (strcmp(name, "in_out_quart") == 0) return EasingType::IN_OUT_QUART;

    // Quintic
    if (strcmp(name, "in_quint") == 0) return EasingType::IN_QUINT;
    if (strcmp(name, "out_quint") == 0) return EasingType::OUT_QUINT;
    if (strcmp(name, "in_out_quint") == 0) return EasingType::IN_OUT_QUINT;

    // Sine
    if (strcmp(name, "in_sine") == 0) return EasingType::IN_SINE;
    if (strcmp(name, "out_sine") == 0) return EasingType::OUT_SINE;
    if (strcmp(name, "in_out_sine") == 0) return EasingType::IN_OUT_SINE;

    // Exponential
    if (strcmp(name, "in_expo") == 0) return EasingType::IN_EXPO;
    if (strcmp(name, "out_expo") == 0) return EasingType::OUT_EXPO;
    if (strcmp(name, "in_out_expo") == 0) return EasingType::IN_OUT_EXPO;

    // Circular
    if (strcmp(name, "in_circ") == 0) return EasingType::IN_CIRC;
    if (strcmp(name, "out_circ") == 0) return EasingType::OUT_CIRC;
    if (strcmp(name, "in_out_circ") == 0) return EasingType::IN_OUT_CIRC;

    // Back
    if (strcmp(name, "in_back") == 0) return EasingType::IN_BACK;
    if (strcmp(name, "out_back") == 0) return EasingType::OUT_BACK;
    if (strcmp(name, "in_out_back") == 0) return EasingType::IN_OUT_BACK;

    // Elastic
    if (strcmp(name, "in_elastic") == 0) return EasingType::IN_ELASTIC;
    if (strcmp(name, "out_elastic") == 0) return EasingType::OUT_ELASTIC;
    if (strcmp(name, "in_out_elastic") == 0) return EasingType::IN_OUT_ELASTIC;

    // Bounce
    if (strcmp(name, "in_bounce") == 0) return EasingType::IN_BOUNCE;
    if (strcmp(name, "out_bounce") == 0) return EasingType::OUT_BOUNCE;
    if (strcmp(name, "in_out_bounce") == 0) return EasingType::IN_OUT_BOUNCE;

    // Default to linear
    return EasingType::LINEAR;
}

const char* easing_to_name(EasingType type) {
    switch (type) {
        case EasingType::LINEAR: return "linear";
        case EasingType::IN_QUAD: return "in_quad";
        case EasingType::OUT_QUAD: return "out_quad";
        case EasingType::IN_OUT_QUAD: return "in_out_quad";
        case EasingType::IN_CUBIC: return "in_cubic";
        case EasingType::OUT_CUBIC: return "out_cubic";
        case EasingType::IN_OUT_CUBIC: return "in_out_cubic";
        case EasingType::IN_QUART: return "in_quart";
        case EasingType::OUT_QUART: return "out_quart";
        case EasingType::IN_OUT_QUART: return "in_out_quart";
        case EasingType::IN_QUINT: return "in_quint";
        case EasingType::OUT_QUINT: return "out_quint";
        case EasingType::IN_OUT_QUINT: return "in_out_quint";
        case EasingType::IN_SINE: return "in_sine";
        case EasingType::OUT_SINE: return "out_sine";
        case EasingType::IN_OUT_SINE: return "in_out_sine";
        case EasingType::IN_EXPO: return "in_expo";
        case EasingType::OUT_EXPO: return "out_expo";
        case EasingType::IN_OUT_EXPO: return "in_out_expo";
        case EasingType::IN_CIRC: return "in_circ";
        case EasingType::OUT_CIRC: return "out_circ";
        case EasingType::IN_OUT_CIRC: return "in_out_circ";
        case EasingType::IN_BACK: return "in_back";
        case EasingType::OUT_BACK: return "out_back";
        case EasingType::IN_OUT_BACK: return "in_out_back";
        case EasingType::IN_ELASTIC: return "in_elastic";
        case EasingType::OUT_ELASTIC: return "out_elastic";
        case EasingType::IN_OUT_ELASTIC: return "in_out_elastic";
        case EasingType::IN_BOUNCE: return "in_bounce";
        case EasingType::OUT_BOUNCE: return "out_bounce";
        case EasingType::IN_OUT_BOUNCE: return "in_out_bounce";
        default: return "linear";
    }
}

} // namespace animation
} // namespace gmr
