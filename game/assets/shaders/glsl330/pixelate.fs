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
uniform float pixelSize;  // Size of each "pixel" in texels (8.0 recommended)
uniform vec2 resolution;  // Texture resolution in pixels

void main()
{
    // Calculate pixelated UV coordinates
    vec2 uv = fragTexCoord;
    vec2 pixelCount = resolution / pixelSize;
    uv = floor(uv * pixelCount) / pixelCount;

    // Sample the texture at pixelated position
    vec4 texelColor = texture(texture0, uv);

    // Apply vertex color and diffuse color
    finalColor = texelColor * colDiffuse * fragColor;
}
