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

public:
    // El constructor ahora recibe si es dinamico y configura su velocidad
    Bot(glm::vec3 pos, Mesh* modelo, float vida, int puntos, bool dinamico = false, float vel = 0.0f) 
        : Entidad(pos, modelo, vida, 0.0f) 
    {
        this->valorPuntos = puntos;
        this->esDinamico = dinamico;
        this->velocidad = vel;
        this->direccionMovimiento = glm::vec3(1.0f, 0.0f, 0.0f); // Moverse de lado a lado en el eje X
        
        this->distanciaRecorrida = 0.0f;
        this->limiteDistancia = 5.0f; // Se moverá 5 metros antes de devolverse
    }

    // El bot actualiza su estado en cada frame del juego
    void update(float deltaTime) {
        if (estaMuerto || modelo == nullptr) return;
        
        // Solo desplazamos y recalculamos si el objetivo es móvil
        if (esDinamico) {
            float desplazamiento = velocidad * deltaTime;
            posicion += direccionMovimiento * desplazamiento;
            distanciaRecorrida += desplazamiento;
            
            // Lógica de rebote: Si alcanza el límite, invierte su dirección
            if (distanciaRecorrida >= limiteDistancia || distanciaRecorrida <= -limiteDistancia) {
                direccionMovimiento *= -1.0f; 
            }

            modelo->position = posicion;
            actualizarHitBox(); // Mueve la caja de colisión junto con el modelo
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

    Mesh* getModelo() {return modelo;}
};