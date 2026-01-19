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
uniform float intensity;  // 0.0 = normal, 1.0 = fully inverted

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);

    // Invert colors
    vec3 inverted = vec3(1.0) - texelColor.rgb;

    // Mix based on intensity
    vec3 result = mix(texelColor.rgb, inverted, intensity);

    finalColor = vec4(result, texelColor.a) * colDiffuse * fragColor;
}
