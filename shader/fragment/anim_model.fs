#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main()
{
    vec3 diffuseColor = texture(texture_diffuse1, TexCoords).rgb;
    FragColor = vec4(diffuseColor, 1.0);
}