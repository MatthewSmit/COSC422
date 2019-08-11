#version 450 core

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 texCoord;

layout(location = 0) out vec4 outColour;

uniform float waterHeight;
uniform float snowHeight;

layout(binding = 1) uniform sampler2D grass;
layout(binding = 2) uniform sampler2D snow;
layout(binding = 3) uniform sampler2D water;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

void main() {
    vec3 baseColour = vec3(0);
    if (texCoord.z <= waterHeight) {
        baseColour = texture(water, texCoord.xy).rgb;
    } else if (texCoord.z > snowHeight + 0.5) {
        baseColour = texture(snow, texCoord.xy).rgb;
    } else if (texCoord.z > snowHeight - 0.5) {
        baseColour = mix(texture(grass, texCoord.xy).rgb, texture(snow, texCoord.xy).rgb, texCoord.z - snowHeight + 0.5);
    } else if(texCoord.z > waterHeight) {
        baseColour = texture(grass, texCoord.xy).rgb;
    }
    float lighting = clamp(ambientLight + clamp(dot(directionLight, normal), 0, 1), 0, 1);
    outColour = vec4(lighting * baseColour, 1);
}