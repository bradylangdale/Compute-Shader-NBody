#version 430

// Game of Life rendering shader
// Just renders the content of the ssbo at binding 1 to screen

#define GOL_WIDTH 768

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;

// Output fragment color
out vec4 finalColor;

// Input game of life grid.
layout(std430, binding = 1) readonly buffer golLayout
{
    vec3 golBuffer[];
};

// Output resolution
uniform vec2 resolution;

void main()
{
    ivec2 coords = ivec2(fragTexCoord*resolution);

    vec3 value = golBuffer[coords.x + coords.y*uvec2(resolution).x];
    finalColor = vec4(value.x, value.y, value.z, 1.0);
}
