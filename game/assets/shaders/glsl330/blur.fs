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
uniform vec2 resolution;  // Texture resolution
uniform float radius;     // Blur radius in pixels (1.0 to 5.0 recommended)

void main()
{
    vec2 texelSize = 1.0 / resolution;
    vec4 result = vec4(0.0);

    // Simple box blur (9 samples)
    result += texture(texture0, fragTexCoord + vec2(-1.0, -1.0) * texelSize * radius);
    result += texture(texture0, fragTexCoord + vec2( 0.0, -1.0) * texelSize * radius);
    result += texture(texture0, fragTexCoord + vec2( 1.0, -1.0) * texelSize * radius);
    result += texture(texture0, fragTexCoord + vec2(-1.0,  0.0) * texelSize * radius);
    result += texture(texture0, fragTexCoord + vec2( 0.0,  0.0) * texelSize * radius);
    result += texture(texture0, fragTexCoord + vec2( 1.0,  0.0) * texelSize * radius);
    result += texture(texture0, fragTexCoord + vec2(-1.0,  1.0) * texelSize * radius);
    result += texture(texture0, fragTexCoord + vec2( 0.0,  1.0) * texelSize * radius);
    result += texture(texture0, fragTexCoord + vec2( 1.0,  1.0) * texelSize * radius);

    result /= 9.0;
    finalColor = result * colDiffuse * fragColor;
}
