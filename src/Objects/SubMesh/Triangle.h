#pragma once
#include <glm/glm.hpp>

class Triangle {
public:
    // Vértices (x, y, z)
    glm::vec3 v1, v2, v3;
    // Vectores normales de cada vértice (nx, ny, nz)
    glm::vec3 vn1, vn2, vn3;
    // Vectores de textura (u, v)
    glm::vec2 vt1, vt2, vt3;

    // Tangente
    glm::vec3 tan1{0.0f, 0.0f, 0.0f}, tan2{0.0f, 0.0f, 0.0f}, tan3{0.0f, 0.0f, 0.0f};
    // Bitangente
    glm::vec3 bitan1{0.0f, 0.0f, 0.0f}, bitan2{0.0f, 0.0f, 0.0f}, bitan3{0.0f, 0.0f, 0.0f};

    // Constructores 
    Triangle();
    
    // Constructor completo con texturas
    Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
             glm::vec2 vt1, glm::vec2 vt2, glm::vec2 vt3,
             glm::vec3 vn1 = glm::vec3(0.0f), 
             glm::vec3 vn2 = glm::vec3(0.0f), 
             glm::vec3 vn3 = glm::vec3(0.0f));
             
    // Constructor simple 
    Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
            glm::vec3 vn1 = glm::vec3(0.0f),
            glm::vec3 vn2 = glm::vec3(0.0f),
            glm::vec3 vn3 = glm::vec3(0.0f));

    // Cálculo de tangentes y bitangentes
    void calcularTangentesBitangentes();
};