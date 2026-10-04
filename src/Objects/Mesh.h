#pragma once
#include "SubMesh.h" 
#include <vector>
#include <string>
#include <limits>
#include <cmath>
#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Mesh {
private:
    // Buffers de OpenGL para dibujar las 12 lineas de la Bounding Box
    GLuint vaoBBox = 0;
    GLuint vboBBox = 0;

public:
    int id = 0;
    std::string nombre = "Objeto3D";
    std::string rutaArchivo = ""; 

    // Submallados de la figura
    std::vector<SubMesh> subMeshes;

    // Propiedades espaciales Globales
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 scale    = glm::vec3(1.0f, 1.0f, 1.0f);

    // Interruptores de visualizacion
    bool wireframe     = false;
    bool seeNormals    = false;
    bool seeVertex     = false;
    bool seeBoudingBox = false;

    // Esquinas del Bounding Box y los 8 vertices (v1..v8)
    glm::vec3 minBounds = glm::vec3(-0.5f);
    glm::vec3 maxBounds = glm::vec3(0.5f);
    glm::vec3 v1, v2, v3, v4, v5, v6, v7, v8;

    // Constructor
    Mesh(int id = 0, std::string nombre = "Objeto3D");

    // Metodos del Objeto Global
    glm::mat4 getMatrizGlobal() const;
    void normalizarYCalcularBBox();
    void construirBBoxGPU();
    void dibujar(GLuint shaderProgram) const;
    void limpiarGPU();

    // Primitivas parametrizables
    static Mesh crearCubo(int idObjeto, int idSubMesh, float lado = 1.0f, float alpha = 1.0f);
    static Mesh crearPiramide(int idObjeto, int idSubMesh, float base = 1.0f, float altura = 1.0f, float alpha = 1.0f);
    static Mesh crearEsfera(int idObjeto, int idSubMesh, float radio = 0.5f, int sectores = 20, int stacks = 20, float alpha = 1.0f);

    // Objetos TinyObjLoader
    static Mesh crearOBJ(int idObjeto, const std::string& rutaArchivo);
};