#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

// samplerCube en lugar del sampler2D normal
uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}