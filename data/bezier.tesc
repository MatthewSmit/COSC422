#version 400 core

layout(vertices = 9) out;

void main() {
    gl_TessLevelInner[0] = 8;
    gl_TessLevelInner[1] = 8;
    gl_TessLevelOuter[0] = 8;
    gl_TessLevelOuter[1] = 8;
    gl_TessLevelOuter[2] = 8;
    gl_TessLevelOuter[3] = 8;

    if (gl_InvocationID == 0) gl_out[gl_InvocationID].gl_Position = gl_in[0].gl_Position;
    if (gl_InvocationID == 1) gl_out[gl_InvocationID].gl_Position = vec4((gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz) * 0.7, 1);
    if (gl_InvocationID == 2) gl_out[gl_InvocationID].gl_Position = gl_in[1].gl_Position;
    if (gl_InvocationID == 3) gl_out[gl_InvocationID].gl_Position = vec4((gl_in[0].gl_Position.xyz + gl_in[3].gl_Position.xyz) * 0.7, 1);
    if (gl_InvocationID == 4) gl_out[gl_InvocationID].gl_Position = vec4((gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz + gl_in[3].gl_Position.xyz) * 0.5, 1);
    if (gl_InvocationID == 5) gl_out[gl_InvocationID].gl_Position = vec4((gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz) * 0.7, 1);
    if (gl_InvocationID == 6) gl_out[gl_InvocationID].gl_Position = gl_in[3].gl_Position;
    if (gl_InvocationID == 7) gl_out[gl_InvocationID].gl_Position = vec4((gl_in[2].gl_Position.xyz + gl_in[3].gl_Position.xyz) * 0.7, 1);
    if (gl_InvocationID == 8) gl_out[gl_InvocationID].gl_Position = gl_in[2].gl_Position;
}