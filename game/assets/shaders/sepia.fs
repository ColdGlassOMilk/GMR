#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform float intensity;  // 0.0 = full color, 1.0 = full sepia

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);

    // Sepia tone matrix
    vec3 sepia;
    sepia.r = dot(texelColor.rgb, vec3(0.393, 0.769, 0.189));
    sepia.g = dot(texelColor.rgb, vec3(0.349, 0.686, 0.168));
    sepia.b = dot(texelColor.rgb, vec3(0.272, 0.534, 0.131));

    // Mix based on intensity
    vec3 result = mix(texelColor.rgb, sepia, intensity);

    finalColor = vec4(result, texelColor.a) * colDiffuse * fragColor;
}
