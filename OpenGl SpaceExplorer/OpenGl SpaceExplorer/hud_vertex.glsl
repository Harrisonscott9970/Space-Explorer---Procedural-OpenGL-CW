#version 410 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

uniform mat4 projection;

out VS_OUT {
    vec3 Color;
} vs_out;

void main() {
    vs_out.Color = color;
    gl_Position = projection * vec4(position, 0.0, 1.0);
}