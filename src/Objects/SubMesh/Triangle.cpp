#include "Triangle.h"

// Inicializa todo en (0, 0, 0)
Triangle::Triangle() 
    : v1(0.0f), v2(0.0f), v3(0.0f), vn1(0.0f), vn2(0.0f), vn3(0.0f), vt1(0.0f), vt2(0.0f), vt3(0.0f) {}

Triangle::Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, 
                   glm::vec2 vt1, glm::vec2 vt2, glm::vec2 vt3,
                   glm::vec3 vn1, glm::vec3 vn2, glm::vec3 vn3) {
    this->v1 = v1;
    this->v2 = v2;
    this->v3 = v3;
    this->vn1 = vn1;
    this->vn2 = vn2;
    this->vn3 = vn3;
    this->vt1 = vt1;
    this->vt2 = vt2;
    this->vt3 = vt3;
}

// Sin repetir los valores por defecto en el .cpp
Triangle::Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
    glm::vec3 vn1, glm::vec3 vn2, glm::vec3 vn3) {
    this->v1 = v1;
    this->v2 = v2;
    this->v3 = v3;
    this->vn1 = vn1;
    this->vn2 = vn2;
    this->vn3 = vn3;
}