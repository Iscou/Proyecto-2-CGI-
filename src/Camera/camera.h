#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float speed;
    float sensitivity;

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 1.0f, 5.0f)) 
        : position(startPos), worldUp(glm::vec3(0.0f, 1.0f, 0.0f)), 
          yaw(-90.0f), pitch(-10.0f), speed(4.0f), sensitivity(0.15f) {
        updateCameraVectors();
    }

    // Devuelve la matriz View para el Vertex Shader
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    // Devuelve la matriz Projection para el Vertex Shader
    glm::mat4 getProjectionMatrix(float aspectRatio) const {
        return glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    }

    // Movimiento con teclado (WASD)
    void processKeyboard(bool w, bool s, bool a, bool d, bool upKey, bool downKey, float deltaTime) {
        float velocity = speed * deltaTime;
        if (w) position += front * velocity;
        if (s) position -= front * velocity;
        if (a) position -= right * velocity;
        if (d) position += right * velocity;
        if (upKey) position += worldUp * velocity;
        if (downKey) position -= worldUp * velocity;
    }

    // Rotacion con el raton
    void processMouseMovement(float xoffset, float yoffset) {
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        // Evitar que la camara se invierta (Gimbal lock en pitch)
        pitch = std::clamp(pitch, -89.0f, 89.0f);
        updateCameraVectors();
    }

private:
    void updateCameraVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
};