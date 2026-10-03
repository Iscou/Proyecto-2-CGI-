#define TINYOBJLOADER_IMPLEMENTATION
#include "ObjLoader.h"
#include "Mesh.h"
#include "tiny_obj_loader.h"
#include <iostream>
#include <limits>

std::vector<float> util::load_model_from_file(const char* filename, glm::mat4 preTransform) {

    std::vector<float> vertices;

    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, error;

    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &error, filename)) {
        std::cout << warn << error << std::endl;
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            glm::vec4 pos = {
                attributes.vertices[3 * index.vertex_index],
                attributes.vertices[3 * index.vertex_index + 1],
                attributes.vertices[3 * index.vertex_index + 2],
                1.0f
            };

            pos = preTransform * pos;

            glm::vec3 normal = {
                attributes.normals[3 * index.normal_index],
                attributes.normals[3 * index.normal_index + 1],
                attributes.normals[3 * index.normal_index + 2]
            };

            normal = glm::normalize(glm::mat3(preTransform) * normal);

            glm::vec2 textCoord = {
                attributes.texcoords[2 * index.texcoord_index],
                attributes.texcoords[2 * index.texcoord_index + 1]
            };

            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            vertices.push_back(textCoord.x);
            vertices.push_back(textCoord.y);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
        }
    }

    return vertices;
}

Mesh ObjLoader::cargarOBJ(int idObjeto, const std::string& rutaArchivo) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Extraer el directorio base del archivo para buscar el .mtl
    std::string baseDir = "";
    size_t lastSlash = rutaArchivo.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        baseDir = rutaArchivo.substr(0, lastSlash + 1);
    }

    // Intentar cargar el archivo .obj
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, rutaArchivo.c_str(), baseDir.c_str());

    if (!warn.empty()) std::cout << "OBJ Loader Warning: " << warn << std::endl;
    if (!err.empty())  std::cerr << "OBJ Loader Error: " << err << std::endl;
    if (!ret) return Mesh(idObjeto, "Error_Carga");

    Mesh m(idObjeto, "OBJ_" + std::to_string(idObjeto));

    // Inicializar Bounding Box extrema
    m.minBounds = glm::vec3(std::numeric_limits<float>::max());
    m.maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    int subMeshIdCounter = 0;

    // Recorrer cada grupo/submalla (shape) en el archivo .obj
    for (const auto& shape : shapes) {
        std::vector<Triangle> tris;
        size_t index_offset = 0;

        // Color por defecto si la sub-malla no tiene material asignado o el .mtl no existe
        glm::vec4 colorSubMesh(0.7f, 0.7f, 0.7f, 1.0f);

        // Obtener el Kd del material asignado a la primera cara del shape
        if (!shape.mesh.material_ids.empty()) {
            int matId = shape.mesh.material_ids[0]; // ID del material para este grupo
            if (matId >= 0 && matId < static_cast<int>(materials.size())) {
                const auto& mat = materials[matId];

                // mat.diffuse es un float[3] con componentes (r, g, b)
                colorSubMesh = glm::vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f);
            }
        }
    

        // Recorrer las caras del shape
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            size_t fv = shape.mesh.num_face_vertices[f];

            // Asumimos triangulación (fv == 3)
            if (fv == 3) {
                glm::vec3 pos[3];
                glm::vec3 norm[3];

                for (size_t v = 0; v < 3; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                    // Extraer posición
                    pos[v] = glm::vec3(
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]
                    );

                    // Actualizar Bounding Box global del mesh
                    m.minBounds = glm::min(m.minBounds, pos[v]);
                    m.maxBounds = glm::max(m.maxBounds, pos[v]);

                    // Extraer Normal (si existe en el .obj)
                    if (idx.normal_index >= 0) {
                        norm[v] = glm::vec3(
                            attrib.normals[3 * idx.normal_index + 0],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]
                        );
                    }
                    else {
                        norm[v] = glm::vec3(0.0f, 1.0f, 0.0f); // Normal por defecto
                    }
                }

                // Crear el triángulo
                tris.push_back(Triangle(pos[0], pos[1], pos[2], norm[0], norm[1], norm[2]));
            }

            index_offset += fv;
        }

        // Crear la SubMesh correspondiente y agregarla al Mesh
        if (!tris.empty()) {
            SubMesh sm(subMeshIdCounter++, tris, colorSubMesh);
            m.subMeshes.push_back(sm);
        }
    }

    // Generar Bounding Box en GPU
    m.construirBBoxGPU();
    return m;
}