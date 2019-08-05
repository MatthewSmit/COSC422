#version 450 core

layout (location = 0) in vec4 position;

layout(std140) uniform ModelInputData {
    mat4 world;
};

void main() {
    gl_Position = world * position;
}