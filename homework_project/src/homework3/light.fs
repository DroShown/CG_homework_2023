#version 330 core
in vec2 TexCoords;

layout (location = 0) out vec4 FragColor;

uniform vec3 lightColor;
uniform float alpha;
uniform int mode;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform sampler2D texture_diffuse3;

void main()
{
    if(mode == 0)
        FragColor = vec4(lightColor, 1.0);
    else if(mode == 1)
    {
        FragColor = vec4(lightColor, alpha) * texture(texture_diffuse1, TexCoords);
    }
    else if(mode == 2)
    {
        FragColor = vec4(lightColor, alpha) * texture(texture_diffuse2, TexCoords);
    }
    else if(mode == 3)
    {
        FragColor = vec4(lightColor, alpha) * texture(texture_diffuse3, TexCoords);
    }


}