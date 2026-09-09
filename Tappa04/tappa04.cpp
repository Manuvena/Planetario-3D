#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

#include "./include/matrices.hh"
#include "./include/mesh.hh"
#include "./include/hotshaders.hh"

//classe per gestione finestra SFML e contesto OpenGL
class Setup
{
public:
    sf::Window* window; //puntatore a finestra SFML
    Setup()
    {
        sf::ContextSettings settings; //crea struttura di impostazioni per il contesto OpenGL
        settings.depthBits= 24; //depth buffer per profondità
        settings.antiAliasingLevel= 4;
        settings.attributeFlags= sf::ContextSettings::Core; //OpenGL moderno
        settings.majorVersion= 4;
        settings.minorVersion= 1;
        window= new sf::Window(sf::VideoMode({800, 800}), "Planetario Tappa 04", sf::State::Windowed, settings);
        window->setVerticalSyncEnabled(true); //attiva VSync per sincronizzare framerate con quello del monitor
        gladLoadGL(sf::Context::getFunction); //carica le funzioni OpenGL tramite GLAD
    }
};

//classe per gestione della camera
class Camera
{
public:
    glm::mat4 v, vp; //view matrix e view projection
    //parametri della camera
    float aspect= 1.0f; 
    float phi= 30.0f; 
    float theta= 30.0f; 
    float fd= 4.0f; 
    float od= 4.0f; 
    void drag(float dx, float dy) //movimento drag del mouse
    {
        phi += dx*0.5f; //modifica angolo orizzontale della camera * fattore di sensibilità
        theta= std::clamp(theta+dy*0.5f, -89.0f, 89.0f); //modifica angolo verticale ma lo limita per evitare si ribalti
        view_projection(); //ricalcola le matrici della camera
    }
    void view_projection() //ricalcola vista e proiezione
    {
        glm::mat4 ry= rotation_y(phi); //crea rotazione attorno ad asse x
        glm::mat4 rx= rotation_x(theta); //crea rotazione attorno ad asse y
        glm::mat4 tz= translation(0, 0, -od); //traslazione lungo z, allontano di od

        //piani di clipping near e far
        float ncp= std::max(0.1f, od-1.0f); 
        float fcp= od+1.0f; 
        //coefficienti della matrice di proiezione prospettica
        float a= (fcp+ncp)/(ncp-fcp);
        float b= 2.0f*fcp*ncp/(ncp-fcp);
        //costruisco matrice di proiezione prospettica
        glm::mat4 pr= glm::mat4(fd/aspect, 0, 0, 0, 0, fd, 0, 0, 0, 0, a, -1.0, 0, 0, b, 0);
        v= tz*rx*ry; //costruisco view matrix combinando distanza e rotazioni
        vp= pr*v; //costruisco view projection
    }
};

//classe per passare da mesh OFF a GPU
class GPUMesh 
{
public:
    glm::vec3 center; //descrive posizione della mesh
    float extent; //descrive dimensione della mesh
    GLuint vao, vbo, ebo; //identificatori OpenGL ebo è element buffer object, memorizza indici delle facce
    size_t index_count; //conserva numer idi indici che dovranno essere letti

    GPUMesh(std::string f) //f è il percorso del file OFF
    {
        Mesh m(f); //legge file OFF e ricostruisce vertici, facce e normali
        center= m.center;
        extent= m.extent;

        std::vector<float> p; //conterrà vertici
        std::vector<unsigned int> i; //conterrà indici dei vertici
        m.pack4gpu(p, i); //prendo dati del file OFF e organizzo nei due vettori
        index_count= i.size();

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, p.size() * sizeof(float), p.data(), GL_STATIC_DRAW);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
        glEnableVertexAttribArray(0);
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, i.size() * sizeof(unsigned int), i.data(), GL_STATIC_DRAW);
    }
    void draw()
    {
        glBindVertexArray(vao);
        //legge indici dell'ebo: li raggruppa tre alla volta, uqanti indici deve leggere, tipo, 0 perchè parte dal primo byte
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    }
};

//organizzazione della scena
class Scene
{
public:
    Camera cam;
    GPUMesh mesh; //una sola mesh cubica
    glm::mat4 mm;
    sf::Window* win;
    GLint m_loc, vp_loc; //per conservare posizioni delle uniform nello shader
    Scene(std::string f, Shaders& s, sf::Window& w) : mesh(f), win(&w) //lista di inizializzazione, costruisco subito gli oggetti
    {
        //recupera le posizioni delle uniform model e vp nel programma shader
        m_loc= glGetUniformLocation(s.program, "model");
        vp_loc= glGetUniformLocation(s.program, "vp");
        //matrice per centrare e normalizzare la mesh del cubo
        mm= scaling(1.0f/mesh.extent)*translation(-mesh.center);
    }
    void draw(float time)
    {
        cam.aspect= (float)win->getSize().x / (float)win->getSize().y; //calcola aspect ratio (larghezza/altezza)
        cam.view_projection();
        //centra la mesh nell'origine e a dimensioni standard, poi la fa ruotare
        glm::mat4 mm_final= rotation_y(time*40.0f)*rotation_x(time*20.0f)*mm;
        //invia allo shader la matrice dell'oggetto e lo disegna
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_final[0][0]);
        glUniformMatrix4fv(vp_loc, 1, GL_FALSE, &cam.vp[0][0]); //invia vp a shader, matrice, non farla trasposta e da indirizzo primo valore
        mesh.draw();
    }
};

//mainprogramma
int main(int argc, char** argv)
{
    Setup s;
    Shaders sh; //crea oggetto che gestisce compilazione degli shader e OpenGL
    const char* vs= "#version 410 core\n layout(location=0) in vec3 pos; uniform mat4 model; uniform mat4 vp; void main() { gl_Position = vp * model * vec4(pos, 1.0); }";
    const char* fs= "#version 410 core\n out vec4 c; void main() { c = vec4(0.2, 0.7, 0.5, 1.0); }";
    sh.compile_attach_link(&vs, &fs); //creo ogetti vs e compilo, crea fs e compila, li collega ad un programma e fa linking
    sh.use(); //rende attivo il programma

    Scene sc("../Risorse/cubo.off", sh, *s.window); //costruttore scena

    glEnable(GL_DEPTH_TEST); //attivo depth test
    sf::Clock clk; //crea un cronometro
    while(s.window->isOpen()) //ciclo ripetuto finchè la finestra è aperta
    {
        while(const std::optional e= s.window->pollEvent()) //se c'è evento pollEvent lo restituisce, altrimenti termina il ciclo
        {
            if(e->is<sf::Event::Closed>()) //se l'evento è di tipo closed
                return 0;
            if(const auto* r= e->getIf<sf::Event::Resized>()) //se è evento di ridimensionamento finestra
                glViewport(0, 0, r->size.x, r->size.y);
            if(const auto* m= e->getIf<sf::Event::MouseMoved>()) //se è evento di movimento del mouse
            {
                static float px= m->position.x;
                static float py= m->position.y; //conservano posizione precendente
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) //se mentre si muove il mouse il tasto sinistro è premuto
                    sc.cam.drag(m->position.x-px, m->position.y-py); //passa a drag differenza tra pos attuale e precedente
                //aggiormo posizione precedente anche quando il tasto non è premuto, così se effettuo nuovo trascinamento non c'è salto
                px= m->position.x;
                py= m->position.y;
            } 
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //elimina colori fotogramma precedente e profondità
        sc.draw(clk.getElapsedTime().asSeconds());
        s.window->display(); //inverto back buffer con front buffer
    }
}
