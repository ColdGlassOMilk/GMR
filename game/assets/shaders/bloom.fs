#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform vec2 resolution;   // Texture resolution
uniform float threshold;   // Brightness threshold (0.5 to 1.0)
uniform float intensity;   // Bloom intensity (0.5 to 2.0)
uniform float radius;      // Blur radius (1.0 to 3.0)

// Output fragment color
out vec4 finalColor;

void main()
{
    vec2 texelSize = 1.0 / resolution;
    vec4 original = texture(texture0, fragTexCoord);

    // Extract bright areas and blur them
    vec4 bloom = vec4(0.0);
    float samples = 0.0;

    // Sample in a circle pattern for blur
    for (float angle = 0.0; angle < 6.28318; angle += 0.785398) {  // 8 directions
        for (float r = 1.0; r <= 3.0; r += 1.0) {
            vec2 offset = vec2(cos(angle), sin(angle)) * texelSize * r * radius;
            vec4 sample_color = texture(texture0, fragTexCoord + offset);

            // Calculate luminance
            float luminance = dot(sample_color.rgb, vec3(0.299, 0.587, 0.114));

            // Only add bright pixels to bloom
            if (luminance > threshold) {
                bloom += sample_color * (luminance - threshold);
                samples += 1.0;
            }
        }
    }

    // Also check center pixel
    float centerLum = dot(original.rgb, vec3(0.299, 0.587, 0.114));
    if (centerLum > threshold) {
        bloom += original * (centerLum - threshold) * 2.0;
        samples += 2.0;
    }

    if (samples > 0.0) {
        bloom /= samples;
    }

    // Add bloom to original
    vec4 result = original + bloom * intensity;
    result.a = original.a;

    finalColor = result * colDiffuse * fragColor;
}
