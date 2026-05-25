#define GLAD_GL_IMPLEMENTATION //main genera il codice di GLAD
#include <glad/gl.h>
#include <SFML/Window.hpp>
#include <iostream>

int main() {
    sf::ContextSettings settings; //oggetto che viene mandato a GPU che serve a definire le proprietà HW da attivare
    settings.depthBits= 24; //z-buffer, 24b per ogni px per calcolare la distanza degli oggetti da telecamera
    settings.stencilBits= 8;
    settings.antiAliasingLevel= 4; //tecnica MSAA per evitare aliasing
    //per impostare versione principale e secondaria di OpenGL
    settings.majorVersion= 4;
    settings.minorVersion= 1;
    //impostiamo flag del contesto sul profilo Core invece che sul vecchio Compatibility, bisogna definire gli shader
    settings.attributeFlags= sf::ContextSettings::Core;

    //istanzia oggetto window e chiede a S.O. di aprire finestra 800x600 px, con barra titolo e bordi, passando struttura settings
    sf::Window window(sf::VideoMode({800, 600}), "Planetario 3D - Tappa 02", sf::State::Windowed, settings);
    window.setVerticalSyncEnabled(true); //abilita il V-Sync per far si hce programma segua il refresh rate del monitor

    //getFunction serve a chiedere a S.O. indirizzo di memoria driver GPU, GLAD li prende e collega al codice
    if(!gladLoadGL(reinterpret_cast<GLADloadfunc>(sf::Context::getFunction)))
        std::cerr << "errore: impossibile inizializzare GLAD" << std::endl;

    glClearColor(0.05f, 0.05f, 0.15f, 1.0f); //modifichiamo registro di memoria della GPU cambiando il colore di pulizia schermo
    glEnable(GL_DEPTH_TEST); //abilitiamo lo z-buffer

    while(window.isOpen()) {
        while(const std::optional<sf::Event> event= window.pollEvent()) {
            if(event->is<sf::Event::Closed>())
                window.close();
        }

        //CPU ordina a GPU di pulire il back buffer e azzerare lo z-buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        window.display();
    }

    return 0;
}