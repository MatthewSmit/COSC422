#version 450 core

layout(triangles) in;
layout(location = 1) in vec2 inTexCoord[];

layout(triangle_strip, max_vertices=3) out;
layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outTexCoord;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

void main() {
    vec3 normal = -normalize(cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz));

    for (int i = 0; i < 3; i++) {
        gl_Position = projectionView * gl_in[i].gl_Position;
        outNormal = normal;
        outTexCoord = vec3(inTexCoord[i], gl_in[i].gl_Position.y);
        EmitVertex();
    }

    EndPrimitive();
}