#pragma once
#include <glm/gtc/type_ptr.hpp>
#include "AABB.h"
#include "Mesh.h"

class Entidad {
protected: 
    float vidaMax;
    float vidaActual;
    float escudo;
    glm::vec3 posicion;
    Mesh* modelo; 
    AABB hitBox;
    bool estaMuerto = false;

public:
    // Constructor 
    Entidad(glm::vec3 posicion, Mesh* modelo, float vidaActual = 100.0f, float escudo = 0.0f);
    virtual ~Entidad() = default; 

    // Actualizar hitbox
    void actualizarHitBox();

    // Recibir dmg 
    void recibirDMG(float recibido);

    // Morir 
    virtual void morir();
    
    // Getters 
    bool getEstaMuerto() const { return estaMuerto; }
    AABB getHitBox() const { return hitBox; }
};