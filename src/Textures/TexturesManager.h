#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

struct TexturaInfo {
    std::string rutaDifusa; 
    std::string rutaNormal;
    bool tieneTextura = false;      
    bool tieneNormalMap;
    float ancho = 1.0f;
    float alto = 1.0f;
};

GLuint cargarTextura(const std::string& ruta);
