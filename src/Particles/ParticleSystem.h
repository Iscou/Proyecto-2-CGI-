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

struct ParticleConfig {
    float startSize = 0.2f;
    float endSize = 0.1f;
    float minSpeed = 2.0f;
    float maxSpeed = 4.0f;
    float coneSpread = 1.5f;
    glm::vec4 baseColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    float colorVariance = 0.5f;
    float particleLife = 1.0f;
    int direction = 2;
};

class ParticleSystem {
public:
    ParticleSystem(unsigned int maxParticles);
    ~ParticleSystem();

    void Update(float dt, glm::vec3 emitterPosition, unsigned int newParticlesCount);
    void Draw(GLuint shaderProgram, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::vec3 viewPos);
    void setConfigParticles(float startSize,
        float endSize,
        float minSpeed,
        float maxSpeed,
        float coneSpread,
        glm::vec4 baseColor,
        float colorVariance = 0.5f,
        float particleLife = 1.0f,
        int direction = 2);

    ParticleConfig Config;

private:
    std::vector<Particle> particles;
    unsigned int maxParticles;
    unsigned int lastUsedParticle;

    unsigned int VAO, VBO;

    unsigned int FirstUnusedParticle();
    void RespawnParticle(Particle& particle, glm::vec3 emitterPosition);
    void InitBuffers();
};