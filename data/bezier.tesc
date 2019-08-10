#version 450 core

layout(vertices = 16) out;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

layout(std140) uniform ModelInputData {
    mat4 world;
    float time;
};

int calculateTesselation(vec3 position) {
    const float D_MIN = 10;
    const float D_MAX = 40;

    const int L_LOW = 20;
    const int L_HIGH = 2;

    float distanceToCamera = distance(cameraPosition, position);
    float x = clamp(1 - (distanceToCamera - D_MIN) / (D_MAX - D_MIN), 0, 1);
    return int(round(x * (L_LOW - L_HIGH) + L_HIGH));
}

void main() {
    const float INITIAL_VELOCITY = 2;
    const float GRAVITY = -0.98;

    vec4 centre = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position + gl_in[3].gl_Position +
        gl_in[4].gl_Position + gl_in[5].gl_Position + gl_in[6].gl_Position + gl_in[7].gl_Position +
        gl_in[8].gl_Position + gl_in[9].gl_Position + gl_in[10].gl_Position + gl_in[11].gl_Position +
        gl_in[12].gl_Position + gl_in[13].gl_Position + gl_in[14].gl_Position + gl_in[15].gl_Position) / 16;

    vec3 translate = normalize(centre.xyz);

    if (gl_InvocationID == 0) {
        int level = calculateTesselation((gl_in[5].gl_Position.xyz + gl_in[6].gl_Position.xyz + gl_in[9].gl_Position.xyz + gl_in[10].gl_Position.xyz) / 4);
        gl_TessLevelInner[0] = level;
        gl_TessLevelInner[1] = level;
        gl_TessLevelOuter[0] = calculateTesselation((gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz + gl_in[3].gl_Position.xyz) / 4);
        gl_TessLevelOuter[1] = calculateTesselation((gl_in[0].gl_Position.xyz + gl_in[4].gl_Position.xyz + gl_in[8].gl_Position.xyz + gl_in[12].gl_Position.xyz) / 4);
        gl_TessLevelOuter[2] = calculateTesselation((gl_in[12].gl_Position.xyz + gl_in[13].gl_Position.xyz + gl_in[14].gl_Position.xyz + gl_in[15].gl_Position.xyz) / 4);
        gl_TessLevelOuter[3] = calculateTesselation((gl_in[3].gl_Position.xyz + gl_in[7].gl_Position.xyz + gl_in[11].gl_Position.xyz + gl_in[15].gl_Position.xyz) / 4);
    }

//    vec3 controlPosition = gl_in[gl_InvocationID].gl_Position.xyz;
    vec3 controlPosition = centre.xyz;
    float velocity = INITIAL_VELOCITY * translate.y;
    float finalTime = -(velocity + sqrt(velocity * velocity - 2 * GRAVITY * controlPosition.y)) / GRAVITY;

    vec3 position = gl_in[gl_InvocationID].gl_Position.xyz;
    float realTime = clamp(time, 0, finalTime);
    position.y += velocity * realTime + 0.5 * GRAVITY * realTime * realTime;
    position.xz += translate.xz * realTime * 2;

    gl_out[gl_InvocationID].gl_Position = vec4(position, 1);
}