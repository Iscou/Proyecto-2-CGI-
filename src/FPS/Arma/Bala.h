#pragma once
#include <glm/gtc/type_ptr.hpp>
#include "AABB.h"
#include "Mesh.h"
#include "Entidad.h"

class Bala {
private:
    // Variables para calculo de dmg
    float dmg; // Corta distancia pecho es el dmg base

    // Multiplicadores de partes del cuerpo
    float multiCabeza;
    float multiPierna;
    // Multiplicadores de distancia 
    float multiLarga;
    float multiMedia;

    // Cotas
    float cortaDist;
    float mediaDist;
    float largaDist;

    // Propiedades fisicas 
    float velocidad; // Metros por segundo
    float distanciaRecorrida = 0;
    glm::vec3 pos; // Posicion
    glm::vec3 dir; // Direccion en la que se dirige

    // Modelo y su caja de colision
    Mesh* modelo;
    AABB hitBox;

public:
    // Constructor
    Bala(float dmg, float velocidad, Mesh model, float multiCabeza, float multiPierna, float multiLarga, float multiMedia, float cortaDist, float mediaDist, float largaDist, glm::vec3 pos, glm::vec3 dir );

    // Actualizar Posicion
    void actualizarPos(float deltaTime) {
        // Calculamos la distancia 
        float deltaDistancia = velocidad * deltaTime;

        // Actualizamos la posicion
        pos += dir * deltaDistancia;

        // Acumulamos la distancia total recorrida
        distanciaRecorrida += deltaDistancia;

        // Actualizar AABB
    }

    // Verificar Colision 
    void verificarColision();

    // Inflingir dmg
    void calcularDMGFinal();

    // Cambiar direccion
    void actualizarDir(glm::vec3 dir) {
        this->dir = dir;
    }

};