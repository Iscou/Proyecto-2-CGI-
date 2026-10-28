#pragma once
#include "Entidad.h"

Entidad::Entidad(glm::vec3 posicion, Mesh* modelo, float vidaActual, float escudo) {
    this->posicion = posicion;
    this->vidaActual = vidaActual;
    this->escudo = escudo;
    this->modelo = modelo;
    this->vidaMax = vidaActual + escudo;
    
    if (modelo != nullptr) {
        this->hitBox = modelo->obtenerAABBGlobal();
    }
}

// Actualizar hitbox
void Entidad::actualizarHitBox() {
    if (modelo != nullptr) {
        this->hitBox = this->modelo->obtenerAABBGlobal();
    }
}

// Recibir dmg 
void Entidad::recibirDMG(float recibido) {
    if (estaMuerto) return;

    vidaActual -= recibido;
    if (vidaActual <= 0.0f) {
        morir();
    }
}

// Morir
void Entidad::morir() {
    estaMuerto = true; 

    if (modelo != nullptr) {
        modelo->borrame = true; 
    }
}