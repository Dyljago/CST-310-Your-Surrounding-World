#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoord;

// Uniforms for lighting and material properties
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform float objectAlpha;

uniform sampler2D texture1; 
uniform bool useTexture; 
uniform bool brighter;

void main()
{
    // Ambient lighting
    float ambientStrength = 0.2;
    if (brighter) {
        ambientStrength = 0.4;
    }
    vec3 ambient = ambientStrength * lightColor;
  
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  

    // Combine the lighting effects
    vec3 finalColor;
    if (useTexture) {
        // Check if texture coordinates are 0.0f
        if (TexCoord.x == 0.0f && TexCoord.y == 1.0f) {
            // Set fragment color to a solid color
            finalColor = (ambient + diffuse + specular) * objectColor;
        } else {
            vec3 texColor = texture(texture1, TexCoord).rgb;
            finalColor = (ambient + diffuse + specular) * texColor;
        }
    } else {
        finalColor = (ambient + diffuse + specular) * (objectColor);
    }

    // Output the final color with the appropriate alpha
    FragColor = vec4(finalColor, objectAlpha);
}
