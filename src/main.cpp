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
#include "Jugador.h"
#include "Bot.h"
#include "Arma.h"
#include "Bala.h"
#include <random> 

namespace fs = std::filesystem;
using json = nlohmann::json;

// Shaders
const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec2 TexCoords;
out mat3 TBN; 

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoords = aTexCoords;

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);
    TBN = mat3(T, B, N);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN; 

out vec4 FragColor;

struct DirLight { vec3 direction; vec3 ambient; vec3 diffuse; vec3 specular; };
struct PointLight { vec3 position; float constant; float linear; float quadratic; vec3 ambient; vec3 diffuse; vec3 specular; };
struct SpotLight { vec3 position; vec3 direction; float cutOff; float outerCutOff; float constant; float linear; float quadratic; vec3 ambient; vec3 diffuse; vec3 specular; };

uniform bool hasTexture;
uniform sampler2D ourTexture;
uniform bool usarNormalMap;      
uniform sampler2D texturaNormal; 
uniform vec4 objectColor;

uniform int modoIluminacion; 
uniform vec3 viewPos;        
uniform DirLight dirLight;
#define NR_POINT_LIGHTS 2
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);

void main() {
    vec4 baseColorAlpha = objectColor;
    if (hasTexture) {
        baseColorAlpha *= texture(ourTexture, TexCoords); 
    }
    
    if(baseColorAlpha.a < 0.1) discard; 

    vec3 norm;
    if (modoIluminacion == 0) {
        vec3 fdx = dFdx(FragPos);
        vec3 fdy = dFdy(FragPos);
        norm = normalize(cross(fdx, fdy));
    } else {
        if (usarNormalMap) {
            norm = texture(texturaNormal, TexCoords).rgb;
            norm = normalize(norm * 2.0 - 1.0);
            norm.y = -norm.y; // Correccion Y- para mapas de DirectX (Unreal/Valorant)
            norm = normalize(TBN * norm);
        } else {
            norm = normalize(TBN[2]);
        }
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);
    vec3 baseColorRgb = baseColorAlpha.rgb;

    result += CalcDirLight(dirLight, norm, viewDir, baseColorRgb);
    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, baseColorRgb);    
    }
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir, baseColorRgb);    
    
    FragColor = vec4(result, baseColorAlpha.a);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(-light.direction);
    vec3 ambient = light.ambient * baseColor;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * baseColor;
    vec3 specular = vec3(0.0);
    if (modoIluminacion == 2) {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0); 
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
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
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
void main() { gl_Position = projection * view * model * vec4(aPos, 1.0); }
)";

const char* pickingFragmentShaderSrc = R"(
#version 330 core
uniform uint meshID;
uniform uint subMeshID;
out uvec3 FragColor; 
void main() { FragColor = uvec3(meshID, subMeshID, uint(gl_PrimitiveID)); }
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
void main() { FragColor = texture(skybox, TexCoords); }
)";

class Proyecto2 : public Engine3D {
private:
    Camera camera;
    GLuint shaderProgram;
    GLuint pickingShaderProgram;
    GLuint pickingFBO;
    GLuint pickingTexture;
    GLuint pickingDepth;
    GLuint skyboxShaderProgram;
    SkyBox* miCielo = nullptr;
    ParticleSystem* particleSystem = new ParticleSystem(1000); 
    glm::vec3* emisorPos = new glm::vec3(0.0f, 0.0f, 0.0f);
    bool emitirParticulas = false;
    int particlesPerFrame = 5;
    TexturaInfo textActual;
    bool tieneTextura = false;

    std::vector<std::string> carasCielo = {
        "assets/skybox/right.jpg", "assets/skybox/left.jpg",
        "assets/skybox/top.jpg", "assets/skybox/bottom.jpg",
        "assets/skybox/front.jpg", "assets/skybox/back.jpg"
    };

    bool modoLibre = false;
    bool tabPresionado = false;
    int triSeleccionadoGlobal = -1;
    int subMeshSeleccionadoId = -1;
    int modIluminacion = 1; 

    std::vector<Mesh> escena;
    int objSeleccionado = 0;
    int contadorID = 1;
    bool rotandoCamara = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool depthTestActivo = true;
    bool cullFaceActivo = false;
    int modoSeleccion = 0; 

    // Variables Shooter 
    bool modoJuego = false;
    bool juegoPausado = false;
    bool escPresionado = false;
    int modoPractica = 0; 
    int puntaje = 0;
    float spawnerTimer = 0.0f;
    float tiempoAparicion = 1.5f; 

    Jugador* jugador = nullptr;
    std::vector<Bot*> botsActivos;
    std::vector<Bala*> balasActivas;

    bool comprobarColision(AABB a, AABB b) {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
               (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
               (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }

    void guardarEscena(const std::string& nombreArchivo) {
        if (!fs::exists("Saves")) fs::create_directory("Saves");
        json jEscena = json::array();
        for (const Mesh& m : escena) {
            json jMesh;
            jMesh["nombre"] = m.nombre;
            jMesh["pos"] = { m.position.x, m.position.y, m.position.z };
            jMesh["rot"] = { m.rotation.x, m.rotation.y, m.rotation.z };
            jMesh["scl"] = { m.scale.x, m.scale.y, m.scale.z };
            if (!m.subMeshes.empty()) {
                jMesh["color"] = { m.subMeshes[0].color.r, m.subMeshes[0].color.g, m.subMeshes[0].color.b, m.subMeshes[0].color.a };
            }
            jEscena.push_back(jMesh);
        }
        std::ofstream archivo("Saves/" + nombreArchivo + ".json");
        archivo << jEscena.dump(4);
    }

    void cargarEscena(const std::string& rutaCompleta) {
        std::ifstream archivo(rutaCompleta);
        if (!archivo.is_open()) return;
        json jEscena;
        archivo >> jEscena;
        for (Mesh& m : escena) m.limpiarGPU();
        escena.clear();
        objSeleccionado = -1;
        contadorID = 1;

        for (const auto& jMesh : jEscena) {
            std::string nombreGuardado = jMesh["nombre"];
            Mesh m;
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

        glDeleteShader(vs); glDeleteShader(fs); glDeleteShader(pv); glDeleteShader(pf);

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

        glDeleteShader(sv); glDeleteShader(sf);
    }

public:
    Proyecto2() : Engine3D(1280, 720, "Proyecto #2 - Renderizado y Manipulacion 3D") {}
    ~Proyecto2() {
        delete miCielo;
        delete jugador;
        for (Bot* bot : botsActivos) delete bot;
        for (Bala* bala : balasActivas) delete bala;
    }

    void setupFramebuffer() {
        glGenFramebuffers(1, &pickingFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

        glGenTextures(1, &pickingTexture);
        glBindTexture(GL_TEXTURE_2D, pickingTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTexture, 0);

        glGenRenderbuffers(1, &pickingDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, pickingDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pickingDepth);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void setup() override {
        compilarShaders();
        setupFramebuffer();

        miCielo = new SkyBox(999, 999, skyboxShaderProgram);
        miCielo->cargarCubemap(carasCielo);

        // Plataforma inicial
        Mesh piso = Mesh::crearCubo(contadorID++, contadorID++);
        piso.scale = glm::vec3(50.0f, 0.1f, 50.0f);
        piso.position = glm::vec3(0.0f, -0.05f, 0.0f); // Justo debajo de los pies
        piso.subMeshes[0].color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f); // Gris oscuro 
        escena.push_back(piso);

        // Bala
        Mesh* modeloBala = new Mesh(Mesh::crearEsfera(9999, 9999, 0.01f)); 
        Bala* balaBase = new Bala(100.0f, 400.0f, modeloBala, 1.5f, 0.8f, 0.5f, 0.8f, 10.0f, 30.0f, 100.0f, glm::vec3(0), glm::vec3(0));
        
        // Arma
        Mesh* modeloPistola = new Mesh(Mesh::crearOBJ(8888, "assets/models/Pistol.obj")); 
        modeloPistola->scale = glm::vec3(0.1f, 0.1f, 0.1f); 
        
        GLuint texDifusa = cargarTextura("colt_low_polyfbx_m1911_BaseColor.png");
        GLuint texNormal = cargarTextura("colt_low_polyfbx_m1911_Normal.png");
        
        for (SubMesh& sm : modeloPistola->subMeshes) {
            sm.hasTexture = true;
            sm.texturaDifusaID = texDifusa;
            
            // APAGADO TEMPORALMENTE: Si ObjLoader no calcula tangentes, el Normal Map corromperá la luz y se verá negro.
            sm.tieneNormalMap = false; 
            sm.texturaNormalID = texNormal;
            sm.color = glm::vec4(1.0f); 
        }

        Arma* pistola = new Arma(true, 7, 35, balaBase, glm::vec3(0), modeloPistola); 
        
        jugador = new Jugador(glm::vec3(0.0f, 1.0f, 0.0f), pistola);
        camera = *(jugador->getCamara()); 

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }

    void onMouseButtonDown(int button, double x, double y) override {
        if (modoJuego) {
            if (button == GLFW_MOUSE_BUTTON_LEFT && !juegoPausado) {
                Bala* nuevaBala = jugador->disparar();
                if (nuevaBala != nullptr) {
                    balasActivas.push_back(nuevaBala);
                }
            }
            return; 
        } else {
            if (ImGui::GetIO().WantCaptureMouse || modoLibre) return;

            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
                GLuint pixelIDs[3];
                glReadPixels(static_cast<GLint>(x), static_cast<GLint>(height - y), 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, pixelIDs);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                if (pixelIDs[0] > 0) {
                    for (int i = 0; i < escena.size(); ++i) {
                        if (escena[i].id == pixelIDs[0]) {
                            objSeleccionado = i;
                            break;
                        }
                    }
                    if (modoSeleccion == 1) { subMeshSeleccionadoId = pixelIDs[1]; triSeleccionadoGlobal = -1; }
                    else if (modoSeleccion == 2) { subMeshSeleccionadoId = pixelIDs[1]; triSeleccionadoGlobal = pixelIDs[2]; }
                    else { subMeshSeleccionadoId = -1; triSeleccionadoGlobal = -1; }
                } else {
                    subMeshSeleccionadoId = -1; triSeleccionadoGlobal = -1;
                }
            }

            if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                rotandoCamara = true;
                lastMouseX = x;
                lastMouseY = y;
            }
        }
    }

    void onMouseMove(double x, double y) override {
        float xoffset = static_cast<float>(x - lastMouseX);
        float yoffset = static_cast<float>(lastMouseY - y);
        lastMouseX = x;
        lastMouseY = y;

        if (modoJuego && !juegoPausado) {
            jugador->rotarVista(xoffset * 0.3f, yoffset * 0.3f);
        } else if (modoLibre || rotandoCamara) {
            camera.processMouseMovement(xoffset * 0.3f, yoffset * 0.3f);
        }
    }

    void onMouseButtonUp(int button, double x, double y) override {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            rotandoCamara = false;
        }
    }

    void update(float deltaTime) override {
        // Toggle Modo Juego (TAB)
        if (isKeyPressed(GLFW_KEY_TAB)) {
            if (!tabPresionado) {
                modoJuego = !modoJuego;
                modoLibre = false; 
                juegoPausado = false; 
                glfwSetInputMode(getWindow(), GLFW_CURSOR, modoJuego ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                if (modoJuego) {
                    double cx, cy; glfwGetCursorPos(getWindow(), &cx, &cy);
                    lastMouseX = cx; lastMouseY = cy;
                }
                tabPresionado = true;
            }
        } else { tabPresionado = false; }

        // Toggle Pausa (ESC)
        if (modoJuego) {
            if (isKeyPressed(GLFW_KEY_ESCAPE)) {
                if (!escPresionado) {
                    juegoPausado = !juegoPausado;
                    glfwSetInputMode(getWindow(), GLFW_CURSOR, juegoPausado ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
                    if (!juegoPausado) {
                        double cx, cy; glfwGetCursorPos(getWindow(), &cx, &cy);
                        lastMouseX = cx; lastMouseY = cy;
                    }
                    escPresionado = true;
                }
            } else { escPresionado = false; }
        }

        if (modoJuego) {
            if (!juegoPausado) {
                jugador->mover(isKeyPressed(GLFW_KEY_W), isKeyPressed(GLFW_KEY_S), isKeyPressed(GLFW_KEY_A), isKeyPressed(GLFW_KEY_D), isKeyPressed(GLFW_KEY_LEFT_SHIFT), deltaTime);
                if (isKeyPressed(GLFW_KEY_SPACE)) jugador->saltar();
                if (isKeyPressed(GLFW_KEY_R)) jugador->recargar();
            }
            
            jugador->update(deltaTime);
            camera = *(jugador->getCamara()); 

            if (!juegoPausado) {
                if (modoPractica > 0) {
                    spawnerTimer += deltaTime;
                    if (spawnerTimer >= tiempoAparicion) {
                        spawnerTimer = 0.0f;
                        float rx = (rand() % 2000 / 100.0f) - 10.0f; 
                        float ry = (rand() % 500 / 100.0f) + 1.0f;   
                        glm::vec3 posAleatoria = glm::vec3(rx, ry, -15.0f); 

                        Mesh* modeloBot = new Mesh(Mesh::crearCubo(contadorID++, contadorID++));
                        modeloBot->subMeshes[0].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); 
                        bool dinamico = (modoPractica == 2);
                        Bot* nuevoBot = new Bot(posAleatoria, modeloBot, 100.0f, 10, dinamico, dinamico ? 3.0f : 0.0f);
                        botsActivos.push_back(nuevoBot);
                    }
                }

                for (auto itBala = balasActivas.begin(); itBala != balasActivas.end(); ) {
                    Bala* b = *itBala;
                    b->actualizarPos(deltaTime);
                    for (Bot* bot : botsActivos) {
                        if (!bot->getEstaMuerto() && comprobarColision(b->getHitBox(), bot->getHitBox())) {
                            b->calcularDMGFinal(bot, 'c'); 
                            if (bot->getEstaMuerto()) puntaje += 10;
                            break; 
                        }
                    }
                    if (b->getModelo()->borrame) { 
                        delete b;
                        itBala = balasActivas.erase(itBala);
                    } else {
                        ++itBala;
                    }
                }

                for (auto itBot = botsActivos.begin(); itBot != botsActivos.end(); ) {
                    (*itBot)->update(deltaTime);
                    if ((*itBot)->getEstaMuerto()) {
                        delete *itBot;
                        itBot = botsActivos.erase(itBot);
                    } else {
                        ++itBot;
                    }
                }
            }
        }

        glm::vec3 posicionAnterior = camera.position;
        if (modoLibre && !ImGui::GetIO().WantCaptureKeyboard && !modoJuego) {
            camera.processKeyboard(
                isKeyPressed(GLFW_KEY_W), isKeyPressed(GLFW_KEY_S),
                isKeyPressed(GLFW_KEY_A), isKeyPressed(GLFW_KEY_D),
                isKeyPressed(GLFW_KEY_SPACE), isKeyPressed(GLFW_KEY_LEFT_SHIFT),
                deltaTime
            );
        }

        if (camera.position != posicionAnterior) {
            glm::vec3 posFutura = camera.position;
            glm::vec3 radioJugador(0.2f, 0.8f, 0.2f);

            camera.position = glm::vec3(posFutura.x, posicionAnterior.y, posicionAnterior.z);
            AABB cajaX = { camera.position - radioJugador, camera.position + radioJugador };
            for (const Mesh& m : escena) {
                if (!m.esDinamico && comprobarColision(cajaX, m.obtenerAABBGlobal())) {
                    camera.position.x = posicionAnterior.x; 
                    break;
                }
            }
            posFutura.x = camera.position.x; 

            camera.position = glm::vec3(posFutura.x, posFutura.y, posicionAnterior.z);
            AABB cajaY = { camera.position - radioJugador, camera.position + radioJugador };
            for (const Mesh& m : escena) {
                if (!m.esDinamico && comprobarColision(cajaY, m.obtenerAABBGlobal())) {
                    camera.position.y = posicionAnterior.y; 
                    break;
                }
            }
            posFutura.y = camera.position.y;

            camera.position = glm::vec3(posFutura.x, posFutura.y, posFutura.z);
            AABB cajaZ = { camera.position - radioJugador, camera.position + radioJugador };
            for (const Mesh& m : escena) {
                if (!m.esDinamico && comprobarColision(cajaZ, m.obtenerAABBGlobal())) {
                    camera.position.z = posicionAnterior.z; 
                    break;
                }
            }
        }

        for (size_t i = 0; i < escena.size(); i++) {
            if (escena[i].esDinamico && glm::length(escena[i].velocidad) > 0.0f) {
                glm::vec3 posAnterior = escena[i].position;
                escena[i].position += escena[i].velocidad * deltaTime; 
                AABB cajaDinamica = escena[i].obtenerAABBGlobal();
                bool choco = false;
                for (size_t j = 0; j < escena.size(); j++) {
                    if (i == j) continue; 
                    AABB cajaOtro = escena[j].obtenerAABBGlobal();
                    if (comprobarColision(cajaDinamica, cajaOtro)) {
                        choco = true;
                        escena[i].position = posAnterior;
                        escena[i].velocidad = glm::vec3(0.0f);
                        break;
                    }
                }
            }
        }

        for (auto it = escena.begin(); it != escena.end(); ) {
            if (it->borrame) {
                it->limpiarGPU();
                it = escena.erase(it);
                objSeleccionado = -1; 
            } else { ++it; }
        }

        if (depthTestActivo) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (cullFaceActivo) { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); } else glDisable(GL_CULL_FACE);

        float aspect = (height > 0) ? (static_cast<float>(width) / height) : 1.0f;
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(aspect);

        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
        GLuint clearColorZeros[4] = { 0, 0, 0, 0 };
        glClearBufferuiv(GL_COLOR, 0, clearColorZeros);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(pickingShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(pickingShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(pickingShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

        for (const Mesh& m : escena) {
            glUniform1ui(glGetUniformLocation(pickingShaderProgram, "meshID"), static_cast<GLuint>(m.id));
            glm::mat4 matGlobal = m.getMatrizGlobal();
            for (const SubMesh& sm : m.subMeshes) {
                glUniform1ui(glGetUniformLocation(pickingShaderProgram, "subMeshID"), static_cast<GLuint>(sm.id));
                glm::mat4 modelFinal = matGlobal * sm.getMatrizLocal();
                glUniformMatrix4fv(glGetUniformLocation(pickingShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelFinal));
                glBindVertexArray(sm.getVAO());
                glDrawArrays(GL_TRIANGLES, 0, sm.getTotalVertices());
                glBindVertexArray(0);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (emitirParticulas) particleSystem->Update(deltaTime, *emisorPos, particlesPerFrame);

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "modoIluminacion"), modIluminacion);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera.position));
        glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.ambient"), 0.1f, 0.1f, 0.1f); 
        glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.diffuse"), 0.4f, 0.4f, 0.4f); 
        glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.specular"), 0.5f, 0.5f, 0.5f); 
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].position"), 2.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].ambient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].diffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[0].specular"), 0.2f, 0.2f, 0.2f); 
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[0].quadratic"), 0.032f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].position"), -2.0f, 2.0f, -2.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].ambient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].diffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(shaderProgram, "pointLights[1].specular"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(shaderProgram, "pointLights[1].quadratic"), 0.032f);
        glUniform3fv(glGetUniformLocation(shaderProgram, "spotLight.position"), 1, glm::value_ptr(camera.position));
        glUniform3fv(glGetUniformLocation(shaderProgram, "spotLight.direction"), 1, glm::value_ptr(camera.front));
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

        if (modoJuego) {
            if (jugador->getArma() != nullptr && jugador->getArma()->getModelo() != nullptr) {
                jugador->getArma()->getModelo()->dibujar(shaderProgram);
            }
            for (Bot* bot : botsActivos) {
                if (bot->getModelo() != nullptr) bot->getModelo()->dibujar(shaderProgram);
            }
            for (Bala* bala : balasActivas) {
                if (bala->getModelo() != nullptr) bala->getModelo()->dibujar(shaderProgram);
            }
        }

        for (const Mesh& m : escena) if (!m.esTrasparente()) m.dibujar(shaderProgram);
        for (const Mesh& m : escena) if (m.esTrasparente()) m.dibujar(shaderProgram);

        if (objSeleccionado >= 0 && objSeleccionado < escena.size()) {
            Mesh& m = escena[objSeleccionado];
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(4.0f);
            glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 0.0f, 1.0f);
            glUniform3f(glGetUniformLocation(shaderProgram, "ambientLight"), 1.0f, 1.0f, 1.0f);
            glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 0.0f, 0.0f, 0.0f);

            if (modoSeleccion == 0) {
                for (const SubMesh& sm : m.subMeshes) {
                    glm::mat4 modelFinal = m.getMatrizGlobal() * sm.getMatrizLocal();
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelFinal));
                    glBindVertexArray(sm.getVAO());
                    glDrawArrays(GL_TRIANGLES, 0, sm.getTotalVertices());
                }
            } else if (modoSeleccion == 1 && subMeshSeleccionadoId != -1) {
                for (const SubMesh& sm : m.subMeshes) {
                    if (sm.id == subMeshSeleccionadoId) {
                        glm::mat4 modelFinal = m.getMatrizGlobal() * sm.getMatrizLocal();
                        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelFinal));
                        glBindVertexArray(sm.getVAO());
                        glDrawArrays(GL_TRIANGLES, 0, sm.getTotalVertices());
                        break;
                    }
                }
            } else if (modoSeleccion == 2 && triSeleccionadoGlobal != -1 && subMeshSeleccionadoId != -1) {
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

        if (miCielo != nullptr) miCielo->dibujar(view, proj);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if (emitirParticulas) particleSystem->Draw(shaderProgram, view, proj, camera.position);
    }

    void drawUI() override {
        if (!modoJuego) {
            ImGui::Begin("Panel de Control 3D");
            ImGui::Text("Rendimiento: %.1f FPS", ImGui::GetIO().Framerate);
            ImGui::ColorEdit3("Color de Fondo", glm::value_ptr(clearColor));
            ImGui::Checkbox("Activar Depth Test", &depthTestActivo);
            ImGui::Checkbox("Activar Back-Face Culling", &cullFaceActivo);
            ImGui::Separator();
            ImGui::Text("Modo de Seleccion:");
            ImGui::RadioButton("Global (Objeto)", &modoSeleccion, 0); ImGui::SameLine();
            ImGui::RadioButton("Local (Sub-malla)", &modoSeleccion, 1); ImGui::SameLine();
            ImGui::RadioButton("Por Triangulo", &modoSeleccion, 2);
            ImGui::Separator();
            ImGui::Text("Modelo de Iluminacion:");
            ImGui::RadioButton("Flat Shading", &modIluminacion, 0); ImGui::SameLine();
            ImGui::RadioButton("Lambert (Mate)", &modIluminacion, 1); ImGui::SameLine();
            ImGui::RadioButton("Blinn-Phong (Brillo)", &modIluminacion, 2);
            ImGui::Separator();
            
            // Particulas y generacion geometrica del UI
            ImGui::Text("Particulas:");
            if (ImGui::Button("Emitir Particulas")) { emitirParticulas = !emitirParticulas; }
            if (emitirParticulas) {
                ImGui::SliderFloat("Tamaño Inicial", &particleSystem->Config.startSize, 0.01f, 1.0f);
                ImGui::DragFloat3("Posicion", glm::value_ptr(*emisorPos), 0.05f);
                ImGui::SliderFloat("Apertura Cono", &particleSystem->Config.coneSpread, 0.0f, 5.0f);
                ImGui::SliderFloat("Velocidad Minima", &particleSystem->Config.minSpeed, 0.1f, 5.0f);
                ImGui::SliderFloat("Velocidad Maxima", &particleSystem->Config.maxSpeed, 5.0f, 10.0f);
                ImGui::ColorEdit4("Color Base", &particleSystem->Config.baseColor[0]);
                ImGui::SliderInt("Direccion", &particleSystem->Config.direction, 0, 5);
                ImGui::SliderInt("Tasa de Emision", &particlesPerFrame, 0, 50);
            }

            ImGui::Separator();
            ImGui::Text("Insertar Figuras:");
            if (ImGui::Button("Cubo")) { escena.push_back(Mesh::crearCubo(contadorID, contadorID, textActual)); objSeleccionado = static_cast<int>(escena.size()) - 1; contadorID++; }
            ImGui::SameLine();
            if (ImGui::Button(".OBJ")) {
                std::string ruta = "pistola"; 
                std::string rutaCompleta = "assets/models/" + ruta + ".obj";
                std::ifstream file(rutaCompleta);
                if (file.is_open()) { escena.push_back(Mesh::crearOBJ(contadorID, rutaCompleta)); objSeleccionado = static_cast<int>(escena.size()) - 1; contadorID++; }
            }
            if (ImGui::Button("Borrar Escena", ImVec2(-1, 0))) { for (Mesh& m : escena) m.limpiarGPU(); escena.clear(); objSeleccionado = -1; }
            ImGui::End();
        } else {
            // HUD del Juego y Menu de Pausa
            if (!juegoPausado) {
                ImDrawList* drawList = ImGui::GetBackgroundDrawList();
                ImVec2 centroPantalla = ImVec2(width * 0.5f, height * 0.5f);
                drawList->AddCircleFilled(centroPantalla, 4.0f, IM_COL32(0, 255, 0, 255)); 
                
                ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
                ImGui::SetNextWindowBgAlpha(0.3f); 
                ImGui::Begin("HUD Shooter", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::Text("PUNTAJE: %d", puntaje);
                ImGui::Text("Pulsa ESC para Pausar");
                ImGui::End();
            } else {
                ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
                ImGui::SetNextWindowBgAlpha(0.8f); 
                ImGui::Begin("Menu de Pausa", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
                ImGui::Text("--- JUEGO PAUSADO ---");
                ImGui::Text("PUNTAJE ACTUAL: %d", puntaje);
                ImGui::Separator();
                ImGui::Text("Rutina de Entrenamiento:");
                ImGui::RadioButton("Gridshot (Estatico)", &modoPractica, 1);
                ImGui::RadioButton("Strafing (Movil)", &modoPractica, 2);
                ImGui::RadioButton("Detener Spawner", &modoPractica, 0);
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                if (ImGui::Button("Limpiar Arena", ImVec2(-1, 0))) {
                    for (Bot* bot : botsActivos) delete bot;
                    botsActivos.clear(); 
                    puntaje = 0;
                }
                ImGui::Separator();
                ImGui::Text("Pulsa ESC para reanudar el combate");
                ImGui::End();
            }
        }
    }
};

int main() { Proyecto2 app; app.run(); return 0; }