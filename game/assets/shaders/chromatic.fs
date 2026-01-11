#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform float offset;  // Chromatic aberration offset (0.001 to 0.01 recommended)

// Output fragment color
out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;

    // Direction from center
    vec2 dir = uv - vec2(0.5);

    // Sample RGB channels with offset
    float r = texture(texture0, uv + dir * offset).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uv - dir * offset).b;
    float a = texture(texture0, uv).a;

    finalColor = vec4(r, g, b, a) * colDiffuse * fragColor;
}
