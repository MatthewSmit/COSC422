#version 450 core

layout(location = 0) in vec3 normal;

layout(location = 0) out vec4 colour;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

void main() {
    const vec3 baseColour = vec3(1);
    float lighting = clamp(ambientLight + clamp(dot(directionLight, normal), 0, 1), 0, 1);
    colour = vec4(lighting * baseColour, 1);
}