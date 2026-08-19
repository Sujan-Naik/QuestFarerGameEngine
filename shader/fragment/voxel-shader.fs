#version 330 core

in vec2 TexCoords;
flat in vec2 AtlasTile;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D texture_diffuse1;

const float ATLAS_TILE_SIZE = 1.0 / 16.0;

void main()
{
    // TexCoords are local coordinates across the greedy quad.
    //
    // Example:
    //   4-block-wide quad -> x goes from 0 to 4
    //
    // fract() makes the selected texture repeat every block.
    vec2 localUV = fract(TexCoords);

    // Move local UV into the selected 64x64 atlas tile.
    vec2 atlasUV =
            (AtlasTile + localUV) *
            ATLAS_TILE_SIZE;

    vec4 texColor =
            texture(
                    texture_diffuse1,
                    atlasUV
            );

    if (texColor.a < 0.1) {
        discard;
    }

    vec3 lightDir =
            normalize(
                    vec3(
                            0.5,
                            1.0,
                            0.3
                    )
            );

    float diff =
            max(
                    dot(
                            normalize(Normal),
                            lightDir
                    ),
                    0.3
            );

    FragColor =
            vec4(
                    texColor.rgb * diff,
                    texColor.a
            );
}