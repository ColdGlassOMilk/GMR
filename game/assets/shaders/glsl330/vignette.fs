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
uniform float radius;     // Inner radius before darkening starts (0.0 to 1.0)
uniform float softness;   // How soft the edge is (0.0 to 1.0)

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);

    // Distance from center
    vec2 center = fragTexCoord - vec2(0.5);
    float dist = length(center) * 1.414;  // Normalize to corners

    // Smooth vignette
    float vignette = smoothstep(radius, radius + softness, dist);
    vignette = 1.0 - vignette;

    texelColor.rgb *= vignette;

    finalColor = texelColor * colDiffuse * fragColor;
}
