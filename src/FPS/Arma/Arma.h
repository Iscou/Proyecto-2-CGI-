#pragma once
#include <glm/gtc/type_ptr.hpp>
#include "AABB.h"
#include "Mesh.h"
#include "Bala.h"
#include "../../Camera/camera.h"

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

    Bala* disparar(glm::vec3 direccionApuntado) {

        if (balasRestantes == 0) {
            return NULL;
        }

        Bala* balaDisparada = new Bala(*bala);
        balaDisparada->actualizarDir(direccionApuntado);
        balasRestantes--;

        return balaDisparada;
    }

    void desplazarModelo(Camera cam, float velocidad) {
        float offsetFrente = 0.5f;  // Qué tan adelante está la pistola respecto a los ojos
        float offsetDerecha = 0.3f;  // Qué tan a la derecha está
        float offsetAbajo = -0.1f; // Qué tan abajo está

        posicion = cam.position +
            (cam.front * offsetFrente) +
            (cam.right * offsetDerecha) +
            (cam.up * offserAbajo);
    }
};