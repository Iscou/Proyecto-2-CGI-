#include "TexturesManager.h"
#include "stb_image.h"

GLuint cargarTextura(const std::string& ruta) {
    int width, height, nrChannels;

    if (ruta == "") return 0;

    std::string rutaCompleta = "assets/textures/" + ruta;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(rutaCompleta.c_str(), &width, &height, &nrChannels, 4);

    if (!image) {
        std::cout << "Error al cargar la textura con stbi_load. Motivo: " << stbi_failure_reason() << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    //glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Textura cargada exitosamente con stb_image!" << std::endl;

    return textureID;
}