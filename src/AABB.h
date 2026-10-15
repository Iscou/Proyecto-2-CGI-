#pragma once
#include <glm/glm.hpp>

// AABB Global para colisiones  
struct AABB {
    glm::vec3 min; // Guardamos las coordenadas min y max del BBox del mesh para que el procesador tambien pueda procesarlas en tiempo real y validar colisiones entre objetos dinamicos con los estaticos
    glm::vec3 max; // y dinamicos con dinamicos
};