#version 450 core

layout (location = 0) in float distanceToCamera[];

layout(vertices = 16) out;

void main() {
    if (gl_InvocationID == 0) {
        const float D_MIN = 10;
        const float D_MAX = 100;

        const int L_LOW = 20;
        const int L_HIGH = 2;

        int level = int(round(((distanceToCamera[0] - D_MIN) / (D_MAX - D_MIN)) * (L_LOW - L_HIGH) + L_HIGH));
        gl_TessLevelInner[0] = level;
        gl_TessLevelInner[1] = level;
        gl_TessLevelOuter[0] = level;
        gl_TessLevelOuter[1] = level;
        gl_TessLevelOuter[2] = level;
        gl_TessLevelOuter[3] = level;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}