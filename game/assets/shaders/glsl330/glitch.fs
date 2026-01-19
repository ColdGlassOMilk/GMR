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
uniform float time;       // Time for animation
uniform float intensity;  // Glitch intensity (0.0 to 1.0)

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 uv = fragTexCoord;

    // Random horizontal offset per scanline
    float lineNoise = rand(vec2(floor(uv.y * 100.0), floor(time * 10.0)));
    if (lineNoise > 1.0 - intensity * 0.1) {
        uv.x += (rand(vec2(time, uv.y)) - 0.5) * intensity * 0.1;
    }

    // Clamp UV after line displacement
    uv = clamp(uv, 0.0, 1.0);

    // Color channel split with clamped UVs
    float splitAmount = intensity * 0.02 * rand(vec2(time * 0.1, 0.0));
    vec2 uvR = clamp(uv + vec2(splitAmount, 0.0), 0.0, 1.0);
    vec2 uvB = clamp(uv - vec2(splitAmount, 0.0), 0.0, 1.0);
    float r = texture(texture0, uvR).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uvB).b;
    float a = texture(texture0, uv).a;

    // Random blocks
    float blockNoise = rand(vec2(floor(uv.x * 20.0), floor(uv.y * 20.0) + floor(time * 5.0)));
    if (blockNoise > 1.0 - intensity * 0.05) {
        vec2 blockUV = clamp(uv + vec2(rand(vec2(time)) - 0.5, rand(vec2(time * 2.0)) - 0.5) * 0.1, 0.0, 1.0);
        r = texture(texture0, blockUV).r;
        g = texture(texture0, blockUV).g;
        b = texture(texture0, blockUV).b;
    }

    finalColor = vec4(r, g, b, a) * colDiffuse * fragColor;
}
