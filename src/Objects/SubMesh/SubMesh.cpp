#include "SubMesh.h"
#include <cmath>

SubMesh::SubMesh(int id, std::string nombre) 
    : id(id), nombre(nombre) {}

SubMesh::SubMesh(int id, const std::vector<Triangle>& tris, glm::vec4 colorDifuso)
    : id(id), nombre("SubMesh_" + std::to_string(id)), triangles(tris), color(colorDifuso) {
    subirAGPU();
}

void SubMesh::calcularNormalesPromediadas() {
    // Calcular la normal plana de cada triangulo con Producto Cruz
    std::vector<glm::vec3> normalesCara(triangles.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < triangles.size(); ++i) {
        glm::vec3 arista1 = triangles[i].v2 - triangles[i].v1;
        glm::vec3 arista2 = triangles[i].v3 - triangles[i].v1;
        glm::vec3 n = glm::cross(arista1, arista2);
        if (glm::length(n) > 1e-6f) {
            normalesCara[i] = glm::normalize(n);
        }
    }

    // Promedia las normales de los triangulos que comparten la misma posicion de vertice
    const float EPSILON = 1e-4f;
    for (size_t i = 0; i < triangles.size(); ++i) {
        glm::vec3 acumV1(0.0f), acumV2(0.0f), acumV3(0.0f);

        for (size_t j = 0; j < triangles.size(); ++j) {
            // Si el vertice i comparte posicion con algun vertice del triangulo j, sumamos su normal
            if (glm::length(triangles[i].v1 - triangles[j].v1) < EPSILON ||
                glm::length(triangles[i].v1 - triangles[j].v2) < EPSILON ||
                glm::length(triangles[i].v1 - triangles[j].v3) < EPSILON) {
                acumV1 += normalesCara[j];
            }
            if (glm::length(triangles[i].v2 - triangles[j].v1) < EPSILON ||
                glm::length(triangles[i].v2 - triangles[j].v2) < EPSILON ||
                glm::length(triangles[i].v2 - triangles[j].v3) < EPSILON) {
                acumV2 += normalesCara[j];
            }
            if (glm::length(triangles[i].v3 - triangles[j].v1) < EPSILON ||
                glm::length(triangles[i].v3 - triangles[j].v2) < EPSILON ||
                glm::length(triangles[i].v3 - triangles[j].v3) < EPSILON) {
                acumV3 += normalesCara[j];
            }
        }

        // Normaliza el promedio obtenido para cada vertice
        triangles[i].vn1 = (glm::length(acumV1) > 1e-6f) ? glm::normalize(acumV1) : normalesCara[i];
        triangles[i].vn2 = (glm::length(acumV2) > 1e-6f) ? glm::normalize(acumV2) : normalesCara[i];
        triangles[i].vn3 = (glm::length(acumV3) > 1e-6f) ? glm::normalize(acumV3) : normalesCara[i];
    }
}

void SubMesh::subirAGPU() {
    limpiarGPU(); // Por si se vuelve a llamar al actualizar normales

    std::vector<float> datosTriangulos;
    std::vector<float> datosLineasNormales;
    const float LONGITUD_NORMAL = 0.12f; // Largo visual de la linea de normal

    for (const Triangle& t : triangles) {
        // Asegurar que las normales esten normalizadas al cargar
        glm::vec3 n1 = (glm::length(t.vn1) > 1e-6f) ? glm::normalize(t.vn1) : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 n2 = (glm::length(t.vn2) > 1e-6f) ? glm::normalize(t.vn2) : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 n3 = (glm::length(t.vn3) > 1e-6f) ? glm::normalize(t.vn3) : glm::vec3(0.0f, 1.0f, 0.0f);

        // Empaqueta los 3 vertices del triangulo (Posicion + Normal) 
        glm::vec3 verts[3] = { t.v1, t.v2, t.v3 };
        glm::vec3 norms[3] = { n1, n2, n3 };

        for (int i = 0; i < 3; ++i) {
            datosTriangulos.push_back(verts[i].x);
            datosTriangulos.push_back(verts[i].y);
            datosTriangulos.push_back(verts[i].z);
            datosTriangulos.push_back(norms[i].x);
            datosTriangulos.push_back(norms[i].y);
            datosTriangulos.push_back(norms[i].z);

            // Empaqueta el segmento de linea para visualizar la normal (v -> v + n*L)
            glm::vec3 punta = verts[i] + norms[i] * LONGITUD_NORMAL;
            // Punto inicial de la linea
            datosLineasNormales.push_back(verts[i].x);
            datosLineasNormales.push_back(verts[i].y);
            datosLineasNormales.push_back(verts[i].z);
            datosLineasNormales.push_back(norms[i].x);
            datosLineasNormales.push_back(norms[i].y);
            datosLineasNormales.push_back(norms[i].z);
            // Punto final de la linea
            datosLineasNormales.push_back(punta.x);
            datosLineasNormales.push_back(punta.y);
            datosLineasNormales.push_back(punta.z);
            datosLineasNormales.push_back(norms[i].x);
            datosLineasNormales.push_back(norms[i].y);
            datosLineasNormales.push_back(norms[i].z);
        }
    }

    totalVertices = static_cast<int>(triangles.size() * 3);
    totalVerticesNormales = totalVertices * 2;

    // Sube VAO y VBO de los Triangulos
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, datosTriangulos.size() * sizeof(float), datosTriangulos.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Sube VAO y VBO de las Lineas de Normales
    glGenVertexArrays(1, &vaoNormales);
    glGenBuffers(1, &vboNormales);
    glBindVertexArray(vaoNormales);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormales);
    glBufferData(GL_ARRAY_BUFFER, datosLineasNormales.size() * sizeof(float), datosLineasNormales.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

glm::mat4 SubMesh::getMatrizLocal() const {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, posicion);
    m = glm::rotate(m, glm::radians(rotacion.x), glm::vec3(1.0f, 0.0f, 0.0f));
    m = glm::rotate(m, glm::radians(rotacion.y), glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::rotate(m, glm::radians(rotacion.z), glm::vec3(0.0f, 0.0f, 1.0f));
    m = glm::scale(m, escala);
    return m;
}

void SubMesh::dibujar(GLuint shaderProgram, const glm::mat4& matrizPadre, 
                      bool verWireframe, bool verVertices, bool verNormales) const {
    if (vao == 0 || totalVertices == 0) return;

    // Evaluar si la submalla posee transparencia
    bool transparente = (color.a < 1.0f);

    if (transparente) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // Evita que la malla transparente bloquee lo que está detrás
    }

    // La transformacion del Objeto Padre afecta a esta SubMalla: M_final = M_padre * M_local
    glm::mat4 modelFinal = matrizPadre * getMatrizLocal();
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelFinal));
    glUniform4fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(color));

    // Dibuja la malla principal
    if (verWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, totalVertices);

    // Visualiza los Vertices si esta activo
    if (verVertices) {
        glPointSize(6.0f);
        glm::vec4 colorPuntos(1.0f, 1.0f, 0.0f, 1.0f); // Amarillo para resaltar
        glUniform4fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(colorPuntos));
        glDrawArrays(GL_POINTS, 0, totalVertices);
    }

    // Visualiza las Normales si esta activo
    if (verNormales && vaoNormales != 0) {
        glm::vec4 colorNormales(0.0f, 1.0f, 1.0f, 1.0f); // Cyan para resaltar
        glUniform4fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(colorNormales));
        glBindVertexArray(vaoNormales);
        glDrawArrays(GL_LINES, 0, totalVerticesNormales);
    }

    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (transparente) {
        glDepthMask(GL_TRUE); // Reactivar la escritura normal en el Z-Buffer
        glDisable(GL_BLEND);  // Desactivar blending para las mallas posteriores que sean opacas
    }

}

void SubMesh::limpiarGPU() {
    if (vao != 0) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo != 0) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (vaoNormales != 0) { glDeleteVertexArrays(1, &vaoNormales); vaoNormales = 0; }
    if (vboNormales != 0) { glDeleteBuffers(1, &vboNormales); vboNormales = 0; }
}