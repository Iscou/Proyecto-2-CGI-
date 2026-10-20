#pragma once
#include <glm/gtc/type_ptr.hpp>
#include "AABB.h"
#include "Mesh.h"
#include "Bala.h"

class Arma {
private:
    // Propiedades del arma 
    bool semiautomatica; 
    int capacidadCargador;
    int balasRestantes;
    int balasReserva;
    Bala* bala;

    // Modelo y posicion 
    glm::vec3 posicion;
    Mesh* modelo;
    AABB hitBox;

public:

    // Constructor
    Arma(bool semiautomatica, int capacidadCargador, int balasReserva, Bala bala, glm::vec3 posicion, Mesh modelo);

    // Declaración dentro de la clase Arma:
    void recargar() {
        int balasFaltantes = capacidadCargador - balasRestantes;
        
        if (balasFaltantes <= 0 || balasReserva <= 0) return; // No hace falta o no hay reserva

        if (balasReserva >= balasFaltantes) {
            balasReserva -= balasFaltantes;
            balasRestantes = capacidadCargador;
        } else {
            balasRestantes += balasReserva;
            balasReserva = 0;
        }
    }

    Bala disparar(glm::vec3 direccionApuntado); 

    void desplazarModelo(glm::vec3 dir, float velocidad);
};