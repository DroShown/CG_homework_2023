#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform vec3 light;
uniform vec3 color;

void main()
{
    float angle = 0.25 * dot(normalize(Normal), normalize(light)) + 0.5;
    vec3 finalColor = color * angle;
    FragColor = vec4(finalColor,1.0);
}