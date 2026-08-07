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

SFML viene scaricata e configurata tramite CMake. Le cartelle `glad`, `glm`, `include` e `Risorse` fanno parte del progetto e devono rimanere nella cartella principale.

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
```

Questi comandi compilano tutte le tappe del progetto con un'unica build.

Gli eseguibili vengono generati nella cartella:

```bash
build/bin
```

Durante la compilazione, CMake copia automaticamente la cartella `Risorse/` dentro `build/Risorse`, in modo che gli eseguibili possano caricare correttamente immagini, texture e file `.off`.

## Esecuzione

Dopo aver compilato il progetto, entrare nella cartella degli eseguibili.

Dalla cartella principale del progetto:

```bash
cd build/bin
```

Per lanciare la tappa finale su Linux:

```bash
./tappa12
```

Per lanciare la tappa finale su Windows:

```bash
tappa12.exe
```
Le altre tappe possono essere lanciate nello stesso modo.

Gli eseguibili devono essere lanciati dalla cartella `build/bin`, perché i percorsi delle risorse nel codice sono relativi a questa posizione.

In particolare, le risorse vengono copiate da CMake in `build/Risorse` e vengono caricate dagli eseguibili tramite percorsi relativi del tipo:

```text
../Risorse/nomefile
```

## Comandi dell'applicazione

Nella tappa finale sono disponibili i seguenti comandi:

- Mouse sinistro premuto + movimento: ruota la camera attorno alla scena.
- Barra spaziatrice: mette in pausa o riprende la simulazione.
- Freccia su: aumenta la velocità della simulazione.
- Freccia giù: diminuisce la velocità della simulazione.
- Chiusura della finestra: termina l'applicazione.