#version 400 core

layout (location = 0) in vec2 position;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
};

void main() {
    gl_Position = projectionView * vec4(position.x, 0, position.y, 1);
}