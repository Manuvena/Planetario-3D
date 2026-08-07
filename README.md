# Planetario 3D

Progetto di grafica 3D interattiva sviluppato in C++ per il corso di Computer Graphics.

L'applicazione realizza un piccolo planetario 3D/sistema solare, con corpi celesti texturizzati, orbite visibili, sfondo stellato, camera orbitante e controlli da tastiera per gestire la simulazione.

## Requisiti

Per compilare il progetto sono necessari:

- CMake
- un compilatore C++ con supporto a C++20
- OpenGL 4.1
- SFML 3.0
- glad
- GLM

SFML viene gestita tramite CMake. Le cartelle `glad`, `glm`, `include` e `Risorse` fanno parte del progetto e devono rimanere nella cartella principale.

## Struttura del progetto

Il progetto è organizzato in tappe successive:

- `Tappa01/`
- `Tappa02/`
- `Tappa03/`
- `Tappa04/`
- `Tappa05/`
- `Tappa06/`
- `Tappa07/`
- `Tappa08/`
- `Tappa09/`
- `Tappa10/`
- `Tappa11/`
- `Tappa12/`

Ogni cartella contiene il codice sorgente relativo alla rispettiva fase di sviluppo.

La cartella `Risorse/` contiene i file comuni usati dalle tappe, come texture, immagini e file `.off`.

La cartella `build/` non fa parte della consegna: viene generata automaticamente durante la compilazione.

## Build

Dalla cartella principale del progetto eseguire:

```bash
cmake -S . -B build
cmake --build build