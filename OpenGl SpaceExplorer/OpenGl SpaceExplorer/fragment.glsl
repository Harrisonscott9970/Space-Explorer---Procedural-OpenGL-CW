#version 410 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} fs_in;

out vec4 FragColor;

uniform vec3 baseColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float isEmissive;

uniform float planetSeed;   // keep ONE
uniform int planetType;
uniform float isAsteroid;

uniform sampler2D diffuseMap;
uniform vec3 noiseOffset;

uniform float scanHighlight; // 0 = none, 1 = target, >1 = scanning

/* ===== SIMPLE SMOOTH NOISE ===== */
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float smoothNoise(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

mat2 rot2(float a) {
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c);
}

void main() {
    if (isEmissive > 0.5) {
        FragColor = vec4(baseColor * 2.5, 1.0);
        return;
    }

    vec3 norm = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    vec3 viewDir  = normalize(viewPos - fs_in.FragPos);

    vec3 ambient  = 0.05 * baseColor;
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * vec3(1.0);

    vec3 halfDir  = normalize(lightDir + viewDir);
    float spec    = pow(max(dot(norm, halfDir), 0.0), 32.0);
    vec3 specular = 0.4 * spec * vec3(1.0);

    vec3 finalColor = baseColor;

    if (isAsteroid > 0.5) {
        finalColor = texture(diffuseMap, fs_in.TexCoord).rgb * 0.8;
    } else {
        vec2 uv = fs_in.TexCoord * 2.5;
        uv += noiseOffset.xy * 0.01;
        uv = rot2(planetSeed * 0.75) * uv;

        float continent = smoothNoise(uv);
        float detail    = smoothNoise(uv * 3.0 + noiseOffset.z * 0.01) * 0.15;
        float height    = continent + detail;

        float latitude = abs(fs_in.TexCoord.y - 0.5);

        float ocean    = smoothstep(0.35, 0.42, height);
        float mountain = smoothstep(0.68, 0.75, height);
        float ice      = smoothstep(0.42, 0.48, latitude);

        float desert   = smoothstep(0.65, 0.72,
                         smoothNoise(uv * 1.5 + noiseOffset.x * 0.01));

        if (planetType == 1) {
            // Lava crack pattern
            float cracks = smoothstep(0.55, 0.7, continent);

            // Dark crust vs bright lava
            vec3 crustColor = vec3(0.08, 0.02, 0.01);
            vec3 lavaColor  = vec3(1.2, 0.35, 0.05); // HDR-style brightness

            finalColor = mix(crustColor, lavaColor, cracks);

            // Make lava self-illuminating
            float glow = cracks * 1.5;
            vec3 lavaLit = ambient + diffuse * finalColor + specular + lavaColor * glow;

            // Scan highlight (green tint) for lava too
            if (scanHighlight > 0.0) {
                vec3 highlightColor = vec3(0.2, 1.0, 0.4);
                lavaLit = mix(lavaLit, highlightColor, clamp(scanHighlight, 0.0, 1.0));
            }

            FragColor = vec4(lavaLit, 1.0);
            return;
        }
        else if (ocean < 0.5)       finalColor = vec3(0.0, 0.15, 0.35);
        else if (ice > 0.75)        finalColor = vec3(0.85, 0.9, 0.95);
        else if (mountain > 0.7)    finalColor = vec3(0.4);
        else if (desert > 0.75)     finalColor = vec3(0.7, 0.65, 0.4);
        else                        finalColor = baseColor;
    }

    vec3 color = ambient + diffuse * finalColor + specular;

    // Scan highlight (green tint)
    if (scanHighlight > 0.0) {
        vec3 highlightColor = vec3(0.2, 1.0, 0.4);
        color = mix(color, highlightColor, clamp(scanHighlight, 0.0, 1.0));
    }

    FragColor = vec4(color, 1.0);
}
