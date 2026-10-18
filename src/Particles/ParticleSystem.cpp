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
    
    // Atributo 0: Posición (aPos)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    
    // Atributo 1: Normal (aNormal)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    
    glBindVertexArray(0);
}

void ParticleSystem::Update(float dt, glm::vec3 emitterPosition, unsigned int newParticlesCount) {
    // Spawn de nuevas partículas con la cantidad pasada por parámetro
    for (unsigned int i = 0; i < newParticlesCount; ++i) {
        int unusedParticle = FirstUnusedParticle();
        RespawnParticle(particles[unusedParticle], emitterPosition);
    }

    // Actualizar físicas y propiedades en tiempo real
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
    // Habilitar blending para transparencias
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Activar el programa de shader directamente con su ID
    glUseProgram(shaderProgram);

    // Enviar matrices de cámara usando OpenGL nativo
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projectionMatrix[0][0]);

    // Posición de la cámara
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), viewPos.x, viewPos.y, viewPos.z);

    // Modo de iluminación (1 para Lambert)
    glUniform1i(glGetUniformLocation(shaderProgram, "modoIluminacion"), 1);

    // Configurar luz direccional básica
    glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
    glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.ambient"), 0.3f, 0.3f, 0.3f);
    glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.diffuse"), 0.5f, 0.5f, 0.5f);
    glUniform3f(glGetUniformLocation(shaderProgram, "dirLight.specular"), 0.5f, 0.5f, 0.5f);

    glBindVertexArray(VAO);
    for (unsigned int i = 0; i < maxParticles; ++i) {
        Particle& p = particles[i];
        if (p.Life > 0.0f) {
            // Construir la matriz Model para la partícula actual
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, p.Position);
            model = glm::scale(model, glm::vec3(p.Size));

            // Enviar la matriz Model y el color del objeto
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), p.Color.r, p.Color.g, p.Color.b, p.Color.a);

            // Dibujar el quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

unsigned int ParticleSystem::FirstUnusedParticle() {
    // Buscar desde el último usado para optimizar
    for (unsigned int i = lastUsedParticle; i < maxParticles; ++i) {
        if (particles[i].Life <= 0.0f) {
            lastUsedParticle = i;
            return i;
        }
    }
    // Si no encuentra, busca desde el principio
    for (unsigned int i = 0; i < lastUsedParticle; ++i) {
        if (particles[i].Life <= 0.0f) {
            lastUsedParticle = i;
            return i;
        }
    }
    return 0; // Si están todas llenas, sobreescribe la primera
}

void ParticleSystem::RespawnParticle(Particle& particle, glm::vec3 emitterPosition) {
    particle.Position = emitterPosition;

    // Usar el cono que hicimos antes pero basado en la configuración
    float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
    float vx = cos(angle) * Config.coneSpread;
    float vz = sin(angle) * Config.coneSpread;

    float speedRange = Config.maxSpeed - Config.minSpeed;
    float vy = Config.minSpeed + (static_cast<float>(rand() % 100) / 100.0f) * speedRange;

    particle.Velocity = glm::vec3(vx, vy, vz);

    // Variación de color dinámica
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