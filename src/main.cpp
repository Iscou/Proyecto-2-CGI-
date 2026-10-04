#include "Engine/engine3D.h"
#include "Camera/camera.h"
#include "Objects/Mesh.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>

// Shaders
const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec4 objectColor;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientLight;

void main() {
    vec3 ambient = ambientLight * objectColor.rgb;
    
    vec3 norm = normalize(Normal);
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor * objectColor.rgb;
    
    vec3 result = ambient + diffuse;
    FragColor = vec4(result, objectColor.a);
}
)";

class Proyecto2 : public Engine3D {
private:
    Camera camera;
    GLuint shaderProgram;

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

    void compilarShaders() {
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertexShaderSrc, nullptr);
        glCompileShader(vs);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragmentShaderSrc, nullptr);
        glCompileShader(fs);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glLinkProgram(shaderProgram);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

public:
    Proyecto2() : Engine3D(1280, 720, "Proyecto #2 - Renderizado y Manipulacion 3D") {}

    void setup() override {
        compilarShaders();
        escena.push_back(Mesh::crearCubo(contadorID, contadorID));
        contadorID++;
        glEnable(GL_DEPTH_TEST);
    }

    void onMouseButtonDown(int button, double x, double y) override {
        if (ImGui::GetIO().WantCaptureMouse) return;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            rotandoCamara = true;
            lastMouseX = x;
            lastMouseY = y;
        }
    }

    void onMouseButtonUp(int button, double x, double y) override {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            rotandoCamara = false;
        }
    }

    void onMouseMove(double x, double y) override {
        if (rotandoCamara) {
            float xoffset = static_cast<float>(x - lastMouseX);
            float yoffset = static_cast<float>(lastMouseY - y);
            lastMouseX = x;
            lastMouseY = y;
            camera.processMouseMovement(xoffset, yoffset);
        }
    }

    void update(float deltaTime) override {
        if (!ImGui::GetIO().WantCaptureKeyboard) {
            camera.processKeyboard(
                isKeyPressed(GLFW_KEY_W),
                isKeyPressed(GLFW_KEY_S),
                isKeyPressed(GLFW_KEY_A),
                isKeyPressed(GLFW_KEY_D),
                isKeyPressed(GLFW_KEY_SPACE),
                isKeyPressed(GLFW_KEY_LEFT_SHIFT),
                deltaTime
            );
        }

        if (depthTestActivo) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);

        if (cullFaceActivo) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        } else {
            glDisable(GL_CULL_FACE);
        }

        glUseProgram(shaderProgram);

        glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"), -0.5f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(shaderProgram, "ambientLight"), 0.25f, 0.25f, 0.25f);

        float aspect = (height > 0) ? (static_cast<float>(width) / height) : 1.0f;
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(aspect);

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

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void drawUI() override {
        ImGui::Begin("Panel de Control 3D");
        ImGui::Text("Rendimiento: %.1f FPS", ImGui::GetIO().Framerate);
        ImGui::ColorEdit3("Color de Fondo", glm::value_ptr(clearColor));
        ImGui::Checkbox("Activar Depth Test", &depthTestActivo);
        ImGui::Checkbox("Activar Back-Face Culling", &cullFaceActivo);
        ImGui::Separator();

        // Botones para crear primitivas
        ImGui::Text("Crear Primitivas:");
        if (ImGui::Button("Cubo")) {
            escena.push_back(Mesh::crearCubo(contadorID, contadorID));
            objSeleccionado = static_cast<int>(escena.size()) - 1;
            contadorID++;
        }
        ImGui::SameLine();
        if (ImGui::Button("Piramide")) {
            escena.push_back(Mesh::crearPiramide(contadorID, contadorID));
            objSeleccionado = static_cast<int>(escena.size()) - 1;
            contadorID++;
        }
        ImGui::SameLine();
        if (ImGui::Button("Esfera")) {
            escena.push_back(Mesh::crearEsfera(contadorID, contadorID));
            objSeleccionado = static_cast<int>(escena.size()) - 1;
            contadorID++;
        }
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
        if (ImGui::Button("Borrar Escena Completa")) {
            for (Mesh& m : escena) m.limpiarGPU();
            escena.clear();
            objSeleccionado = -1;
        }
        ImGui::Separator();

        // Selector e inspector del objeto actual
        if (!escena.empty() && objSeleccionado >= 0 && objSeleccionado < static_cast<int>(escena.size())) {
            ImGui::SliderInt("Seleccionar Objeto", &objSeleccionado, 0, static_cast<int>(escena.size()) - 1);
            Mesh& actual = escena[objSeleccionado];

            ImGui::Text("Editando: %s", actual.nombre.c_str());
            ImGui::DragFloat3("Posicion", glm::value_ptr(actual.position), 0.05f);
            ImGui::DragFloat3("Rotacion", glm::value_ptr(actual.rotation), 1.0f);
            ImGui::DragFloat3("Escala", glm::value_ptr(actual.scale), 0.05f, 0.05f, 10.0f);

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

            if (ImGui::Button("Eliminar Objeto")) {
                actual.limpiarGPU();
                escena.erase(escena.begin() + objSeleccionado);
                if (objSeleccionado >= static_cast<int>(escena.size())) {
                    objSeleccionado = static_cast<int>(escena.size()) - 1;
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