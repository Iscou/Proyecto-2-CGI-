#pragma once
#include <glm/glm.hpp>

class Triangle {
public:
    // Vertices (x, y, z)
    glm::vec3 v1, v2, v3;
    // Vectores normales de cada vertice (nx, ny, nz)
    glm::vec3 vn1, vn2, vn3;

    // Coordenads UV
    glm::vec2 uv1, uv2, uv3;

    // Tangente 
    glm::vec3 tan1{0.0f, 0.0f, 0.0f}, tan2{0.0f, 0.0f, 0.0f}, tan3{0.0f, 0.0f, 0.0f};
    // Bitangente
    glm::vec3 bitan1{0.0f, 0.0f, 0.0f}, bitan2{0.0f, 0.0f, 0.0f}, bitan3{0.0f, 0.0f, 0.0f};

    // Constructores 
    Triangle();
    Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, 
             glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3,
             glm::vec3 vn1 = glm::vec3(0.0f), 
             glm::vec3 vn2 = glm::vec3(0.0f), 
             glm::vec3 vn3 = glm::vec3(0.0f));

    // Calculo de tangentes y bitangentres de cada vertice
    void calcularTangentesBitangentes();
};