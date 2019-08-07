#version 450 core

layout(quads, equal_spacing, ccw) in;

layout(location = 0) in vec2 outTerrainLookup[];

layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 texCoord;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

layout(binding = 0) uniform sampler2D heightmap;

void main() {
    vec4 position = mix(mix(gl_in[0].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x),
        mix(gl_in[1].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x),
        gl_TessCoord.y);

    vec2 lookup = mix(mix(outTerrainLookup[0], outTerrainLookup[3], gl_TessCoord.x),
        mix(outTerrainLookup[1], outTerrainLookup[2], gl_TessCoord.x),
        gl_TessCoord.y);

    position.y = texture(heightmap, lookup).r * 10;

    const vec2 size = vec2(0.01, 0.0);
    const ivec3 offset = ivec3(-1, 0, 1);

    float s01 = textureOffset(heightmap, lookup, offset.xy).r;
    float s21 = textureOffset(heightmap, lookup, offset.zy).r;
    float s10 = textureOffset(heightmap, lookup, offset.yx).r;
    float s12 = textureOffset(heightmap, lookup, offset.yz).r;

    vec3 va = normalize(vec3(size.x, s21 - s01, size.y));
    vec3 vb = normalize(vec3(size.y, s12 - s10, size.x));
    normal = normalize(cross(vb, va));
    normal.x = 1 - normal.x;

    gl_Position = projectionView * position;
    texCoord = gl_TessCoord.xy;
}