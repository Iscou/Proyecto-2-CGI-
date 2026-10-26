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
    float gravedad = -18.0f;
    float fuerzaSalto = 6.0f;
    bool enSuelo = true;
    float alturaOjos = 1.0f;

    // Control de recuperacion de Recoil
    float recoilAcumulado = 0.0f;

public:
    Jugador(glm::vec3 posInicial, Arma* arma) 
        : Entidad(posInicial, nullptr, 100.0f, 0.0f) 
    {
        this->cam = Camera(posInicial);
        this->armaActiva = arma;
        this->alturaOjos = posInicial.y;
    }

    void update(float deltaTime) {
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
            posYActual = alturaOjos; 
        }

        cam.position.y = posYActual; 
        this->posicion = cam.position;

        // Recuperacion suave del Recoil usando el metodo publico de Camera
        if (recoilAcumulado > 0.0f) {
            float recup = 12.0f * deltaTime; // Velocidad de bajada
            if (recup > recoilAcumulado) recup = recoilAcumulado;
            
            // Dividimos entre sensitivity para rotar los grados exactos y actualizar vectores
            cam.processMouseMovement(0.0f, -recup / cam.sensitivity);
            recoilAcumulado -= recup;
        }

        if (armaActiva != nullptr) {
            armaActiva->desplazarModelo(cam, 0.0f);
        }
    }

    void mover(bool w, bool s, bool a, bool d, bool shift, float deltaTime) {
        cam.processKeyboard(w, s, a, d, false, shift, deltaTime);
    }

    void rotarVista(float xoffset, float yoffset) {
        cam.processMouseMovement(xoffset, yoffset);
    }

    void saltar() {
        if (enSuelo) {
            velocidadY = fuerzaSalto;
            enSuelo = false;
        }
    }

    Bala* disparar(bool municionInfinita = false) {
        if (armaActiva != nullptr) {
            Bala* proyectil = armaActiva->disparar(cam.front, municionInfinita);
            
            if (proyectil != nullptr) {
                // Aplicamos culatazo de 2.5f (que multiplica por 0.15f de sensitivity = 0.375 grados)
                cam.processMouseMovement(0.0f, 2.5f); 
                recoilAcumulado += 2.5f * cam.sensitivity;
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

    void sumarPuntos(int puntos) { puntajeActual += puntos; }
    int getPuntos() const { return puntajeActual; }
    Camera* getCamara() { return &cam; } 
    Arma* getArma() { return armaActiva; }
};