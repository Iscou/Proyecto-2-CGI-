#pragma once
#include "Entidad.h"

class Bot : public Entidad {
private:
    int valorPuntos;
    
    // Variables para movimiento dinámico
    bool esDinamico;
    float velocidad;
    glm::vec3 direccionMovimiento;
    
    // Cotas para el "Strafing" (ir y venir)
    float distanciaRecorrida;
    float limiteDistancia; 

    // Control de tiempo de vida (esferas)
    bool tieneTiempoVida;
    float tiempoVida;

    // Física parabólica para Skeet Shooting
    bool esParabolico = false;
    glm::vec3 velParabolica = glm::vec3(0.0f);
    float gravedadBot = -9.8f;

public:
    Bot(glm::vec3 pos, Mesh* modelo, float vida, int puntos, bool dinamico = false, float vel = 0.0f, bool conTiempo = false, float tVida = 3.0f) 
        : Entidad(pos, modelo, vida, 0.0f) 
    {
        this->valorPuntos = puntos;
        this->esDinamico = dinamico;
        this->velocidad = vel;
        this->direccionMovimiento = glm::vec3(1.0f, 0.0f, 0.0f); 
        
        this->distanciaRecorrida = 0.0f;
        this->limiteDistancia = 5.0f; 

        this->tieneTiempoVida = conTiempo;
        this->tiempoVida = tVida;
    }

    void activarModoSkeet(glm::vec3 velocidadInicial) {
        this->esParabolico = true;
        this->velParabolica = velocidadInicial;
    }

    void update(float deltaTime) {
        if (estaMuerto || modelo == nullptr) return;
        
        if (tieneTiempoVida) {
            tiempoVida -= deltaTime;
            if (tiempoVida <= 0.0f) {
                valorPuntos = 0; 
                morir();
                return;
            }
        }

        if (esParabolico) {
            velParabolica.y += gravedadBot * deltaTime;
            posicion += velParabolica * deltaTime;
            modelo->position = posicion;
            actualizarHitBox();

            // Si cae de vuelta bajo el suelo sin ser destruido, desaparece
            if (posicion.y < -3.0f && velParabolica.y < 0.0f) {
                valorPuntos = 0;
                morir();
            }
        } else if (esDinamico) {
            float desplazamiento = velocidad * deltaTime;
            posicion += direccionMovimiento * desplazamiento;
            distanciaRecorrida += desplazamiento;
            
            if (distanciaRecorrida >= limiteDistancia || distanciaRecorrida <= -limiteDistancia) {
                direccionMovimiento *= -1.0f; 
            }

            modelo->position = posicion;
            actualizarHitBox(); 
        }
    }

    void morir() override {
        Entidad::morir(); 
    }

    int getPuntos() {
        if(estaMuerto){
            return valorPuntos;
        }
        return 0;
    }

    Mesh* getModelo() { return modelo; }
};