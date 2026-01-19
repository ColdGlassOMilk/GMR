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
uniform float offset;  // Chromatic aberration offset (0.001 to 0.01 recommended)

void main()
{
    vec2 uv = fragTexCoord;

    // Direction from center
    vec2 dir = uv - vec2(0.5);

    // Calculate offset UVs and clamp to valid range
    vec2 uvR = clamp(uv + dir * offset, 0.0, 1.0);
    vec2 uvB = clamp(uv - dir * offset, 0.0, 1.0);

    // Sample RGB channels with offset
    float r = texture(texture0, uvR).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uvB).b;
    float a = texture(texture0, uv).a;

    finalColor = vec4(r, g, b, a) * colDiffuse * fragColor;
}
