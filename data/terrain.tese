#version 450 core

layout(quads, equal_spacing, ccw) in;

layout(location = 0) in vec2 outTerrainLookup[];
layout(location = 1) in int tessLevelInner[];

layout(location = 1) out vec2 texCoord;

layout(std140) uniform SceneInputData {
    mat4 projectionView;
    vec3 cameraPosition;
    vec3 directionLight;
    float ambientLight;
};

uniform float waterHeight;

layout(binding = 0) uniform sampler2D heightmap;

void main() {
    vec4 position = mix(mix(gl_in[0].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x),
        mix(gl_in[1].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x),
        gl_TessCoord.y);

    vec2 lookup = mix(mix(outTerrainLookup[0], outTerrainLookup[3], gl_TessCoord.x),
        mix(outTerrainLookup[1], outTerrainLookup[2], gl_TessCoord.x),
        gl_TessCoord.y);

    position.y = texture(heightmap, lookup).r * 10;
    if (position.y < waterHeight) {
        position.y = waterHeight - 0.0001;
    }

    gl_Position = position;
    texCoord = gl_TessCoord.xy;
}