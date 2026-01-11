#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform float intensity;  // 0.0 = full color, 1.0 = full grayscale

// Output fragment color
out vec4 finalColor;

void main()
{
    // Sample the texture
    vec4 texelColor = texture(texture0, fragTexCoord);

    // Calculate grayscale using luminance weights
    float gray = dot(texelColor.rgb, vec3(0.299, 0.587, 0.114));

    // Mix between original color and grayscale based on intensity
    vec3 result = mix(texelColor.rgb, vec3(gray), intensity);

    // Apply vertex color, diffuse color, and preserve alpha
    finalColor = vec4(result, texelColor.a) * colDiffuse * fragColor;
}
