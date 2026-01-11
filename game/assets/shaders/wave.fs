#version 100

precision mediump float;

// Input from vertex shader
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform float time;       // Time in seconds
uniform float amplitude;  // Wave amplitude (0.0 to 0.1 recommended)
uniform float frequency;  // Wave frequency (5.0 to 20.0 recommended)

void main()
{
    // Create wave distortion
    vec2 uv = fragTexCoord;
    float wave = sin(uv.y * frequency + time * 3.0) * amplitude;
    uv.x += wave;

    // Sample the texture with distorted coordinates
    vec4 texelColor = texture2D(texture0, uv);

    // Apply vertex color and diffuse color
    gl_FragColor = texelColor * colDiffuse * fragColor;
}
