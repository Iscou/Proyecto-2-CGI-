#include "engine3D.h"

Engine3D::Engine3D(int width, int height, const std::string& title) 
    : width(width), height(height), title(title) {
    for (int i = 0; i < GLFW_KEY_LAST; ++i) keyState[i] = false;
    for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; ++i) mouseButtonState[i] = false;
    init();
}

Engine3D::~Engine3D() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Engine3D::init() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW\n";
        return;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        std::cout << "Failed to create GLFW window\n";
        return;
    }
    glfwMakeContextCurrent(window);

    // Cargador compatible con tu archivo glad.c actual
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }

    glViewport(0, 0, width, height);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Configuracion de ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void Engine3D::run() {
    setup();
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // Limpiamos pantalla (Color y Profundidad 3D)
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Habilitar la mezcla de colores
        glEnable(GL_BLEND);
        // Establecer la función de mezcla estándar para transparencias Alpha (SrcAlpha, 1 - SrcAlpha)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Renderizado de la escena 3D
        update(deltaTime);

        // Renderizado de la interfaz ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        drawUI();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

glm::vec2 Engine3D::getMousePosition() {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
}

bool Engine3D::isKeyPressed(int key) const {
    if (key < 0 || key >= GLFW_KEY_LAST) return false;
    return keyState[key];
}

bool Engine3D::isMouseButtonPressed(int button) const {
    if (button < 0 || button >= GLFW_MOUSE_BUTTON_LAST) return false;
    return mouseButtonState[button];
}

void Engine3D::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key < 0 || key >= GLFW_KEY_LAST) return;
    Engine3D* engine = static_cast<Engine3D*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        engine->keyState[key] = true;
        engine->onkeyDown(key);
    } else if (action == GLFW_RELEASE) {
        engine->keyState[key] = false;
        engine->onkeyUp(key);
    }
}

void Engine3D::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button < 0 || button >= GLFW_MOUSE_BUTTON_LAST) return;
    Engine3D* engine = static_cast<Engine3D*>(glfwGetWindowUserPointer(window));
    glm::vec2 mousePos = engine->getMousePosition();
    if (action == GLFW_PRESS) {
        engine->mouseButtonState[button] = true;
        engine->onMouseButtonDown(button, mousePos.x, mousePos.y);
    } else if (action == GLFW_RELEASE) {
        engine->mouseButtonState[button] = false;
        engine->onMouseButtonUp(button, mousePos.x, mousePos.y);
    }
}

void Engine3D::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Engine3D* engine = static_cast<Engine3D*>(glfwGetWindowUserPointer(window));
    if (engine) {
        engine->onMouseMove(xpos, ypos);
    }
}

void Engine3D::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return;
    Engine3D* engine = static_cast<Engine3D*>(glfwGetWindowUserPointer(window));
    if (engine) {
        engine->width = width;
        engine->height = height;
        glViewport(0, 0, width, height);
        engine->onResize(width, height);
    }
}