#include "TexturesManager.h"
#include "stb_image.h"

GLuint cargarTextura(const std::string& ruta) {
    int width, height, nrChannels;

    std::string rutaCompleta = "assets/textures/" + ruta;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(rutaCompleta.c_str(), &width, &height, &nrChannels, 0);

    if (!image) {
        std::cout << "Error al cargar la textura con stbi_load. Motivo: " << stbi_failure_reason() << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configuración de parámetros de la textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Enviar a OpenGL (maneja RGB o RGBA automáticamente según el canal)
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Liberar la memoria de la imagen y desvincular
    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Textura cargada exitosamente con stb_image!" << std::endl;

    return textureID;
}