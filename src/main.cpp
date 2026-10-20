#include "Engine/engine3D.h"
#include "Camera/camera.h"
#include "Objects/Mesh.h"
#include "Particles/ParticleSystem.h"
#include "Textures/TexturesManager.h"
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Skybox/Skybox.h"
#include "AABB.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

// Shaders
const char* vertexShaderSrc = R"(
#version 330 core

// Atributos de entrada (El Vertex de 14 floats)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// Variables de salida hacia el Fragment Shader
out vec3 FragPos;
out vec2 TexCoords;
out mat3 TBN; 

// Uniformes 
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Calculamos la posición del vértice en el mundo
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Pasamos las coordenadas UV sin alterar
    TexCoords = aTexCoords;

    // Calculamos la Matriz Normal para rotar los vectores correctamente 
    // (Se usa la inversa transpuesta para evitar que el escalado deforme las normales)
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    
    // Rotamos la Tangente, Bitangente y Normal al espacio del mundo real
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    // Re-ortogonalizamos (Proceso de Gram-Schmidt)
    // Asegura que T siga siendo perfectamente perpendicular a N
    T = normalize(T - dot(T, N) * N);
    // Y recalculamos B con el producto cruz para garantizar un sistema de coordenadas perfecto
    B = cross(N, T);

    // Armamos la Matriz TBN (De espacio tangente a espacio del mundo)
    TBN = mat3(T, B, N);

    // Calculamos la posición final del vértice en la pantalla
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core

// Variables de entrada desde el Vertex Shader
in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN; 

out vec4 FragColor;

// Modelos de iluminación 
struct DirLight { vec3 direction; vec3 ambient; vec3 diffuse; vec3 specular; };
struct PointLight { vec3 position; float constant; float linear; float quadratic; vec3 ambient; vec3 diffuse; vec3 specular; };
struct SpotLight { vec3 position; vec3 direction; float cutOff; float outerCutOff; float constant; float linear; float quadratic; vec3 ambient; vec3 diffuse; vec3 specular; };

// Texturas y Materiales 
// Textura Difusa 
uniform bool hasTexture;
uniform sampler2D ourTexture;

// Normal Map 
uniform bool usarNormalMap;      
uniform sampler2D texturaNormal; 

// Color base (si no hay textura difusa)
uniform vec4 objectColor;

// Iluminación
uniform int modoIluminacion; // 0 es Flat, 1 Lambert y 2 Blinn-Phong
uniform vec3 viewPos;        // Posicion de la camara (necesario para Blinn Phong)

// Luces en la escena
uniform DirLight dirLight;
#define NR_POINT_LIGHTS 2
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

// Prototipos de funciones para las luces
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);

void main() {
    //  Obtener el color base del objeto
    vec4 baseColorAlpha = objectColor;
    if (hasTexture) {
        baseColorAlpha *= texture(ourTexture, TexCoords); 
    }
    
    // Descartamos pixeles invisibles (mallas con transparencia total)
    if(baseColorAlpha.a < 0.1) discard; 

    // Preparar la Normal
    vec3 norm;
    if (modoIluminacion == 0) {
        // FLAT SHADING: Ignora todo y calcula una normal plana matematica
        vec3 fdx = dFdx(FragPos);
        vec3 fdy = dFdy(FragPos);
        norm = normalize(cross(fdx, fdy));
    } else {
        // LAMBERT Y BLINN-PHONG
        if (usarNormalMap) {
            // Leer el color violeta del mapa
            norm = texture(texturaNormal, TexCoords).rgb;
            // Transformar ese color al rango de vectores [-1.0 a 1.0]
            norm = normalize(norm * 2.0 - 1.0);
            // Multiplicar por TBN para rotar la normal falsa al mundo real
            norm = normalize(TBN * norm);
        } else {
            // Si el objeto no tiene Normal Map, usamos la normal real (La columna Z de TBN)
            norm = normalize(TBN[2]);
        }
    }

    // Calcular la iluminacion
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);
    vec3 baseColorRgb = baseColorAlpha.rgb;

    // Sumar luces enviando la normal y el color base
    result += CalcDirLight(dirLight, norm, viewDir, baseColorRgb);
    
    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, baseColorRgb);    
    }
    
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir, baseColorRgb);    
    
    // Instanciamos el color final obtenido
    FragColor = vec4(result, baseColorAlpha.a);
}

// Definición de las funciones de luz 

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(-light.direction);
    
    // Ambiental
    vec3 ambient = light.ambient * baseColor;
    
    // Difusa
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * baseColor;
    
    // Especular en blinn-phong
    vec3 specular = vec3(0.0);
    if (modoIluminacion == 2) {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0); // 32.0 es el nivel de pulido
        specular = light.specular * spec * baseColor;
    }
    
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    vec3 ambient = light.ambient * baseColor;
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * baseColor;
    
    vec3 specular = vec3(0.0);
    if (modoIluminacion == 2) {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        specular = light.specular * spec * baseColor;
    }
    
    // Atenuación por distancia
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    return (ambient + diffuse + specular) * attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * baseColor;
    
    vec3 specular = vec3(0.0);
    if (modoIluminacion == 2) {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        specular = light.specular * spec * baseColor;
    }
    
    // Suavizado de los bordes del cono de luz
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Atenuación por distancia
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    vec3 ambient = light.ambient * baseColor;
    return ambient * attenuation + (diffuse + specular) * attenuation * intensity;
}
)";

const char* pickingVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* pickingFragmentShaderSrc = R"(
#version 330 core
uniform uint meshID;
uniform uint subMeshID;
out uvec3 FragColor; 

void main() {
    FragColor = uvec3(meshID, subMeshID, uint(gl_PrimitiveID));
}
)";

const char* skyboxVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 TexCoords;
uniform mat4 projection;
uniform mat4 view;
void main() {
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";

const char* skyboxFragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 TexCoords;
uniform samplerCube skybox;
void main() {    
    FragColor = texture(skybox, TexCoords);
}
)";

class Proyecto2 : public Engine3D {
private:
    Camera camera;
    GLuint shaderProgram;
    GLuint pickingShaderProgram;
    GLuint pickingFBO;
    GLuint pickingTexture;
    GLuint pickingDepth;

    // Skybox
    GLuint skyboxShaderProgram;
    SkyBox* miCielo = nullptr;

    ParticleSystem* particleSystem = new ParticleSystem(1000); // Sistema de partículas
    glm::vec3* emisorPos = new glm::vec3(0.0f, 0.0f, 0.0f);
    bool emitirParticulas = false;
    int particlesPerFrame = 5;

    // Texturas
    TexturaInfo textActual;
    bool tieneTextura = false;

    // Rutas del skybox 
    std::vector<std::string> carasCielo = {
        "assets/skybox/right.jpg",
        "assets/skybox/left.jpg",
        "assets/skybox/top.jpg",
        "assets/skybox/bottom.jpg",
        "assets/skybox/front.jpg",
        "assets/skybox/back.jpg"
    };

    // Camara 
    bool modoLibre = false;
    bool tabPresionado = false;
    int triSeleccionadoGlobal = -1;
    int subMeshSeleccionadoId = -1;

    // Iluminacion 
    int modIluminacion = 1; // 0 para Flat, 1 para Lambert y 2 para Blinn-Phong

    // Lista de objetos 3D en la escena y control de seleccion
    std::vector<Mesh> escena;
    int objSeleccionado = 0;
    int contadorID = 1;

    // Control del raton para la camara
    bool rotandoCamara = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;

    // Estados de OpenGL exigidos por el PDF
    bool depthTestActivo = true;
    bool cullFaceActivo = false;

    int modoSeleccion = 0; // Mod select. 0: Global, 1: Local, 2: Triangulo

    // Colisiones 

    bool comprobarColision(AABB a, AABB b) {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
            (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
            (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }

    void guardarEscena(const std::string& nombreArchivo) {
        if (!fs::exists("Saves")) fs::create_directory("Saves"); // Crea la carpeta default

        json jEscena = json::array();
        for (const Mesh& m : escena) {
            json jMesh;
            jMesh["nombre"] = m.nombre;
            jMesh["pos"] = { m.position.x, m.position.y, m.position.z };
            jMesh["rot"] = { m.rotation.x, m.rotation.y, m.rotation.z };
            jMesh["scl"] = { m.scale.x, m.scale.y, m.scale.z };

            // Guardamos el color de la primera submalla 
            if (!m.subMeshes.empty()) {
                jMesh["color"] = { m.subMeshes[0].color.r, m.subMeshes[0].color.g, m.subMeshes[0].color.b, m.subMeshes[0].color.a };
            }
            jEscena.push_back(jMesh);
        }

        std::ofstream archivo("Saves/" + nombreArchivo + ".json");
        archivo << jEscena.dump(4);
        std::cout << "Escena guardada en Saves/" << nombreArchivo << ".json\n";
    }

    void cargarEscena(const std::string& rutaCompleta) {
        std::ifstream archivo(rutaCompleta);
        if (!archivo.is_open()) return;
        json jEscena;
        archivo >> jEscena;

        // Borrar escena actual y limpiar GPU
        for (Mesh& m : escena) m.limpiarGPU();
        escena.clear();
        objSeleccionado = -1;
        contadorID = 1;

        for (const auto& jMesh : jEscena) {
            std::string nombreGuardado = jMesh["nombre"];
            Mesh m;

            // Reconstruir la figura basandose en el sufijo
            if (nombreGuardado.find("Cubo") != std::string::npos) m = Mesh::crearCubo(contadorID, contadorID);
            else if (nombreGuardado.find("Piramide") != std::string::npos) m = Mesh::crearPiramide(contadorID, contadorID);
            else if (nombreGuardado.find("Esfera") != std::string::npos) m = Mesh::crearEsfera(contadorID, contadorID);
            else if (nombreGuardado.find("Cilindro") != std::string::npos) m = Mesh::crearCilindro(contadorID, contadorID);
            else {
                size_t pos = nombreGuardado.find('.');
                std::string original = std::string(nombreGuardado.substr(0, pos));
                m = Mesh::crearOBJ(contadorID, "assets/models/" + original + ".obj");
            }


            m.position = glm::vec3(jMesh["pos"][0], jMesh["pos"][1], jMesh["pos"][2]);
            m.rotation = glm::vec3(jMesh["rot"][0], jMesh["rot"][1], jMesh["rot"][2]);
            m.scale = glm::vec3(jMesh["scl"][0], jMesh["scl"][1], jMesh["scl"][2]);

            if (!m.subMeshes.empty() && jMesh.contains("color")) {
                m.subMeshes[0].color = glm::vec4(jMesh["color"][0], jMesh["color"][1], jMesh["color"][2], jMesh["color"][3]);
            }

            escena.push_back(m);
            contadorID++;
        }
        std::cout << "Escena cargada desde " << rutaCompleta << "\n";
    }

    void compilarShaders() {
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertexShaderSrc, nullptr);
        glCompileShader(vs);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragmentShaderSrc, nullptr);
        glCompileShader(fs);

        GLuint pv = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(pv, 1, &pickingVertexShaderSrc, nullptr);
        glCompileShader(pv);

        GLuint pf = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(pf, 1, &pickingFragmentShaderSrc, nullptr);
        glCompileShader(pf);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glLinkProgram(shaderProgram);

        pickingShaderProgram = glCreateProgram();
        glAttachShader(pickingShaderProgram, pv);
        glAttachShader(pickingShaderProgram, pf);
        glLinkProgram(pickingShaderProgram);

        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteShader(pv);
        glDeleteShader(pf);

        // Compilado para skybox 
        GLuint sv = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(sv, 1, &skyboxVertexShaderSrc, nullptr);
        glCompileShader(sv);

        GLuint sf = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(sf, 1, &skyboxFragmentShaderSrc, nullptr);
        glCompileShader(sf);

        skyboxShaderProgram = glCreateProgram();
        glAttachShader(skyboxShaderProgram, sv);
        glAttachShader(skyboxShaderProgram, sf);
        glLinkProgram(skyboxShaderProgram);

        glDeleteShader(sv);
        glDeleteShader(sf);
    }


public:
    Proyecto2() : Engine3D(1280, 720, "Proyecto #2 - Renderizado y Manipulacion 3D") {}
    ~Proyecto2() {
        delete miCielo;
    }

    void setupFramebuffer() {
        glGenFramebuffers(1, &pickingFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

        // Textura de Enteros sin sing para guardar IDs i
        glGenTextures(1, &pickingTexture);
        glBindTexture(GL_TEXTURE_2D, pickingTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTexture, 0);

        // Renderbuffer para la profundidad (para que los objetos de enfrente tapen a los de atras)
        glGenRenderbuffers(1, &pickingDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, pickingDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pickingDepth);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void setup() override {
        compilarShaders();
        setupFramebuffer();

        // Instanciar el Skybox y mandarle las texturas a la VRAM
        miCielo = new SkyBox(999, 999, skyboxShaderProgram);
        miCielo->cargarCubemap(carasCielo);

        escena.push_back(Mesh::crearCubo(contadorID, contadorID));
        contadorID++;
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }

    void onMouseButtonDown(int button, double x, double y) override {
        if (ImGui::GetIO().WantCaptureMouse || modoLibre) return;

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
            GLuint pixelIDs[3];
            glReadPixels(x, height - y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, pixelIDs);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            if (pixelIDs[0] > 0) {
                for (int i = 0; i < escena.size(); ++i) {
                    if (escena[i].id == pixelIDs[0]) {
                        objSeleccionado = i;
                        break;
                    }
                }

                if (modoSeleccion == 1) {
                    subMeshSeleccionadoId = pixelIDs[1];
                    triSeleccionadoGlobal = -1;
                }
                else if (modoSeleccion == 2) {
                    subMeshSeleccionadoId = pixelIDs[1];
                    triSeleccionadoGlobal = pixelIDs[2];
                }
                else {
                    subMeshSeleccionadoId = -1;
                    triSeleccionadoGlobal = -1;
                }
            }
            else {
                subMeshSeleccionadoId = -1;
                triSeleccionadoGlobal = -1;
            }
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            rotandoCamara = true;
            lastMouseX = x;
            lastMouseY = y;
        }
    }

    void onMouseMove(double x, double y) override {
        // Se mueve libremente con el cursor oculto o manteniendo presionado clic derecho
        if (modoLibre || rotandoCamara) {
            float xoffset = static_cast<float>(x - lastMouseX);
            float yoffset = static_cast<float>(lastMouseY - y);
            lastMouseX = x;
            lastMouseY = y;
            camera.processMouseMovement(xoffset * 0.3f, yoffset * 0.3f);
        }
    }

    void onMouseButtonUp(int button, double x, double y) override {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            rotandoCamara = false;
        }
    }


    void update(float deltaTime) override {
        // Alternar modo libre
        if (isKeyPressed(GLFW_KEY_TAB)) {
            if (!tabPresionado) {
                modoLibre = !modoLibre;
                glfwSetInputMode(getWindow(), GLFW_CURSOR, modoLibre ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

                if (modoLibre) {
                    double cx, cy;
                    glfwGetCursorPos(getWindow(), &cx, &cy);
                    lastMouseX = cx;
                    lastMouseY = cy;
                }
                tabPresionado = true;
            }
        }
        else {
            tabPresionado = false;
        }

        // Guardamos el movimiento previo de la camara
        glm::vec3 posicionAnterior = camera.position;
        // Movimiento WASD
        if (modoLibre && !ImGui::GetIO().WantCaptureKeyboard) {
            camera.processKeyboard(
                isKeyPressed(GLFW_KEY_W), isKeyPressed(GLFW_KEY_S),
                isKeyPressed(GLFW_KEY_A), isKeyPressed(GLFW_KEY_D),
                isKeyPressed(GLFW_KEY_SPACE), isKeyPressed(GLFW_KEY_LEFT_SHIFT),
                deltaTime
            );
        }

        // Si la camara se desplazo, entonces
        if (camera.position != posicionAnterior) {
            glm::vec3 posFutura = camera.position;
            glm::vec3 radioJugador(0.2f, 0.8f, 0.2f);

            // validamos el eje x
            camera.position = glm::vec3(posFutura.x, posicionAnterior.y, posicionAnterior.z);
            AABB cajaX = { camera.position - radioJugador, camera.position + radioJugador };
            for (const Mesh& m : escena) {
                if (!m.esDinamico && comprobarColision(cajaX, m.obtenerAABBGlobal())) {
                    camera.position.x = posicionAnterior.x; // Chocamos en X, cancelamos solo X
                    break;
                }
            }
            posFutura.x = camera.position.x; // Guardamos el X validado

            // validamos el eje y
            camera.position = glm::vec3(posFutura.x, posFutura.y, posicionAnterior.z);
            AABB cajaY = { camera.position - radioJugador, camera.position + radioJugador };
            for (const Mesh& m : escena) {
                if (!m.esDinamico && comprobarColision(cajaY, m.obtenerAABBGlobal())) {
                    camera.position.y = posicionAnterior.y; // Chocamos en Y, cancelamos solo Y
                    break;
                }
            }
            posFutura.y = camera.position.y;

            // validamos el eje z
            camera.position = glm::vec3(posFutura.x, posFutura.y, posFutura.z);
            AABB cajaZ = { camera.position - radioJugador, camera.position + radioJugador };
            for (const Mesh& m : escena) {
                if (!m.esDinamico && comprobarColision(cajaZ, m.obtenerAABBGlobal())) {
                    camera.position.z = posicionAnterior.z; // Chocamos en Z, cancelamos solo Z
                    break;
                }
            }
        }

        // Detector de colisiones entre los Meshes
        for (size_t i = 0; i < escena.size(); i++) {
            // Solo procesamos objetos dinamico y velocidad > 0
            if (escena[i].esDinamico && glm::length(escena[i].velocidad) > 0.0f) {

                glm::vec3 posAnterior = escena[i].position;
                escena[i].position += escena[i].velocidad * deltaTime; // Despplazamos por adelantado el objeto

                AABB cajaDinamica = escena[i].obtenerAABBGlobal();
                bool choco = false;

                // Comprobar contra los demas objetos 
                for (size_t j = 0; j < escena.size(); j++) {
                    if (i == j) continue; // Sin contarlo a el mismo

                    AABB cajaOtro = escena[j].obtenerAABBGlobal();

                    if (comprobarColision(cajaDinamica, cajaOtro)) {
                        choco = true;
                        // Lo devolvemos a su posicion original
                        escena[i].position = posAnterior;
                        escena[i].velocidad = glm::vec3(0.0f);

                        // Si chocan podrian ser elimminados 
                        // if (escena[j].esDinamico) {
                        //     escena[i].borrame = true;
                        //     escena[j].borrame = true;
                        // }
                        break;
                    }
                }
            }
        }

        // Elimina objetos de la escena marcados para ello
        for (auto it = escena.begin(); it != escena.end(); ) {
            if (it->borrame) {
                it->limpiarGPU();
                it = escena.erase(it);
                objSeleccionado = -1; // Liberamos el apuntador a objetos por si acaso
            }
            else {
                ++it;
            }
        }

        // Estados de OpenGL compartidos
        if (depthTestActivo) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);

        if (cullFaceActivo) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }
        else {
            glDisable(GL_CULL_FACE);
        }

        // Calculo de matrices comunes
        float aspect = (height > 0) ? (static_cast<float>(width) / height) : 1.0f;
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(aspect);

        // Dibujar monitor oculto
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

        // Limpiamos el buffer de enteros usando [0,0,0] 
        GLuint clearColorZeros[4] = { 0, 0, 0, 0 };
        glClearBufferuiv(GL_COLOR, 0, clearColorZeros);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(pickingShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(pickingShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(pickingShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

        // Dibujamos solo geometria y enviamos los IDs puros
        for (const Mesh& m : escena) {
            glUniform1ui(glGetUniformLocation(pickingShaderProgram, "meshID"), static_cast<GLuint>(m.id));
            glm::mat4 matGlobal = m.getMatrizGlobal();

            for (const SubMesh& sm : m.subMeshes) {
                glUniform1ui(glGetUniformLocation(pickingShaderProgram, "subMeshID"), static_cast<GLuint>(sm.id));

                // Calculamos la matriz final 
                glm::mat4 modelFinal = matGlobal * sm.getMatrizLocal();
                glUniformMatrix4fv(glGetUniformLocation(pickingShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelFinal));

                // Dibujado simple (sin wireframe, sin puntos, sin normales)
                glBindVertexArray(sm.getVAO());
                glDrawArrays(GL_TRIANGLES, 0, sm.getTotalVertices());
                glBindVertexArray(0);
            }
        }

        // Dibujar en pantalla
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Actualizar el sistema de partículas
        if (emitirParticulas) {
            particleSystem->Update(deltaTime, *emisorPos, particlesPerFrame);
        }

        // Limpieza clasica de color visible
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Data g lobal
        glUniform1i(glGetUniformLocation(shaderProgram, "modoIluminacion"), modIluminacion);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera.position));

        // Luz direciconal
        glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.ambient"), 0.1f, 0.1f, 0.1f); // Luz base muy bajita
        glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.diffuse"), 0.4f, 0.4f, 0.4f); // Gris medio
        glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.specular"), 0.5f, 0.5f, 0.5f); // Brillo del sol

        // Point Light #1
        // Posicion inicial ajustable
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].position"), 2.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].ambient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].diffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].specular"), 0.2f, 0.2f, 0.2f); // Brillo suave
        // Constantes fisicas para que la luz viaje unos 50 metros antes de apagarse
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].quadratic"), 0.032f);

        // Point Light #2
        // Posicion inicial ajustable
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].position"), -2.0f, 2.0f, -2.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].ambient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].diffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].specular"), 1.0f, 1.0f, 1.0f);

        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].quadratic"), 0.032f);

        // Spot Lights
        glUniform3fv(glGetUniformLocation(shaderProgram, "spotLight.position"), 1, glm::value_ptr(camera.position));
        glUniform3fv(glGetUniformLocation(shaderProgram, "spotLight.direction"), 1, glm::value_ptr(camera.front));
        // Angulos de la linterna: 12.5 grados internos, 15.0 externos para difuminar
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.outerCutOff"), glm::cos(glm::radians(15.0f)));
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.linear"), 0.09f);
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.quadratic"), 0.032f);
        glUniform3f(glGetUniformLocation(shaderProgram, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "spotLight.diffuse"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "spotLight.specular"), 1.0f, 1.0f, 1.0f);


        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

        // Renderizar solo mallas opacas
        for (const Mesh& m : escena) {
            if (!m.esTrasparente()) {
                m.dibujar(shaderProgram);
            }
        }

        // Renderizar solo mallas transparentes
        for (const Mesh& m : escena) {
            if (m.esTrasparente()) {
                m.dibujar(shaderProgram);
            }
        }

        // Marcado visual del triangulo
        if (objSeleccionado >= 0 && objSeleccionado < escena.size()) {
            Mesh& m = escena[objSeleccionado];

            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(4.0f);

            glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 0.0f, 1.0f);
            glUniform3f(glGetUniformLocation(shaderProgram, "ambientLight"), 1.0f, 1.0f, 1.0f);
            glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 0.0f, 0.0f, 0.0f);

            if (modoSeleccion == 0) {
                // Modo Global: Dibujar todas las submallas del objeto
                for (const SubMesh& sm : m.subMeshes) {
                    glm::mat4 modelFinal = m.getMatrizGlobal() * sm.getMatrizLocal();
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelFinal));
                    glBindVertexArray(sm.getVAO());
                    glDrawArrays(GL_TRIANGLES, 0, sm.getTotalVertices());
                }
            }
            else if (modoSeleccion == 1 && subMeshSeleccionadoId != -1) {
                // Modo Local: Dibujar solo la submalla especifica
                for (const SubMesh& sm : m.subMeshes) {
                    if (sm.id == subMeshSeleccionadoId) {
                        glm::mat4 modelFinal = m.getMatrizGlobal() * sm.getMatrizLocal();
                        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelFinal));
                        glBindVertexArray(sm.getVAO());
                        glDrawArrays(GL_TRIANGLES, 0, sm.getTotalVertices());
                        break;
                    }
                }
            }
            else if (modoSeleccion == 2 && triSeleccionadoGlobal != -1 && subMeshSeleccionadoId != -1) {
                // Modo Triangulo: Dibujar un solo triangulo
                for (const SubMesh& sm : m.subMeshes) {
                    if (sm.id == subMeshSeleccionadoId) {
                        glm::mat4 modelFinal = m.getMatrizGlobal() * sm.getMatrizLocal();
                        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelFinal));
                        glBindVertexArray(sm.getVAO());
                        glDrawArrays(GL_TRIANGLES, triSeleccionadoGlobal * 3, 3);
                        break;
                    }
                }
            }

            glBindVertexArray(0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_DEPTH_TEST);
            glLineWidth(1.0f);
        }

        if (miCielo != nullptr) {
            miCielo->dibujar(view, proj);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Renderizar el sistema de partículas al final
        if (emitirParticulas) {
            particleSystem->Draw(shaderProgram, view, proj, camera.position);
        }
    }

    void drawUI() override {

        // Panel General (Escena y Creacion)
        ImGui::Begin("Panel de Control 3D");
        ImGui::Text("Rendimiento: %.1f FPS", ImGui::GetIO().Framerate);
        ImGui::ColorEdit3("Color de Fondo", glm::value_ptr(clearColor));
        ImGui::Checkbox("Activar Depth Test", &depthTestActivo);
        ImGui::Checkbox("Activar Back-Face Culling", &cullFaceActivo);

        ImGui::Separator();

        ImGui::Text("Modo de Seleccion (Click en Pantalla):");
        ImGui::RadioButton("Global (Objeto)", &modoSeleccion, 0); ImGui::SameLine();
        ImGui::RadioButton("Local (Sub-malla)", &modoSeleccion, 1); ImGui::SameLine();
        ImGui::RadioButton("Por Triangulo", &modoSeleccion, 2);

        ImGui::Separator();

        ImGui::Text("Modelo de Iluminacion:");
        ImGui::RadioButton("Flat Shading", &modIluminacion, 0); ImGui::SameLine();
        ImGui::RadioButton("Lambert (Mate)", &modIluminacion, 1); ImGui::SameLine();
        ImGui::RadioButton("Blinn-Phong (Brillo)", &modIluminacion, 2);

        ImGui::Separator();
        ImGui::Text("Particulas:");
        if (ImGui::Button("Emitir Particulas")) {
            emitirParticulas = !emitirParticulas;
        }
        if (emitirParticulas) {

            ImGui::SliderFloat("Tamaño Inicial", &particleSystem->Config.startSize, 0.01f, 1.0f);
            ImGui::DragFloat3("Posicion", glm::value_ptr(*emisorPos), 0.05f);
            ImGui::SliderFloat("Apertura Cono", &particleSystem->Config.coneSpread, 0.0f, 5.0f);
            ImGui::SliderFloat("Velocidad Mínima", &particleSystem->Config.minSpeed, 0.1f, 5.0f);
            ImGui::SliderFloat("Velocidad Máxima", &particleSystem->Config.maxSpeed, 5.0f, 10.0f);
            ImGui::ColorEdit4("Color Base", &particleSystem->Config.baseColor[0]);

            ImGui::SliderInt("Direccion", &particleSystem->Config.direction, 0, 5);
            // La cantidad de partículas por frame también la puedes controlar dinámicamente aquí
            ImGui::SliderInt("Tasa de Emisión", &particlesPerFrame, 0, 50);

        }

        ImGui::Separator();
        ImGui::Text("Texturas:");
        if (ImGui::Button("Crear objeto con textura")) {
            std::cout << "Ingresa el nombre de la textura difusa. Ej: 'ladrillo.jpg': ";
            std::cin >> textActual.rutaDifusa;
            
            std::string rutaCompleta = "assets/textures/" + textActual.rutaDifusa;
            std::ifstream file(rutaCompleta);
            if (!file.is_open()) {
                std::cerr << "Error: La textura difusa no se encuentra en /assets/textures" << std::endl;
                textActual.tieneTextura = false;
                tieneTextura = false;
            } else {
                textActual.tieneTextura = true;
                tieneTextura = true;
                
                // Verificamos si la textura tiene normal Map
                char resp;
                std::cout << "¿Tiene Normal Map (Relieve)? (s/n): ";
                std::cin >> resp;
                
                if (resp == 's' || resp == 'S') {
                    std::cout << "Ingresa el nombre del Normal Map. Ej: 'ladrillo_normal.jpg': ";
                    std::cin >> textActual.rutaNormal;
                    std::string rutaNormalCompleta = "assets/textures/" + textActual.rutaNormal;
                    std::ifstream fileNormal(rutaNormalCompleta);
                    
                    if (!fileNormal.is_open()) {
                        std::cerr << "Error: Normal Map no encontrado. Se desactivara el relieve." << std::endl;
                        textActual.tieneNormalMap = false;
                    } else {
                        textActual.tieneNormalMap = true;
                    }
                } else {
                    textActual.tieneNormalMap = false;
                }
            }
        }
        
        if (tieneTextura) { 
            ImGui::Text("Difusa: %s", textActual.rutaDifusa.c_str()); 
            if (textActual.tieneNormalMap) {
                ImGui::Text("Relieve (Normal): %s", textActual.rutaNormal.c_str()); 
            }
            
            if (ImGui::Button("Desactivar Texturas")) {
                tieneTextura = false;
                textActual.tieneTextura = false;
                textActual.tieneNormalMap = false;
                textActual.rutaDifusa = "";
                textActual.rutaNormal = "";
            }
        }
        ImGui::Separator();

        ImGui::Text("Insertar Figuras:");
        if (ImGui::Button("Cubo")) {
            escena.push_back(Mesh::crearCubo(contadorID, contadorID, textActual));
            objSeleccionado = static_cast<int>(escena.size()) - 1;
            contadorID++;
        }
        ImGui::SameLine();
        if (ImGui::Button("Piramide")) {
            escena.push_back(Mesh::crearPiramide(contadorID, contadorID, textActual));
            objSeleccionado = static_cast<int>(escena.size()) - 1;
            contadorID++;
        }
        ImGui::SameLine();
        if (ImGui::Button("Esfera")) {
            escena.push_back(Mesh::crearEsfera(contadorID, contadorID));
            objSeleccionado = static_cast<int>(escena.size()) - 1;
            contadorID++;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cilindro")) {
            escena.push_back(Mesh::crearCilindro(contadorID, contadorID));
            objSeleccionado = static_cast<int>(escena.size()) - 1;
            contadorID++;
        }
        ImGui::SameLine();
        if (ImGui::Button(".OBJ")) {
            std::cout << "Escribe la ruta del archivo .obj: ";
            std::string ruta;
            std::cin >> ruta;
            std::string rutaCompleta = "assets/models/" + ruta + ".obj";
            std::ifstream file(rutaCompleta);
            if (file.is_open()) {
                escena.push_back(Mesh::crearOBJ(contadorID, rutaCompleta));
                objSeleccionado = static_cast<int>(escena.size()) - 1;
                contadorID++;
            }
            else {
                std::cerr << "Error: No se puede abrir el archivo .obj: " << rutaCompleta << std::endl;
            }
        }
        ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Espaciado

        if (ImGui::Button("Borrar Escena Completa", ImVec2(-1, 0))) {
            for (Mesh& m : escena) m.limpiarGPU();
            escena.clear();
            objSeleccionado = -1;
        }

        ImGui::End();
        // Inspector de objetos (Solo visible si hay seleccion)

        if (!escena.empty() && objSeleccionado >= 0 && objSeleccionado < static_cast<int>(escena.size())) {
            Mesh& actual = escena[objSeleccionado];

            ImGui::Begin("Inspector de Propiedades");
            ImGui::SliderInt("Navegar Escena", &objSeleccionado, 0, static_cast<int>(escena.size()) - 1);
            ImGui::Separator();

            ImGui::Text("Objeto: %s", actual.nombre.c_str());
            ImGui::DragFloat3("Posicion", glm::value_ptr(actual.position), 0.05f);
            ImGui::DragFloat3("Rotacion", glm::value_ptr(actual.rotation), 1.0f);
            ImGui::DragFloat3("Escala", glm::value_ptr(actual.scale), 0.05f, 0.05f, 10.0f);

            ImGui::Separator();
            ImGui::Text("Apariencia:");
            if (!actual.subMeshes.empty()) {
                for (size_t i = 0; i < actual.subMeshes.size(); ++i) {
                    ImGui::PushID(static_cast<int>(i)); // Empuja el índice actual al stack de IDs de ImGui

                    std::string etiqueta = "SubMesh " + std::to_string(i) + " Color";
                    ImGui::ColorEdit4(etiqueta.c_str(), glm::value_ptr(actual.subMeshes[i].color));

                    ImGui::PopID(); // Restaura el stack de IDs
                }
            }

            ImGui::Checkbox("Modo Wireframe", &actual.wireframe);
            ImGui::Checkbox("Visualizar Normales", &actual.seeNormals);
            ImGui::Checkbox("Visualizar Vertices", &actual.seeVertex);
            ImGui::Checkbox("Visualizar Bounding Box", &actual.seeBoudingBox);

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            // Boton rojo para eliminar
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Eliminar Objeto", ImVec2(-1, 0))) {
                actual.limpiarGPU();
                escena.erase(escena.begin() + objSeleccionado);
                objSeleccionado = escena.empty() ? -1 : static_cast<int>(escena.size()) - 1;
            }
            ImGui::PopStyleColor(3);

            ImGui::End();
        }

        // Cargado y guardado de escena
        ImGui::Begin("Guardar y Cargar Escena");

        static char nombreArchivoGuardar[64] = "mi_escena";
        ImGui::InputText(".json", nombreArchivoGuardar, IM_ARRAYSIZE(nombreArchivoGuardar));
        ImGui::SameLine();
        if (ImGui::Button("Guardar Archivo")) {
            guardarEscena(std::string(nombreArchivoGuardar));
        }

        ImGui::Separator();
        ImGui::Text("Archivos Guardados (Saves/):");

        // Leer la carpeta nativamente 
        if (fs::exists("Saves")) {
            for (const auto& entry : fs::directory_iterator("Saves")) {
                if (entry.path().extension() == ".json") {
                    std::string nombreArchivo = entry.path().filename().string();
                    if (ImGui::Button(("Cargar " + nombreArchivo).c_str())) {
                        cargarEscena(entry.path().string());
                    }
                }
            }
        }
        ImGui::End();
    }
};

int main() {
    Proyecto2 app;
    app.run();
    return 0;
}