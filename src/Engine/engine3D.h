#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <vector>
#include <string>
#include <iostream>

class Engine3D {
private: 
    GLFWwindow* window;
    std::string title;
    
    void init();
    
    bool keyState[GLFW_KEY_LAST];
    bool mouseButtonState[GLFW_MOUSE_BUTTON_LAST];
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

protected:
    int width, height;
    glm::vec3 clearColor = glm::vec3(0.1f, 0.1f, 0.15f);

    glm::vec2 getMousePosition();
    bool isKeyPressed(int key) const;
    bool isMouseButtonPressed(int button) const;
    GLFWwindow* getWindow() const { return window; }

public:
    Engine3D(int width, int height, const std::string& title);
    virtual ~Engine3D();
    void run();

    virtual void onkeyDown(int key) {}
    virtual void onkeyUp(int key) {}
    virtual void onMouseButtonDown(int button, double x, double y) {}
    virtual void onMouseButtonUp(int button, double x, double y) {}
    virtual void onMouseMove(double x, double y) {}
    virtual void setup() {}
    virtual void update(float deltaTime) {}
    virtual void drawUI() {}
    virtual void onResize(int w, int h) {} 
};