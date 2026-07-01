#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;
layout(location = 5) in ivec4 boneIds;
layout(location = 6) in vec4 weights;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

const int MAX_BONES = 200;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

out vec2 TexCoords;
out vec3 FragPos;
out mat3 TBN;

void main()
{
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    vec3 totalTangent = vec3(0.0f);

    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    {
        if(boneIds[i] == -1)
            continue;
        if(boneIds[i] >= MAX_BONES)
        {
            totalPosition = vec4(pos, 1.0f);
            totalNormal = norm;
            totalTangent = tangent;
            break;
        }

        vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(pos, 1.0f);
        totalPosition += localPosition * weights[i];

        vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * norm;
        totalNormal += localNormal * weights[i];

        vec3 localTangent = mat3(finalBonesMatrices[boneIds[i]]) * tangent;
        totalTangent += localTangent * weights[i];
    }

    FragPos = vec3(model * totalPosition);
    TexCoords = tex;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * totalTangent);
    vec3 N = normalize(normalMatrix * totalNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    gl_Position = projection * view * model * totalPosition;
}