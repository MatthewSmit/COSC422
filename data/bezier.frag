#version 450 core

layout(location = 0) in vec3 normal;

layout(location = 0) out vec4 colour;

void main() {
    colour = vec4(normal.xyz, 1);
}