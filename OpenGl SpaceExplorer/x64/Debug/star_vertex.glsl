#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in float brightness;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT {
    flat float Brightness;
} vs_out;

void main() {
    vs_out.Brightness = brightness;
    gl_Position = projection * view * model * vec4(position, 1.0);
}