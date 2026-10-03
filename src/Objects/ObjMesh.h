#pragma once
#include <vector>
#include <string>
#include <limits>
#include <cmath>
#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include "ObjLoader.h"

struct MeshCreateInfo {
    const char* filename;
    glm::mat4 preTransform;
};

class ObjMesh {
public:
    unsigned int VBO, VAO, vertexCount;

    ObjMesh(MeshCreateInfo* createInfo);
    ~ObjMesh();
};