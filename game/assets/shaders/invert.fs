#version 100

precision mediump float;

// Input from vertex shader
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values (raylib defaults)
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Custom uniforms
uniform float intensity;  // 0.0 = normal, 1.0 = fully inverted

void main()
{
    vec4 texelColor = texture2D(texture0, fragTexCoord);

    // Invert colors
    vec3 inverted = vec3(1.0) - texelColor.rgb;

    // Mix based on intensity
    vec3 result = mix(texelColor.rgb, inverted, intensity);

    gl_FragColor = vec4(result, texelColor.a) * colDiffuse * fragColor;
}
