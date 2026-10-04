#pragma once
#include "Triangle.h"
#include <vector>
#include <string>
#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class SubMesh {
private:
    // Buffers de OpenGL para los triangulos de la submalla
    GLuint vao = 0;
    GLuint vbo = 0;
    int totalVertices = 0;

    // Buffers de OpenGL para dibujar las lineas de las normales
    GLuint vaoNormales = 0;
    GLuint vboNormales = 0;
    int totalVerticesNormales = 0;

public:
    int id; // ID unico para Color Picking
    std::string nombre;
    std::vector<Triangle> triangles;

    // Propiedades alterables cuando se selecciona en modo Local
    glm::vec4 color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f); // Color difuso (kd) + alfa
    glm::vec3 posicion = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 rotacion = glm::vec3(0.0f, 0.0f, 0.0f);    // Grados en X, Y, Z
    glm::vec3 escala   = glm::vec3(1.0f, 1.0f, 1.0f);

    // Constructores
    SubMesh(int id = 0, std::string nombre = "SubMesh");
    SubMesh(int id, const std::vector<Triangle>& tris, glm::vec4 colorDifuso = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));

    // Aproxima normales promediando las normales de las caras que comparten un vertice
    void calcularNormalesPromediadas();

    // Empaqueta los triangulos y las lineas de normales hacia la GPU (VAO/VBO)
    void subirAGPU();

    // Calcula la matriz de transformacion propia de esta submalla (T * R * S)
    glm::mat4 getMatrizLocal() const;

    //  Renderiza la submalla multiplicando la matriz del Objeto padre por la local
    void dibujar(GLuint shaderProgram, const glm::mat4& matrizPadre, 
                 bool verWireframe, bool verVertices, bool verNormales) const;

    // Libera la memoria de la tarjeta grafica al eliminar el objeto
    void limpiarGPU();

    GLuint getVAO() const { return vao; }
    int getTotalVertices() const { return totalVertices; }
};