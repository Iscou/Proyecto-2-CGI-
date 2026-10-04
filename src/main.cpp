#include "Engine/engine3D.h"
#include "Camera/camera.h"
#include "Objects/Mesh.h"
#include <glm/gtc/type_ptr.hpp>

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

class Proyecto2 : public Engine3D {
private:
    Camera camera;
    GLuint shaderProgram;
    GLuint pickingShaderProgram;
    GLuint pickingFBO;
    GLuint pickingTexture;
    GLuint pickingDepth;

    // Camara 
    bool modoLibre = false;
    bool tabPresionado = false;
    int triSeleccionadoGlobal = -1;
    int subMeshSeleccionadoId = -1;

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
    }
   

public:
    Proyecto2() : Engine3D(1280, 720, "Proyecto #2 - Renderizado y Manipulacion 3D") {}

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
        escena.push_back(Mesh::crearCubo(contadorID, contadorID));
        contadorID++;
        glEnable(GL_DEPTH_TEST);
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
                } else if (modoSeleccion == 2) {
                    subMeshSeleccionadoId = pixelIDs[1];
                    triSeleccionadoGlobal = pixelIDs[2];
                } else {
                    subMeshSeleccionadoId = -1;
                    triSeleccionadoGlobal = -1; 
                }
            } else {
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
        } else {
            tabPresionado = false;
        }

        // Movimiento WASD
        if (modoLibre && !ImGui::GetIO().WantCaptureKeyboard) {
            camera.processKeyboard(
                isKeyPressed(GLFW_KEY_W), isKeyPressed(GLFW_KEY_S),
                isKeyPressed(GLFW_KEY_A), isKeyPressed(GLFW_KEY_D),
                isKeyPressed(GLFW_KEY_SPACE), isKeyPressed(GLFW_KEY_LEFT_SHIFT),
                deltaTime
            );
        }

        // Estados de OpenGL compartidos
        if (depthTestActivo) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);

        if (cullFaceActivo) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        } else {
            glDisable(GL_CULL_FACE);
        }

        // Calculo de matrices comunes
        float aspect = (height > 0) ? (static_cast<float>(width) / height) : 1.0f;
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(aspect);

        // Dibujar monitor oculto
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
        
        // Limpiamos el buffer de enteros usando [0,0,0] 
        GLuint clearColorZeros[4] = {0, 0, 0, 0};
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
        
        // Limpieza clasica de color visible
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Luces
        glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"), -0.5f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(shaderProgram, "ambientLight"), 0.25f, 0.25f, 0.25f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

        // Dibujar todos los objetos normalmente
        for (const Mesh& m : escena) {
            m.dibujar(shaderProgram);
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

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

        ImGui::Text("Insertar Figuras:");
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
        ImGui::SameLine();
        if (ImGui::Button("Cilindro")) {
            escena.push_back(Mesh::crearCilindro(contadorID, contadorID));
            objSeleccionado = static_cast<int>(escena.size()) - 1;
            contadorID++;
        }

        ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Espaciado
        if (ImGui::Button("Borrar Escena Completa", ImVec2(-1, 0))) { // Boton ancho
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
                ImGui::ColorEdit4("Color Difuso (kd)", glm::value_ptr(actual.subMeshes[0].color));
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
    }
};

int main() {
    Proyecto2 app;
    app.run();
    return 0;
}