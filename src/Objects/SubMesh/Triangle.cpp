#include "Triangle.h"

// Inicializa todo en (0, 0, 0)
Triangle::Triangle() 
    : v1(0.0f), v2(0.0f), v3(0.0f), 
      uv1(0.0f), uv2(0.0f), uv3(0.0f),
      vn1(0.0f), vn2(0.0f), vn3(0.0f), 
      tan1(0.0f), tan2(0.0f), tan3(0.0f),
      bitan1(0.0f), bitan2(0.0f), bitan3(0.0f) {}

// Sin repetir los valores por defecto en el .cpp
Triangle::Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, 
                   glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3,
                   glm::vec3 vn1, glm::vec3 vn2, glm::vec3 vn3) {
    this->v1 = v1;
    this->v2 = v2;
    this->v3 = v3;
    
    this->uv1 = uv1;
    this->uv2 = uv2;
    this->uv3 = uv3;
    
    this->vn1 = vn1;
    this->vn2 = vn2;
    this->vn3 = vn3;

    this->tan1 = glm::vec3(0.0f);
    this->tan2 = glm::vec3(0.0f);
    this->tan3 = glm::vec3(0.0f);
    
    this->bitan1 = glm::vec3(0.0f);
    this->bitan2 = glm::vec3(0.0f);
    this->bitan3 = glm::vec3(0.0f);
}

// Calculo de tangentes y bitangentres de cada vertice
void Triangle::calcularTangentesBitangentes() {
    // Calculamos la diferencias entre los vertices de los triangulos
    glm::vec3 edge1 = v2 - v1;
    glm::vec3 edge2 = v3 - v1;

    // Ahora entre las coords UV (del texturizado)
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    // Calculamos la inversa del determinante de la matriz UV
    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    glm::vec3 tangent, bitangent;

    // Resolvemos un SEL para X, Y y Z
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

    // Normalizamos
    tangent = glm::normalize(tangent);
    bitangent = glm::normalize(bitangent);

    // Como la cara de este triángulo es plana, los 3 vértices comparten la misma Tangente y la misma Bitangente.
    tan1 = tan2 = tan3 = tangent;
    bitan1 = bitan2 = bitan3 = bitangent;
}
