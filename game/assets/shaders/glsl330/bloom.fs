#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Output
out vec4 finalColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform vec2 resolution;   // Texture resolution
uniform float threshold;   // Brightness threshold (0.5 to 1.0)
uniform float intensity;   // Bloom intensity (0.5 to 2.0)
uniform float radius;      // Blur radius (1.0 to 3.0)

void main()
{
    vec2 texelSize = 1.0 / resolution;
    vec4 original = texture(texture0, fragTexCoord);

    // Extract bright areas and blur them
    vec4 bloom = vec4(0.0);
    float samples = 0.0;

    // 8 directions at 3 radii
    vec2 dir0 = vec2(1.0, 0.0);
    vec2 dir1 = vec2(0.707, 0.707);
    vec2 dir2 = vec2(0.0, 1.0);
    vec2 dir3 = vec2(-0.707, 0.707);
    vec2 dir4 = vec2(-1.0, 0.0);
    vec2 dir5 = vec2(-0.707, -0.707);
    vec2 dir6 = vec2(0.0, -1.0);
    vec2 dir7 = vec2(0.707, -0.707);

    vec4 s;
    float lum;

    // Direction 0
    s = texture(texture0, fragTexCoord + dir0 * texelSize * 1.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir0 * texelSize * 2.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir0 * texelSize * 3.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }

    // Direction 1
    s = texture(texture0, fragTexCoord + dir1 * texelSize * 1.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir1 * texelSize * 2.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir1 * texelSize * 3.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }

    // Direction 2
    s = texture(texture0, fragTexCoord + dir2 * texelSize * 1.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir2 * texelSize * 2.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir2 * texelSize * 3.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }

    // Direction 3
    s = texture(texture0, fragTexCoord + dir3 * texelSize * 1.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir3 * texelSize * 2.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir3 * texelSize * 3.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }

    // Direction 4
    s = texture(texture0, fragTexCoord + dir4 * texelSize * 1.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir4 * texelSize * 2.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir4 * texelSize * 3.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }

    // Direction 5
    s = texture(texture0, fragTexCoord + dir5 * texelSize * 1.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir5 * texelSize * 2.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir5 * texelSize * 3.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }

    // Direction 6
    s = texture(texture0, fragTexCoord + dir6 * texelSize * 1.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir6 * texelSize * 2.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir6 * texelSize * 3.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }

    // Direction 7
    s = texture(texture0, fragTexCoord + dir7 * texelSize * 1.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir7 * texelSize * 2.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }
    s = texture(texture0, fragTexCoord + dir7 * texelSize * 3.0 * radius);
    lum = dot(s.rgb, vec3(0.299, 0.587, 0.114));
    if (lum > threshold) { bloom += s * (lum - threshold); samples += 1.0; }

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
