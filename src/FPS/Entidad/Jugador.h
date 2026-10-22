#pragma once
#include "Entidad.h"
#include "Arma.h"
#include "../../Camera/camera.h"

class Jugador : public Entidad {
private: 
    Camera cam;
    Arma* armaActiva; 
    int puntajeActual = 0;

    // Fisicas de salto y gravedad
    float velocidadY = 0.0f;
    float gravedad = -18.0f; // Un valor alto evita que el salto parezca que estas en la luna
    float fuerzaSalto = 6.0f;
    bool enSuelo = true;
    float alturaOjos = 1.0f; // La altura de la cámara respecto al suelo cero

public:
    // Constructor
    Jugador(glm::vec3 posInicial, Arma* arma) 
        : Entidad(posInicial, nullptr, 100.0f, 0.0f) // El jugador no se ve asi mismo
    {
        this->cam = Camera(posInicial);
        this->armaActiva = arma;
        this->alturaOjos = posInicial.y;
    }

    // Actualizacion principal por frame
    void update(float deltaTime) {
        // Guardamos la altura Y antes de que la camara se desplace
        float posYActual = cam.position.y; 

        if (!enSuelo) {
            velocidadY += gravedad * deltaTime;
            posYActual += velocidadY * deltaTime;

            if (posYActual <= alturaOjos) {
                posYActual = alturaOjos;
                velocidadY = 0.0f;
                enSuelo = true;
            }
        } else {
            // Clava al jugador al suelo, impidiendo que vuele o traspase el piso
            posYActual = alturaOjos; 
        }

        // Restaura la altura procesada y sincroniza la camara
        cam.position.y = posYActual; 
        this->posicion = cam.position;

        if (armaActiva != nullptr) {
            armaActiva->desplazarModelo(cam, 0.0f);
        }
    }


    // Delega el movimiento WASD a la camara
    void mover(bool w, bool s, bool a, bool d, bool shift, float deltaTime) {
        cam.processKeyboard(w, s, a, d, false, shift, deltaTime);
    }

    // Delega el movimiento del raton
    void rotarVista(float xoffset, float yoffset) {
        cam.processMouseMovement(xoffset, yoffset);
    }

    void saltar() {
        if (enSuelo) {
            velocidadY = fuerzaSalto;
            enSuelo = false;
        }
    }

    Bala* disparar() {
        if (armaActiva != nullptr) {
            Bala* proyectil = armaActiva->disparar(cam.front);
            
            if (proyectil != nullptr) {
                // Simulación de Recoil Procedural:
                // Damos un pequeño tirón a la vista hacia arriba
                cam.processMouseMovement(0.0f, 2.5f); 
            }
            return proyectil;
        }
        return nullptr;
    }

    void recargar() {
        if (armaActiva != nullptr) {
            armaActiva->recargar();
        }
    }

    // getters y setters
    void sumarPuntos(int puntos) { puntajeActual += puntos; }
    int getPuntos() const { return puntajeActual; }
    Camera* getCamara() { return &cam; } 
    Arma* getArma() { return armaActiva;}
};