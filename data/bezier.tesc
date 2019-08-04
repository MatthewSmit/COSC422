#version 450 core

layout(vertices = 16) out;

void main() {
    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = 2;
        gl_TessLevelInner[1] = 2;
        gl_TessLevelOuter[0] = 2;
        gl_TessLevelOuter[1] = 2;
        gl_TessLevelOuter[2] = 2;
        gl_TessLevelOuter[3] = 2;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}