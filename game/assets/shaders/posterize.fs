#version 100

precision mediump float;

// Input from vertex shader
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform float levels;  // Number of color levels (2.0 to 16.0 recommended)

void main()
{
    vec4 texelColor = texture2D(texture0, fragTexCoord);

    // Posterize by reducing color levels
    vec3 posterized = floor(texelColor.rgb * levels + 0.5) / levels;

    gl_FragColor = vec4(posterized, texelColor.a) * colDiffuse * fragColor;
}
