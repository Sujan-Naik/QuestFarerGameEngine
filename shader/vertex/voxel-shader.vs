#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in vec2 aAtlasTile;

out vec3 Normal;
out vec2 TexCoords;
flat out vec2 AtlasTile;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Normal = normalize(
        mat3(transpose(inverse(model))) * aNormal
    );

    TexCoords = aTexCoords;

    // Critical: do not interpolate the atlas tile.
    AtlasTile = aAtlasTile;

    gl_Position =
        projection *
        view *
        model *
        vec4(aPos, 1.0);
}