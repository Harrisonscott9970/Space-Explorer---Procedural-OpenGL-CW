#version 410 core

in VS_OUT {
    vec3 Color;
} fs_in;

out vec4 FragColor;

void main() {
    FragColor = vec4(fs_in.Color, 1.0);
}