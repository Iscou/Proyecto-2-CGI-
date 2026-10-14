#include "Mesh.h"
#include <vector>
#include <string>
#include "stb_image.h"
#include <iostream>

class SkyBox {
    private:
    // El Skybox como tal
    Mesh skybox; 

    // ID para la textura y el shader
    GLuint idText;
    GLuint idShaderP; 

    public:
    // Constructor 
    SkyBox(int idMesh, int idSubMesh, GLuint shaderID);

    // Loader para las caras del SkyBox
    bool cargarCubemap(std::vector<std::string> caras);

    // Draw del skybox
    void dibujar(glm::mat4 view, glm::mat4 projection);
};