#version 330 core
in vec2 TexCoords;
in vec3 Normal;
out vec4 FragColor;

uniform sampler2D texture_diffuse1; // Your Atlas

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(Normal), lightDir), 0.3);

    // The TexCoords now handle the "which block" logic automatically
    vec4 texColor = texture(texture_diffuse1, TexCoords);

    if(texColor.a < 0.1) discard;
    FragColor = texColor * diff;
}