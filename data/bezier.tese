#version 450 core

layout(quads, equal_spacing, ccw) in;

layout(location = 0) out vec3 normal;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

vec3 bezierTangent(vec3 p1, vec3 p2, vec3 p3, vec3 p4, float t) {
//    t = clamp(t, 0.01, 0.99);
    return normalize(3 * (1 - t) * (1 - t) * (p2 - p1) + 6 * (1 - t) * t * (p3 - p2) + 3 * t * t * (p4 - p3));
}

vec4 calculatePosition() {
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

    return projectionView * (Au * p1 + Bu * p2 + Cu * p3 + Du * p4);
}

vec3 calculateNormal() {
    float u = clamp(gl_TessCoord.x, 0.00001, 0.99999);
    float v = clamp(gl_TessCoord.y, 0.00001, 0.99999);

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

    vec4 q1 = Au * gl_in[0].gl_Position + Bu * gl_in[4].gl_Position + Cu * gl_in[8].gl_Position + Du * gl_in[12].gl_Position;
    vec4 q2 = Au * gl_in[1].gl_Position + Bu * gl_in[5].gl_Position + Cu * gl_in[9].gl_Position + Du * gl_in[13].gl_Position;
    vec4 q3 = Au * gl_in[2].gl_Position + Bu * gl_in[6].gl_Position + Cu * gl_in[10].gl_Position + Du * gl_in[14].gl_Position;
    vec4 q4 = Au * gl_in[3].gl_Position + Bu * gl_in[7].gl_Position + Cu * gl_in[11].gl_Position + Du * gl_in[15].gl_Position;

    vec3 tangentA = bezierTangent(p1.xyz, p2.xyz, p3.xyz, p4.xyz, u);
    vec3 tangentB = bezierTangent(q1.xyz, q2.xyz, q3.xyz, q4.xyz, v);

    return normalize(cross(tangentA, tangentB));
}

void main() {
    gl_Position = calculatePosition();
    normal = calculateNormal();
}