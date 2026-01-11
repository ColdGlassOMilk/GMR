#version 100

precision mediump float;

// Input from vertex shader
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform float radius;     // Inner radius before darkening starts (0.0 to 1.0)
uniform float softness;   // How soft the edge is (0.0 to 1.0)

void main()
{
    vec4 texelColor = texture2D(texture0, fragTexCoord);

    // Distance from center
    vec2 center = fragTexCoord - vec2(0.5);
    float dist = length(center) * 1.414;  // Normalize to corners

    // Smooth vignette
    float vignette = smoothstep(radius, radius + softness, dist);
    vignette = 1.0 - vignette;

    texelColor.rgb *= vignette;

    gl_FragColor = texelColor * colDiffuse * fragColor;
}
