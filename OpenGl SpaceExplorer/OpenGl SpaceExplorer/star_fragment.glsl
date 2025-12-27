#version 410 core

in VS_OUT {
    flat float Brightness;
} fs_in;

uniform vec3 baseColor;

out vec4 FragColor;

void main() {
    // Stars are just bright points - multiply by brightness
    FragColor = vec4(baseColor * fs_in.Brightness, 1.0);
}