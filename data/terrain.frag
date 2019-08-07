#version 450 core

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColour;

layout(binding = 0) uniform sampler2D heightmap;

void main() {
    outColour = vec4(normal, 1);
//    outColour = vec4(0.5, 0.5, 0.5, 1);
}