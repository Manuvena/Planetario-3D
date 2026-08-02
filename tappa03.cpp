#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <iostream>
#include "./include/hotshaders.hh"

int main() {
    sf::ContextSettings settings;
    settings.depthBits= 24;
    settings.attributeFlags= sf::ContextSettings::Core;
    settings.majorVersion= 4;
    settings.minorVersion= 1;

    sf::Window window(sf::VideoMode({800, 600}), "Planetario - Tappa 03", sf::State::Windowed, settings);
    window.setVerticalSyncEnabled(true);

    gladLoadGL(sf::Context::getFunction);

    Shaders shaders; //prende testo degli shader in GLSL e lo manda a compilatore GPU che lo compila e genera il program object salvato in VRAM
    shaders.use(); //software di esecuzione puntato sul program object creato

    //coordinate dei 3 vertici del triangolo: in OpenGl schermo va da -1.0 a 1.0 con 0.0 il centro
    float vertices[]= {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    };

    GLuint vbo, vao;
    
    //genera e attiva un vao (vertex array object) in VRAM, memorizzerà le impostazioni del vbo
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //genera vbo (vertex buffer object) in VRAM allocando memoria fisica e ci copia dentro i vertici: CPU troppo lentaa mandare dati ad ogni frame, VRAM più veloce
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //vbo è solo un blocco di memoria, viene configurato vao con le regole di lettura per la GPU:
    //manda dati alla location= 0, leggi float a gruppi di 3 (x,y,z), per trovare vertice successivo salta 12B (3 float)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); //apre comunicazione verso lo shader

    glBindBuffer(GL_ARRAY_BUFFER, 0); //sgancia vbo per evitare che dati vengano sovrascritti in modo accidentale
    glBindVertexArray(0); //sgancia vao

    bool running= true;
    while(running) {
        while(const std::optional event= window.pollEvent()) {
            if(event->is<sf::Event::Closed>()) {
                running = false;
            }  
            //gestisce resize finestra in modo da non deformare o tqaglaire disegno
            else if(const auto* resized= event->getIf<sf::Event::Resized>()) {
                glViewport(0, 0, resized->size.x, resized->size.y);
            }
        }

        glClearColor(0.05f, 0.05f, 0.15f, 1.0f); //modifichiamo registro di memoria della GPU cambiando il colore di pulizia schermo
        glClear(GL_COLOR_BUFFER_BIT);

        shaders.use(); //GPU attiva istruzioni del vertex e fragment shader memorizzate nel program object
        glBindVertexArray(vao); //riattiva il vao per usare la configurazione che abbiamo stabilito
        
        //esecuzione mandata a GPU: disegna un triangolo usando i 3 vertici nel vao attivo
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.display();
    }

    //pulizia VRAM
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}