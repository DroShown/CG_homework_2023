#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseTexture;
uniform samplerCube depthMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform float far_plane;
uniform bool shadows;

uniform int ballColor;

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

vec3 wallColors[7] = vec3[]
(
    vec3(0.8f, 0.59f, 0.51f),//left:2
    vec3(0.5f, 0.6f, 0.55f),//right:3
    vec3(0.65f, 0.7f, 0.5f),//bottom:4
    vec3(1.0f, 1.0f, 1.0f),//up:5
    vec3(0.52f, 0.47f, 0.60f),//back:6
    vec3(1.0f, 1.0f, 1.0f),//front:7
    vec3(0.9f,0.2f,0.1f)//doll:8
);

float ShadowCalculation(vec3 fragPos)
{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;
    // use the fragment to light vector to sample from the depth map    
    // float closestDepth = texture(depthMap, fragToLight).r;
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    // closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // test for shadows
    // float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    // float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;
    // PCF
    // float shadow = 0.0;
    // float bias = 0.05; 
    // float samples = 4.0;
    // float offset = 0.1;
    // for(float x = -offset; x < offset; x += offset / (samples * 0.5))
    // {
        // for(float y = -offset; y < offset; y += offset / (samples * 0.5))
        // {
            // for(float z = -offset; z < offset; z += offset / (samples * 0.5))
            // {
                // float closestDepth = texture(depthMap, fragToLight + vec3(x, y, z)).r; // use lightdir to lookup cubemap
                // closestDepth *= far_plane;   // Undo mapping [0;1]
                // if(currentDepth - bias > closestDepth)
                    // shadow += 1.0;
            // }
        // }
    // }
    // shadow /= (samples * samples * samples);
    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= far_plane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
        
    // display closestDepth as debug (to visualize depth cubemap)
    // FragColor = vec4(vec3(closestDepth / far_plane), 1.0);    
        
    return shadow;
}

void main()
{
    vec3 color = vec3(1.0f,1.0f,1.0f);
    float Ka = 0.4;
    float Kd = 1.0;
    float Ks = 1.2;
    float Ns = 250.0;
    if(ballColor == 0 || ballColor == 4) color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    else if(ballColor > 0)
    {
        color = wallColors[ballColor - 2];
        Ka = 0.2;
        Kd = 1.3;
        Ks = 1.2;
        Ns = 300.0;
    }
    else if(fs_in.TexCoords.x <= 1.0f) {
        color = texture(diffuseTexture, fs_in.TexCoords * 4.0f).rgb;
        Ka = 0.3;
        Kd = 0.9;
        Ks = 0.5;
//         Ns = 8.0;
    }
    else
    {
        color = wallColors[int(fs_in.TexCoords.x) - 2];
        Ka = 0.6;
        Kd = 1.5;
        Ks = 0.0;
        Ns = 8.0;
    }

    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.7);
    // ambient
    vec3 ambient = Ka * lightColor;
    // diffuse
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = Kd * diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), Ns);
    vec3 specular = Ks * spec * lightColor;
    // calculate shadow
    float shadow = (shadows && (fs_in.TexCoords.x != 5.0f)) ? ShadowCalculation(fs_in.FragPos) : 0.0;
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);
}