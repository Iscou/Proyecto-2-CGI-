#include "Skybox.h"

// Constructor
SkyBox::SkyBox(int idMesh, int idSubMesh, GLuint shaderID) {
    this->skybox = Mesh::crearCubo(idMesh, idSubMesh);
    this->idShaderP = shaderID;
}

// Loader de las caras

bool SkyBox:: cargarCubemap(std::vector<std::string> caras) {
    // Bandera para error al cargar
    bool error = false;

    // Reservamos la memoria para las texturas
    glGenTextures(1, &idText);
    // Le indicamos a opengl que vamos a trabajar con el texturizado de un cubo
    glBindTexture(GL_TEXTURE_CUBE_MAP, idText);
    
    // Propiedades de la imagen
    int ancho, alto, canales;

    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < caras.size(); i++) {

        // Cargamos la imagen a RAM
        unsigned char* data = stbi_load(caras[i].c_str(), &ancho, &alto, &canales, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, ancho, alto, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data); 
        } else {
            std::cout << "CRÍTICO: " << stbi_failure_reason() << " -> " << caras[i] << std::endl;
            stbi_image_free(data);
            error = true;
            break;
        }   

    }

    if (error) {
        glDeleteTextures(1, &idText); // Liberamos la VRAM reservada para la textura
        return false;
    } else {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        return true;
    }
    
}

void SkyBox::dibujar(glm::mat4 view, glm::mat4 projection) {
    glDepthFunc(GL_LEQUAL);

    // Verificamos si el Culling esta encendido actualmente
    GLboolean cullingActivo;
    glGetBooleanv(GL_CULL_FACE, &cullingActivo);
    
    // Lo apagamos a la fuerza para poder ver el cielo por dentro
    glDisable(GL_CULL_FACE); 

    glUseProgram(idShaderP);
    glm::mat4 viewSinTraslacion = glm::mat4(glm::mat3(view));

    unsigned int viewLoc = glGetUniformLocation(idShaderP, "view");
    unsigned int projLoc = glGetUniformLocation(idShaderP, "projection");
    
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewSinTraslacion));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindTexture(GL_TEXTURE_CUBE_MAP, idText);
    this->skybox.dibujar(idShaderP);

    // Restauramos el estado exacto en el que estaba antes
    if (cullingActivo) glEnable(GL_CULL_FACE);
    
    glDepthFunc(GL_LESS);
}