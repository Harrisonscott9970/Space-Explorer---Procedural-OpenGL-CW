#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

/* === Procedural === */
uniform float surfaceNoise;   // 0.0 – 1.0

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} vs_out;

void main()
{
    /* --- Procedural vertex displacement --- */
    float displacementStrength = 0.25; // keep subtle
    vec3 displacedPos = position + normal * surfaceNoise * displacementStrength;

    vec4 worldPos = model * vec4(displacedPos, 1.0);
    vs_out.FragPos = worldPos.xyz;

    /* Correct normal transform */
    vs_out.Normal = mat3(transpose(inverse(model))) * normal;
    vs_out.TexCoord = texCoord;

    gl_Position = projection * view * worldPos;
}
