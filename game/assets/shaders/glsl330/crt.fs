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
uniform vec2 resolution;    // Screen resolution
uniform float curvature;    // Screen curvature amount (0.0 to 0.5)
uniform float scanlineIntensity;  // Scanline darkness (0.0 to 1.0)

vec2 curve(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / vec2(curvature + 0.001, curvature + 0.001);
    uv = uv + uv * offset * offset;
    uv = uv * 0.5 + 0.5;
    return uv;
}

void main()
{
    vec2 uv = curve(fragTexCoord);

    // Check bounds - black outside curved screen
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Sample texture
    vec4 texelColor = texture(texture0, uv);

    // Scanlines
    float scanline = sin(uv.y * resolution.y * 3.14159) * 0.5 + 0.5;
    scanline = pow(scanline, 1.5) * scanlineIntensity + (1.0 - scanlineIntensity);

    // Apply scanline
    texelColor.rgb *= scanline;

    // Slight vignette
    float vignette = uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
    vignette = clamp(pow(16.0 * vignette, 0.3), 0.0, 1.0);
    texelColor.rgb *= vignette;

    finalColor = texelColor * colDiffuse * fragColor;
}
