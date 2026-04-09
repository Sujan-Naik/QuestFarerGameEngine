#version 330 core
in vec3 Normal;

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(norm, lightDir), 0.3);

    vec3 color = vec3(0.5, 0.7, 0.4);  // Default green
    FragColor = vec4(color * diff, 1.0);
}