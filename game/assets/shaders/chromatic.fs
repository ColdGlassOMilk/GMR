#version 100

precision mediump float;

// Input from vertex shader
varying vec2 fragTexCoord;
varying vec4 fragColor;

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

    // Sample RGB channels with offset
    float r = texture2D(texture0, uv + dir * offset).r;
    float g = texture2D(texture0, uv).g;
    float b = texture2D(texture0, uv - dir * offset).b;
    float a = texture2D(texture0, uv).a;

    gl_FragColor = vec4(r, g, b, a) * colDiffuse * fragColor;
}
