#version 400 core

layout(quads, equal_spacing, ccw) in;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
};

layout(std140) uniform ModelInputData {
    mat4 world;
};

void main() {
    mat4 worldProjectionView = world * projectionView;

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    float iu2 = (1 - u) * (1 - u);
    float iv2 = (1 - v) * (1 - v);
    float _2viv = 2 * v * (1 - v);
    float _2iuu = 2 * (1 - u) * u;
    float u2 = u * u;
    float v2 = v * v;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p02 = gl_in[2].gl_Position;
    vec4 p10 = gl_in[3].gl_Position;
    vec4 p11 = gl_in[4].gl_Position;
    vec4 p12 = gl_in[5].gl_Position;
    vec4 p20 = gl_in[6].gl_Position;
    vec4 p21 = gl_in[7].gl_Position;
    vec4 p22 = gl_in[8].gl_Position;

    vec4 position = iu2 * (iv2 * p00 + _2viv * p10 + v2 * p20)
    + _2iuu * (iv2 * p01 + _2viv * p11 + v2 * p21)
    + u2 * (iv2 * p02 + _2viv * p12 + v2 * p22);

    gl_Position = worldProjectionView * position;
}