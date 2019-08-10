#version 450 core

layout(location = 0) in vec2 terrainLookup[];

layout(vertices = 4) out;
layout(location = 0) out vec2 outTerrainLookup[];
layout(location = 1) out int tessLevelInner[];

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

int calculateTesselation(vec3 position) {
    const float D_MIN = 25;
    const float D_MAX = 125;

    const int L_LOW = 20;
    const int L_HIGH = 2;

    float distanceToCamera = distance(cameraPosition, position);
    float x = clamp(1 - (distanceToCamera - D_MIN) / (D_MAX - D_MIN), 0, 1);
    return int(round(x * (L_LOW - L_HIGH) + L_HIGH));
}

void main() {
    int level = calculateTesselation((gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz + gl_in[3].gl_Position.xyz) / 4);

    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = level;
        gl_TessLevelInner[1] = level;
        gl_TessLevelOuter[0] = calculateTesselation((gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz) / 2);
        gl_TessLevelOuter[1] = calculateTesselation((gl_in[0].gl_Position.xyz + gl_in[3].gl_Position.xyz) / 2);
        gl_TessLevelOuter[2] = calculateTesselation((gl_in[2].gl_Position.xyz + gl_in[3].gl_Position.xyz) / 2);
        gl_TessLevelOuter[3] = calculateTesselation((gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz) / 2);
    }

    tessLevelInner[gl_InvocationID] = level;
    outTerrainLookup[gl_InvocationID] = terrainLookup[gl_InvocationID];
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}