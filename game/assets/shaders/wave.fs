#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform float time;       // Time in seconds
uniform float amplitude;  // Wave amplitude (0.0 to 0.1 recommended)
uniform float frequency;  // Wave frequency (5.0 to 20.0 recommended)

// Output fragment color
out vec4 finalColor;

void main()
{
    // Create wave distortion
    vec2 uv = fragTexCoord;
    float wave = sin(uv.y * frequency + time * 3.0) * amplitude;
    uv.x += wave;

    // Sample the texture with distorted coordinates
    vec4 texelColor = texture(texture0, uv);

    // Apply vertex color and diffuse color
    finalColor = texelColor * colDiffuse * fragColor;
}
