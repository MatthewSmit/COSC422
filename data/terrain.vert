#version 450 core

layout (location = 0) in vec4 position;
layout (location = 0) out vec2 textureLookup;

layout(binding = 0) uniform sampler2D heightmap;

void main() {
    textureLookup = position.zw;
    gl_Position = vec4(position.x, texture(heightmap, textureLookup).r * 10, position.y, 1);
}