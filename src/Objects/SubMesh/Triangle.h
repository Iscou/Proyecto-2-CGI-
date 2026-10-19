#pragma once
#include <glm/glm.hpp>

class Triangle {
public:
    // Vertices (x, y, z)
    glm::vec3 v1, v2, v3;
    // Vectores normales de cada vertice (nx, ny, nz)
    glm::vec3 vn1, vn2, vn3;
	// Vectores textura de cada vertice (u, v)
	glm::vec2 vt1, vt2, vt3;

    // Constructores 
    Triangle();
    Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
             glm::vec2 vt1, glm::vec2 vt2, glm::vec2 vt3,
             glm::vec3 vn1 = glm::vec3(0.0f), 
             glm::vec3 vn2 = glm::vec3(0.0f), 
             glm::vec3 vn3 = glm::vec3(0.0f));
    Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
            glm::vec3 vn1 = glm::vec3(0.0f),
            glm::vec3 vn2 = glm::vec3(0.0f),
            glm::vec3 vn3 = glm::vec3(0.0f));
};