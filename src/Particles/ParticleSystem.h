#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

struct Particle {
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec4 Color;
    float Life;
    float Size;
};

class ParticleSystem {
public:
    ParticleSystem(unsigned int maxParticles);
    ~ParticleSystem();

    void Update(float dt, glm::vec3 emitterPosition, unsigned int newParticlesCount);
    void Draw(GLuint shaderProgram, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::vec3 viewPos);

private:
    std::vector<Particle> particles;
    unsigned int maxParticles;
    unsigned int lastUsedParticle;

    unsigned int VAO, VBO;

    unsigned int FirstUnusedParticle();
    void RespawnParticle(Particle& particle, glm::vec3 emitterPosition);
    void InitBuffers();
};