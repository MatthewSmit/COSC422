#version 450 core

layout (location = 0) in vec4 position;

layout (location = 0) out float distanceToCamera;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

layout(std140) uniform ModelInputData {
    mat4 world;
};

void main() {
    gl_Position = world * position;

    distanceToCamera = distance(cameraPosition, gl_Position.xzy);
}