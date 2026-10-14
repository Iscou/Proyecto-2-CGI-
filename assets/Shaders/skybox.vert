#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    // Las coordenadas de textura del cielo son exactamente sus posiciones 3D
    TexCoords = aPos;
    
    // Calculamos la posicion en pantalla
    vec4 pos = projection * view * vec4(aPos, 1.0);
    
    // El truco maestro: Forzamos que Z sea igual a W
    gl_Position = pos.xyww;
}