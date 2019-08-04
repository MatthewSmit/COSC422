#version 450 core

layout(quads, equal_spacing, ccw) in;

layout(location = 0) out vec3 normal;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

vec4 bezierTangent(vec4 p1, vec4 p2, vec4 p3, vec4 p4, float t) {
    return normalize(p1 * (-1 + 2 * t - t * t) +
        p2 * (1 - 4 * t + 3 * t * t) +
        p3 * (2 * t - 3 * t * t) +
        p4 * (t * t));
}

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    float Au = (1 - u) * (1 - u) * (1 - u);
    float Bu = 3 * u * (1 - u) * (1 - u);
    float Cu = 3 * u * u * (1 - u);
    float Du = u * u * u;

    float Av = (1 - v) * (1 - v) * (1 - v);
    float Bv = 3 * v * (1 - v) * (1 - v);
    float Cv = 3 * v * v * (1 - v);
    float Dv = v * v * v;

    vec4 p1 = Av * gl_in[0].gl_Position + Bv * gl_in[1].gl_Position + Cv * gl_in[2].gl_Position + Dv * gl_in[3].gl_Position;
    vec4 p2 = Av * gl_in[4].gl_Position + Bv * gl_in[5].gl_Position + Cv * gl_in[6].gl_Position + Dv * gl_in[7].gl_Position;
    vec4 p3 = Av * gl_in[8].gl_Position + Bv * gl_in[9].gl_Position + Cv * gl_in[10].gl_Position + Dv * gl_in[11].gl_Position;
    vec4 p4 = Av * gl_in[12].gl_Position + Bv * gl_in[13].gl_Position + Cv * gl_in[14].gl_Position + Dv * gl_in[15].gl_Position;

    vec4 position = Au * p1 + Bu * p2 + Cu * p3 + Du * p4;

    vec4 q1 = Au * gl_in[0].gl_Position + Bu * gl_in[4].gl_Position + Cu * gl_in[8].gl_Position + Du * gl_in[12].gl_Position;
    vec4 q2 = Au * gl_in[1].gl_Position + Bu * gl_in[5].gl_Position + Cu * gl_in[9].gl_Position + Du * gl_in[13].gl_Position;
    vec4 q3 = Au * gl_in[2].gl_Position + Bu * gl_in[6].gl_Position + Cu * gl_in[10].gl_Position + Du * gl_in[14].gl_Position;
    vec4 q4 = Au * gl_in[3].gl_Position + Bu * gl_in[7].gl_Position + Cu * gl_in[11].gl_Position + Du * gl_in[15].gl_Position;

    vec4 tangentA = bezierTangent(p1, p2, p3, p4, u);
    vec4 tangentB = bezierTangent(q1, q2, q3, q4, v);

    gl_Position = projectionView * position;
    normal = normalize(cross(tangentA.xyz, tangentB.xyz));
}