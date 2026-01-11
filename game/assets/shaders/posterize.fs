#version 330

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform float levels;  // Number of color levels (2.0 to 16.0 recommended)

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);

    // Posterize by reducing color levels
    vec3 posterized = floor(texelColor.rgb * levels + 0.5) / levels;

    finalColor = vec4(posterized, texelColor.a) * colDiffuse * fragColor;
}
