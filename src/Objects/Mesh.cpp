#include "Mesh.h"

// Constructor 
Mesh::Mesh(int id, std::string nombre) : id(id), nombre(nombre) {}

// Matriz Global (T * R * S), Traslacion, rotacion y escalamiento
glm::mat4 Mesh::getMatrizGlobal() const {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, position);
    m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    m = glm::scale(m, scale);
    return m;
}

//  Normalizar el objeto al origen (0,0,0) en [-0.5, 0.5] y armar su Bounding Box
void Mesh::normalizarYCalcularBBox() {
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    // Buscar minimos y maximos en todas las submallas
    for (const SubMesh& sm : subMeshes) {
        for (const Triangle& t : sm.triangles) {
            glm::vec3 verts[3] = { t.v1, t.v2, t.v3 };
            for (int i = 0; i < 3; ++i) {
                minBounds = glm::min(minBounds, verts[i]);
                maxBounds = glm::max(maxBounds, verts[i]);
            }
        }
    }

    glm::vec3 centro = (minBounds + maxBounds) * 0.5f;
    glm::vec3 dims = maxBounds - minBounds;
    float maxDim = std::max(dims.x, std::max(dims.y, dims.z));
    if (maxDim < 1e-6f) maxDim = 1.0f;

    // Centrar y escalar cada vertice para normalizar el objeto
    for (SubMesh& sm : subMeshes) {
        for (Triangle& t : sm.triangles) {
            t.v1 = (t.v1 - centro) / maxDim;
            t.v2 = (t.v2 - centro) / maxDim;
            t.v3 = (t.v3 - centro) / maxDim;
        }
        sm.subirAGPU();
    }

    // Recalcular minBounds y maxBounds ya normalizados
    minBounds = (minBounds - centro) / maxDim;
    maxBounds = (maxBounds - centro) / maxDim;
    construirBBoxGPU();
}

// Construir los 8 vertices (v1..v8) y subir las 12 aristas a la GPU
void Mesh::construirBBoxGPU() {
    if (vaoBBox != 0) glDeleteVertexArrays(1, &vaoBBox);
    if (vboBBox != 0) glDeleteBuffers(1, &vboBBox);

    // las 8 esquinas de la caja
    v1 = glm::vec3(minBounds.x, minBounds.y, minBounds.z);
    v2 = glm::vec3(maxBounds.x, minBounds.y, minBounds.z);
    v3 = glm::vec3(maxBounds.x, maxBounds.y, minBounds.z);
    v4 = glm::vec3(minBounds.x, maxBounds.y, minBounds.z);
    v5 = glm::vec3(minBounds.x, minBounds.y, maxBounds.z);
    v6 = glm::vec3(maxBounds.x, minBounds.y, maxBounds.z);
    v7 = glm::vec3(maxBounds.x, maxBounds.y, maxBounds.z);
    v8 = glm::vec3(minBounds.x, maxBounds.y, maxBounds.z);

    // 12 aristas (24 puntos) conectando v1 ... v8
    glm::vec3 lineas[24] = {
        v1,v2, v2,v3, v3,v4, v4,v1, // Cara trasera
        v5,v6, v6,v7, v7,v8, v8,v5, // Cara delantera
        v1,v5, v2,v6, v3,v7, v4,v8  // Conexiones laterales
    };

    std::vector<float> datos;
    for (int i = 0; i < 24; ++i) {
        datos.push_back(lineas[i].x); datos.push_back(lineas[i].y); datos.push_back(lineas[i].z);
        datos.push_back(0.0f);        datos.push_back(1.0f);        datos.push_back(0.0f);
    }

    glGenVertexArrays(1, &vaoBBox);
    glGenBuffers(1, &vboBBox);
    glBindVertexArray(vaoBBox);
    glBindBuffer(GL_ARRAY_BUFFER, vboBBox);
    glBufferData(GL_ARRAY_BUFFER, datos.size() * sizeof(float), datos.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

// Dibujar todas las submallas y la Bounding Box
void Mesh::dibujar(GLuint shaderProgram) const {
    glm::mat4 matGlobal = getMatrizGlobal();

    for (const SubMesh& sm : subMeshes) {
        sm.dibujar(shaderProgram, matGlobal, wireframe, seeVertex, seeNormals);
    }

    if (seeBoudingBox && vaoBBox != 0) {
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(matGlobal));
        glm::vec4 colorCaja(0.0f, 1.0f, 0.0f, 1.0f); // Verde brillante
        glUniform4fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(colorCaja));
        glBindVertexArray(vaoBBox);
        glDrawArrays(GL_LINES, 0, 24);
        glBindVertexArray(0);
    }
}

void Mesh::limpiarGPU() {
    for (SubMesh& sm : subMeshes) sm.limpiarGPU();
    if (vaoBBox != 0) { glDeleteVertexArrays(1, &vaoBBox); vaoBBox = 0; }
    if (vboBBox != 0) { glDeleteBuffers(1, &vboBBox); vboBBox = 0; }
}

// Primitivas parametrizables
Mesh Mesh::crearCubo(int idObjeto, int idSubMesh, float lado) {
    Mesh m(idObjeto, "Cubo_" + std::to_string(idObjeto));
    float h = lado * 0.5f;
    glm::vec3 p[8] = {
        {-h,-h,-h}, {h,-h,-h}, {h,h,-h}, {-h,h,-h},
        {-h,-h, h}, {h,-h, h}, {h,h, h}, {-h,h, h}
    };
    std::vector<Triangle> tris = {
        Triangle(p[4], p[5], p[6]), Triangle(p[4], p[6], p[7]), // Frente
        Triangle(p[1], p[0], p[3]), Triangle(p[1], p[3], p[2]), // Atras
        Triangle(p[0], p[4], p[7]), Triangle(p[0], p[7], p[3]), // Izquierda
        Triangle(p[5], p[1], p[2]), Triangle(p[5], p[2], p[6]), // Derecha
        Triangle(p[7], p[6], p[2]), Triangle(p[7], p[2], p[3]), // Arriba
        Triangle(p[0], p[1], p[5]), Triangle(p[0], p[5], p[4])  // Abajo
    };
    SubMesh sm(idSubMesh, tris, glm::vec4(0.2f, 0.6f, 0.9f, 1.0f));
    sm.calcularNormalesPromediadas();
    sm.subirAGPU();
    m.subMeshes.push_back(sm);
    m.minBounds = glm::vec3(-h); m.maxBounds = glm::vec3(h);
    m.construirBBoxGPU();
    return m;
}

Mesh Mesh::crearPiramide(int idObjeto, int idSubMesh, float base, float altura) {
    Mesh m(idObjeto, "Piramide_" + std::to_string(idObjeto));
    float h = base * 0.5f;
    float yMin = -altura * 0.5f, yMax = altura * 0.5f;
    glm::vec3 punta(0.0f, yMax, 0.0f);
    glm::vec3 b0(-h, yMin,  h), b1(h, yMin,  h), b2(h, yMin, -h), b3(-h, yMin, -h);

    std::vector<Triangle> tris = {
        Triangle(b0, b1, punta), Triangle(b1, b2, punta),
        Triangle(b2, b3, punta), Triangle(b3, b0, punta),
        Triangle(b0, b3, b2),    Triangle(b0, b2, b1) // Base cuadrada
    };
    SubMesh sm(idSubMesh, tris, glm::vec4(0.9f, 0.7f, 0.2f, 1.0f));
    sm.calcularNormalesPromediadas();
    sm.subirAGPU();
    m.subMeshes.push_back(sm);
    m.minBounds = glm::vec3(-h, yMin, -h); m.maxBounds = glm::vec3(h, yMax, h);
    m.construirBBoxGPU();
    return m;
}

Mesh Mesh::crearEsfera(int idObjeto, int idSubMesh, float radio, int sectores, int stacks) {
    Mesh m(idObjeto, "Esfera_" + std::to_string(idObjeto));
    std::vector<Triangle> tris;
    const float PI = 3.14159265359f;

    auto getPunto = [&](int i, int j) {
        float u = (float)j / sectores * 2.0f * PI;
        float v = (float)i / stacks * PI - (PI * 0.5f);
        return glm::vec3(radio * cos(v) * cos(u), radio * sin(v), radio * cos(v) * sin(u));
    };

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectores; ++j) {
            glm::vec3 p1 = getPunto(i, j),     p2 = getPunto(i, j + 1);
            glm::vec3 p3 = getPunto(i + 1, j), p4 = getPunto(i + 1, j + 1);
            tris.push_back(Triangle(p1, p3, p2, glm::normalize(p1), glm::normalize(p3), glm::normalize(p2)));
            tris.push_back(Triangle(p2, p3, p4, glm::normalize(p2), glm::normalize(p3), glm::normalize(p4)));
        }
    }
    SubMesh sm(idSubMesh, tris, glm::vec4(0.9f, 0.3f, 0.3f, 1.0f));
    m.subMeshes.push_back(sm);
    m.minBounds = glm::vec3(-radio); m.maxBounds = glm::vec3(radio);
    m.construirBBoxGPU();
    return m;
}

// Primitiva del cilindro
Mesh Mesh::crearCilindro (int idObjeto, int idSubMesh, float radio, float altura, int sectores) {
    Mesh m(idObjeto, "Cilindro_" + std::to_string(idObjeto));

    std::vector<Triangle> tris;
    const float PI = 3.14159265359f;
    
    float yMin = -altura * 0.5f;
    float yMax = altura * 0.5f; 

    glm::vec3 centroArriba(0.0f, yMax, 0.0f);
    glm::vec3 centroAbajo(0.0f, yMin, 0.0f);

    for (int i = 0; i < sectores; ++i) {
        float u1 = (float)i / sectores * 2.0f * PI;
        float u2 = (float)(i + 1) / sectores * 2.0f * PI;

        glm::vec3 p1Arriba(radio * cos(u1), yMax, radio * sin(u1));
        glm::vec3 p2Arriba(radio * cos(u2), yMax, radio * sin(u2));
        glm::vec3 p1Abajo(radio * cos(u1), yMin, radio * sin(u1));
        glm::vec3 p2Abajo(radio * cos(u2), yMin, radio * sin(u2));

        // Tapa superior e inferior
        tris.push_back(Triangle(centroArriba, p1Arriba, p2Arriba, glm::vec3(0,1,0), glm::vec3(0,1,0), glm::vec3(0,1,0)));
        tris.push_back(Triangle(centroAbajo, p2Abajo, p1Abajo, glm::vec3(0,-1,0), glm::vec3(0,-1,0), glm::vec3(0,-1,0)));

        // Cuerpo lateral (Calculamos sus normales puramente en XZ)
        glm::vec3 n1 = glm::normalize(glm::vec3(p1Abajo.x, 0.0f, p1Abajo.z));
        glm::vec3 n2 = glm::normalize(glm::vec3(p2Abajo.x, 0.0f, p2Abajo.z));
        
        tris.push_back(Triangle(p1Abajo, p1Arriba, p2Arriba, n1, n1, n2));
        tris.push_back(Triangle(p1Abajo, p2Arriba, p2Abajo, n1, n2, n2));
    }

    SubMesh sm(idSubMesh, tris, glm::vec4(0.8f, 0.4f, 0.8f, 1.0f));
    sm.subirAGPU(); // Pasamos las normales listas
    m.subMeshes.push_back(sm);
    m.minBounds = glm::vec3(-radio, yMin, -radio); 
    m.maxBounds = glm::vec3(radio, yMax, radio);
    m.construirBBoxGPU();
    return m;
}