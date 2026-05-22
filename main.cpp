#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>  // Sempre per primo!
#include <SFML/Window.hpp>
#include <iostream>

int main() {
    // 1. Configurazione del contesto OpenGL 4.1 Core
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Core;

    // CORREZIONE SFML 3: Usiamo sf::State::Windowed al posto di sf::Style::Default
    sf::Window window(sf::VideoMode({800, 600}), "Planetario 3D - Tappa 02", sf::State::Windowed, settings);
    window.setFramerateLimit(60);

    // CORREZIONE GLAD 2: Usiamo gladLoadGL e GLADloadfunc
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(sf::Context::getFunction))) {
        std::cerr << "Errore: Impossibile inizializzare GLAD!" << std::endl;
        return -1;
    }

    std::cout << "OpenGL Versione caricata con successo: " << glGetString(GL_VERSION) << std::endl;

    // Colore di pulizia dello schermo (Blu notte)
    glClearColor(0.05f, 0.05f, 0.15f, 1.0f);

    // 3. Event Loop Principale
    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }

        // 4. Rendering nativo OpenGL
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Scambio dei buffer
        window.display();
    }

    return 0;
}