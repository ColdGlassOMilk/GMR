#version 100

precision mediump float;

// Input from vertex shader
varying vec2 fragTexCoord;
varying vec4 fragColor;

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
    vec4 texelColor = texture2D(texture0, uv);

    // Apply vertex color and diffuse color
    gl_FragColor = texelColor * colDiffuse * fragColor;
}
