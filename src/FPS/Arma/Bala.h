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

    // Cotas de distancia
    float cortaDist;
    float mediaDist;
    float largaDist;

    // Propiedades fisicas 
    float velocidad; // Metros por segundo
    float distanciaRecorrida;
    glm::vec3 pos; // Posicion
    glm::vec3 dir; // Direccion en la que se dirige

    // Modelo y su caja de colision
    Mesh* modelo;
    AABB hitBox;

public:
    // Constructor implementado
    Bala(const Bala& otro)
        : dmg(otro.dmg), velocidad(otro.velocidad),
        multiCabeza(otro.multiCabeza), multiPierna(otro.multiPierna),
        multiLarga(otro.multiLarga), multiMedia(otro.multiMedia),
        cortaDist(otro.cortaDist), mediaDist(otro.mediaDist), largaDist(otro.largaDist),
        distanciaRecorrida(0.0f), pos(otro.pos), dir(otro.dir),
        modelo(nullptr), hitBox(otro.hitBox)
    {
        if (otro.modelo != nullptr) {
            modelo = new Mesh(*otro.modelo);
            modelo->borrame = false;
            modelo->position = otro.modelo->position;
        }
    }

    Bala(float dmg, float velocidad, Mesh* model, float multiCabeza, float multiPierna, 
         float multiLarga, float multiMedia, float cortaDist, float mediaDist, 
         float largaDist, glm::vec3 pos, glm::vec3 dir) 
    {
        this->dmg = dmg;
        this->velocidad = velocidad;
        this->modelo = model;
        this->multiCabeza = multiCabeza;
        this->multiPierna = multiPierna;
        this->multiLarga = multiLarga;
        this->multiMedia = multiMedia;
        this->cortaDist = cortaDist;
        this->mediaDist = mediaDist;
        this->largaDist = largaDist;
        this->pos = pos;
        this->dir = glm::normalize(dir); // Aseguramos que el vector dirección esté normalizado
        this->distanciaRecorrida = 0.0f;
        
        if (this->modelo != nullptr) {
            this->modelo->position = pos;
            this->hitBox = this->modelo->obtenerAABBGlobal();
        }
    }

    void setPosicionInicial(glm::vec3 nuevaPos) {
        this->pos = nuevaPos;
        this->distanciaRecorrida = 0.0f; 
        if (modelo != nullptr) {
            modelo->position = nuevaPos;
            hitBox = modelo->obtenerAABBGlobal();
        }
    }

    // Actualizar Posicion
    void actualizarPos(float deltaTime) {
        if (modelo == nullptr || modelo->borrame) return;

        // Calculamos la distancia de este frame
        float deltaDistancia = velocidad * deltaTime;

        // Actualizamos la posicion
        pos += dir * deltaDistancia;

        // Acumulamos la distancia total recorrida
        distanciaRecorrida += deltaDistancia;

        // Movemos el modelo 
        modelo->position = pos;

        // Actualizar AABB
        hitBox = modelo->obtenerAABBGlobal();

        // Eliminar si supera la distancia máxima permitida para no consumir memoria
        if (distanciaRecorrida >= largaDist) { 
            modelo->borrame = true; 
        }
    }

    // Infligir dmg al objetivo y destruirse
    void calcularDMGFinal(Entidad* objetivo, char parteCuerpo) {
        if (objetivo == nullptr || modelo == nullptr || modelo->borrame) return;

        // Evaluar multiplicador por distancia
        float multiplicadorDistancia = 1.0f; // Corta distancia (base) por defecto
        
        if (distanciaRecorrida > mediaDist) {
            multiplicadorDistancia = multiLarga;
        } else if (distanciaRecorrida > cortaDist) {
            multiplicadorDistancia = multiMedia;
        }

        // Evaluar multiplicador por zona de impacto
        float multiplicadorCuerpo = 1.0f; // Pecho 'c' (base) por defecto
        
        if (parteCuerpo == 'h') {
            multiplicadorCuerpo = multiCabeza;
        } else if (parteCuerpo == 'l') {
            multiplicadorCuerpo = multiPierna;
        }

        // Calcular daño total y aplicarlo
        float danoFinal = dmg * multiplicadorDistancia * multiplicadorCuerpo;
        objetivo->recibirDMG(danoFinal);

        // La bala se destruye al impactar contra la entidad
        modelo->borrame = true;
    }

    // Cambiar direccion
    void actualizarDir(glm::vec3 nuevaDir) {
        this->dir = glm::normalize(nuevaDir);
    }

    // Getters
    AABB getHitBox() {
        return modelo->obtenerAABBGlobal();
    }

    Mesh* getModelo() {
        return modelo;
    }
};