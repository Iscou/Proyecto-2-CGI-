#pragma once
#include <glm/gtc/type_ptr.hpp>
#include "AABB.h"
#include "Mesh.h"
#include "Bala.h"
#include "../../Camera/camera.h"

class Arma {
private:
    bool semiautomatica; 
    int capacidadCargador;
    int balasRestantes;
    int balasReserva;
    Bala* bala;

    float distanciaRecorrida;
    glm::vec3 posicion;
    Mesh* modelo;
    AABB hitBox;

public:
    Arma(bool semiautomatica, int capacidadCargador, int balasReserva, Bala* bala, glm::vec3 posicion, Mesh* modelo) {
        this->semiautomatica = semiautomatica;
        this->capacidadCargador = capacidadCargador;
        this->balasRestantes = capacidadCargador;
        this->balasReserva = balasReserva;
        this->bala = bala;
        this->posicion = posicion;
        this->modelo = modelo;
    }

    void recargar() {
        int balasFaltantes = capacidadCargador - balasRestantes;
        if (balasFaltantes <= 0 || balasReserva <= 0) return; 

        if (balasReserva >= balasFaltantes) {
            balasReserva -= balasFaltantes;
            balasRestantes = capacidadCargador;
        } else {
            balasRestantes += balasReserva;
            balasReserva = 0;
        }
    }

    Bala* disparar(glm::vec3 direccionApuntado, bool municionInfinita = false) {
        if (!municionInfinita && balasRestantes == 0) return nullptr;

        Bala* balaDisparada = new Bala(*bala);
        balaDisparada->actualizarDir(direccionApuntado);
        
        // Mueve la bala a la punta del cañón 
        glm::vec3 puntoDeDisparo = this->posicion + (direccionApuntado * 0.5f);
        balaDisparada->setPosicionInicial(puntoDeDisparo);
        
        if (!municionInfinita) {
            balasRestantes--;
        }
        return balaDisparada;
    }

    void desplazarModelo(Camera cam, float velocidad) {
        float offsetFrente = 0.35f;  
        float offsetDerecha = 0.15f;  
        float offsetAbajo = -0.15f; 

        posicion = cam.position +
            (cam.front * offsetFrente) +
            (cam.right * offsetDerecha) +
            (cam.up * offsetAbajo);

        modelo->position = posicion;
        
        float yawRad = glm::radians(-cam.yaw - 90.0f);
        modelo->rotation.y = -cam.yaw + 180.0f;
        modelo->rotation.x = cam.pitch * cos(yawRad);
        modelo->rotation.z = cam.pitch * sin(yawRad);
    }

    Mesh* getModelo() { return modelo; }
    int getBalasRestantes() const { return balasRestantes; }
    int getBalasReserva() const { return balasReserva; }
};