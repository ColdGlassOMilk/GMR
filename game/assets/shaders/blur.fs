#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform vec2 resolution;  // Texture resolution
uniform float radius;     // Blur radius in pixels (1.0 to 5.0 recommended)

// Output fragment color
out vec4 finalColor;

void main()
{
    vec2 texelSize = 1.0 / resolution;
    vec4 result = vec4(0.0);

    // Simple box blur (9 samples)
    float total = 0.0;
    for (float x = -1.0; x <= 1.0; x += 1.0) {
        for (float y = -1.0; y <= 1.0; y += 1.0) {
            vec2 offset = vec2(x, y) * texelSize * radius;
            result += texture(texture0, fragTexCoord + offset);
            total += 1.0;
        }
    }

    result /= total;
    finalColor = result * colDiffuse * fragColor;
}
