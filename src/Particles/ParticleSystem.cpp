#include "ParticleSystem.h"

ParticleSystem::ParticleSystem(unsigned int maxParticles) 
    : maxParticles(maxParticles), lastUsedParticle(0) {
    particles.resize(maxParticles);
    InitBuffers();
}

void ParticleSystem::InitBuffers() {
    float quadVertices[] = {
        // Posiciones          // Normales
        -0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,

        -0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    
    glBindVertexArray(0);
}

void ParticleSystem::Update(float dt, glm::vec3 emitterPosition, unsigned int newParticlesCount) {
    for (unsigned int i = 0; i < newParticlesCount; ++i) {
        int unusedParticle = FirstUnusedParticle();
        RespawnParticle(particles[unusedParticle], emitterPosition);
    }

    for (unsigned int i = 0; i < maxParticles; ++i) {
        Particle& p = particles[i];
        if (p.Life > 0.0f) {
            p.Life -= dt;
            if (p.Life > 0.0f) {
                // Movimiento
                p.Position += p.Velocity * dt;
                
                p.Color.a = p.Life / Config.particleLife; 

                p.Size -= dt * 0.1f; 
            }
        }
    }
}

void ParticleSystem::Draw(GLuint shaderProgram, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::vec3 viewPos) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projectionMatrix[0][0]);

    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), viewPos.x, viewPos.y, viewPos.z);

    glUniform1i(glGetUniformLocation(shaderProgram, "modoIluminacion"), 1);

    glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
    glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.ambient"), 0.3f, 0.3f, 0.3f);
    glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.diffuse"), 0.5f, 0.5f, 0.5f);
    glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.specular"), 0.5f, 0.5f, 0.5f);

    glBindVertexArray(VAO);
    for (unsigned int i = 0; i < maxParticles; ++i) {
        Particle& p = particles[i];
        if (p.Life > 0.0f) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, p.Position);
            model = glm::scale(model, glm::vec3(p.Size));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), p.Color.r, p.Color.g, p.Color.b, p.Color.a);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

unsigned int ParticleSystem::FirstUnusedParticle() {
    for (unsigned int i = lastUsedParticle; i < maxParticles; ++i) {
        if (particles[i].Life <= 0.0f) {
            lastUsedParticle = i;
            return i;
        }
    }
    for (unsigned int i = 0; i < lastUsedParticle; ++i) {
        if (particles[i].Life <= 0.0f) {
            lastUsedParticle = i;
            return i;
        }
    }
    return 0;
}

void ParticleSystem::RespawnParticle(Particle& particle, glm::vec3 emitterPosition) {
    particle.Position = emitterPosition;

    float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
    float speedRange = Config.maxSpeed - Config.minSpeed;

    float vx, vy, vz;

    if (Config.direction == 0 || Config.direction == 1) {
        vx = Config.minSpeed + (static_cast<float>(rand() % 100) / 100.0f) * speedRange;
        if (Config.direction == 1) vx = -vx;
        vy = cos(angle) * Config.coneSpread;
        vz = sin(angle) * Config.coneSpread;
    } else if (Config.direction == 2 || Config.direction == 3) {
        vy = Config.minSpeed + (static_cast<float>(rand() % 100) / 100.0f) * speedRange;
        if (Config.direction == 3) vy = -vy;
        vx = cos(angle) * Config.coneSpread;
        vz = sin(angle) * Config.coneSpread;
    }
    else {
        vz = Config.minSpeed + (static_cast<float>(rand() % 100) / 100.0f) * speedRange;
        if (Config.direction == 5) vz = -vz;
        vx = cos(angle) * Config.coneSpread;
        vy = sin(angle) * Config.coneSpread;
    }

    particle.Velocity = glm::vec3(vx, vy, vz);

    float rVar = ((rand() % 100) / 100.0f) * Config.colorVariance;
    particle.Color = glm::vec4(
        glm::clamp(Config.baseColor.r + rVar, 0.0f, 1.0f),
        glm::clamp(Config.baseColor.g + rVar, 0.0f, 1.0f),
        glm::clamp(Config.baseColor.b, 0.0f, 1.0f),
        Config.baseColor.a
    );

    particle.Life = Config.particleLife;
    particle.Size = Config.startSize;
}

void ParticleSystem::setConfigParticles(float startSize,
                                        float endSize,
                                        float minSpeed,
                                        float maxSpeed,
                                        float coneSpread,
                                        glm::vec4 baseColor,
                                        float colorVariance,
                                        float particleLife,
                                        int direction) {
    Config.startSize = startSize;
    Config.endSize = endSize;
    Config.minSpeed = minSpeed;
    Config.maxSpeed = maxSpeed;
    Config.coneSpread = coneSpread;
    Config.baseColor = baseColor;
    Config.colorVariance = colorVariance;
    Config.particleLife = particleLife;
    Config.direction = direction;
}