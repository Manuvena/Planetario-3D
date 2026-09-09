#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cmath>

#include "./include/matrices.hh"
#include "./include/mesh.hh"
#include "./include/hotshaders.hh"

//funzione per creazione file .OFF della sfera per la mesh
void generaSferaOFF() {
    std::ofstream out("sfera.off");
    int parallels=40, meridians=40; //costruisco sfera dividendola in paralleli e meridiani
    int num_vertices=parallels*meridians+2; //identifica il numero di vertici della sfera + 2 dei poli
    int num_faces=parallels*meridians*2; //ogni faccia della sfera è divisa in triangoli, siccome genero dei quadrilateri corrispondono a 2 trinagoli
    
    out<<"OFF\n"<<num_vertices<<" "<<num_faces<<" 0\n";
    out<<"0.0 -1.0 0.0\n"; //coordinate polo inferiore

    for(int i=1; i<=parallels; i++) { //scorro paralleli intermedi
        float theta=3.14159265359f*i/(parallels+1.0f); //angolo verticale, a quale altezza della sfera si trova il parallelo corrente
        for(int j=0; j<meridians; j++) { //scorro i punti attorno al parallelo
            float phi=2.0f*3.14159265359f*j/meridians; //angolo orizzontale attorno a tutta circonferenza
            //trasformo theta e phi in coordinate 3D (x,y,z)
            out<<(std::sin(theta)*std::cos(phi))<<" "<<-std::cos(theta)<<" "<<(std::sin(theta)*std::sin(phi))<<"\n";
        }
    }
    
    out<<"0.0 1.0 0.0\n"; //coordinate polo superiore, ultimo vertice

    //genero facce del polo inferiore
    for(int j=0; j<meridians; j++) { //scorro primo anello
        out<<"3 0 "<<(1+(j+1)%meridians)<<" "<<(1+j)<<"\n"; //modulo per chiudere il giro
    }
        
    //facce centrali
    for(int i=0; i<parallels-1; i++) { //scorre la fascia tra un parallelo e il successivo
        for(int j=0; j<meridians; j++) { //dentro la fascia scorre una cella alla volta
            int next_j=(j+1)%meridians; //trovo meridiano successivo, modulo perchè l'ultimo sarà il primo
            int v0=1+i*meridians+j, v1=1+i*meridians+next_j; //calcola vertici superiori della cella
            int v2=1+(i+1)*meridians+next_j, v3=1+(i+1)*meridians+j; //calcola vertici inferiori della cella
            out<<"3 "<<v0<<" "<<v1<<" "<<v3<<"\n"; //primo triangolo
            out<<"3 "<<v1<<" "<<v2<<" "<<v3<<"\n"; //secondo triangolo
        }
    }
    
    int nord=num_vertices-1, start=1+(parallels-1)*meridians; //indice polo sup. e indice del pirmo vertice dell'ultimo parallelo
    //genero facce polo superiore
    for(int j=0; j<meridians; j++) { //scorro l'ultimo anello
        out<<"3 "<<nord<<" "<<(start+j)<<" "<<(start+(j+1)%meridians)<<"\n";
    }
        
    out.close();
}

//classe per gestione finestra SFML e contesto OpenGL
class Setup {
public:
    sf::Window* window; //puntatore a finestra SFML
    Setup() {
        sf::ContextSettings settings; //crea struttura di impostazioni per il contesto OpenGL
        settings.depthBits=24; //depth buffer per profondità
        settings.stencilBits=8;
        settings.antiAliasingLevel=4;
        settings.attributeFlags=sf::ContextSettings::Attribute::Core; //OpenGL moderno
        settings.majorVersion=4;
        settings.minorVersion=1;
        window=new sf::Window(sf::VideoMode({800, 800}), "Planetario Tappa 07", sf::Style::Default, sf::State::Windowed, settings);
        window->setVerticalSyncEnabled(true); //attiva VSync per sincronizzare framerate con quello del monitor
        if(!window->setActive(true)) { //attiva ocntesto OpenGL della finestra
            exit(1);
        }
        gladLoadGL(sf::Context::getFunction); //carica le funzioni OpenGL tramite GLAD
    }
    ~Setup() { //distruttore
        delete window;
    }
};

//classe per gestione della camera
class Camera {
public:
    glm::mat4 v, vp; //view matrix e view projection
    //parametri della camera
    float aspect=1.0f;
    float phi=30.0f, theta=30.0f, fd=2.0f, od=15.0f; 
    
    void drag(float dx, float dy) { //movimento drag del mouse
        phi+=dx*0.5f; //modifica angolo orizzontale della camera * fattore di sensibilità
        theta=std::clamp(theta+dy*0.5f, -89.0f, 89.0f); //modifica angolo verticale ma lo limita per evitare si ribalti
        view_projection(); //ricalcola le matrici della camera
    }
    
    void view_projection() { //ricalcola vista e proiezione
        glm::mat4 ry=rotation_y(phi); //crea rotazione attorno ad asse x
        glm::mat4 rx=rotation_x(theta); //crea rotazione attorno ad asse y
        glm::mat4 tz=translation(0, 0, -od); //traslazione lungo z, allontano di od
        //piani di clipping near e far
        float ncp=std::max(0.1f, od-10.0f);
        float fcp=od+10.0f;
        //coefficienti della matrice di proiezione prospettica
        float a=(fcp+ncp)/(ncp-fcp);
        float b=2.0f*fcp*ncp/(ncp-fcp);
        //costruisco matrice di proiezione prospettica
        glm::mat4 pr=glm::mat4(fd/aspect, 0, 0, 0, 0, fd, 0, 0, 0, 0, a, -1.0, 0, 0, b, 0);
        v=tz*rx*ry; //costruisco view matrix combinando distanza e rotazioni
        vp=pr*v; //costruisco view projection
    }
};

//classe per passare da mesh OFF a GPU
class GPUMesh {
public:
    glm::vec3 center; //descrive posizione della mesh
    float extent; //descrive dimensione della mesh
    GLuint vao, vbo, ebo; //identificatori OpenGL ebo è element buffer object, memorizza indici delle facce
    size_t index_count; //conserva numer idi indici che dovranno essere letti
    
    GPUMesh(std::string f) { //f è il percorso del file OFF
        Mesh m(f); //legge file OFF e ricostruisce vertici, facce e normali
        center=m.center;
        extent=m.extent;
        std::vector<float> p; //conterrà vertici
        std::vector<unsigned int> i; //conterrà indici dei vertici
        m.pack4gpu(p, i); //prendo dati del file OFF e organizzo nei due vettori
        index_count=i.size();
        
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, p.size()*sizeof(float), p.data(), GL_STATIC_DRAW);
        
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, i.size()*sizeof(unsigned int), i.data(), GL_STATIC_DRAW);
    }
    
    void draw() {
        glBindVertexArray(vao);
        //legge indici dell'ebo: li raggruppa tre alla volta, uqanti indici deve leggere, tipo, 0 perchè parte dal primo byte
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    }
};

//organizzazione della scena
class Scene {
public:
    Camera cam;
    GPUMesh mesh; //una sola mesh sferica
    glm::mat4 mesh_base_norm;
    sf::Window* win;
    GLint m_loc, vp_loc, color_loc, light_pos_loc, is_sun_loc; //per conservare posizioni delle uniform nello shader

    Scene(std::string f, Shaders& s, sf::Window& w) : mesh(f), win(&w) { //lista di inizializzazione, costruisco subito gli oggetti
        //crea uniform nel programma shader, flag per trattare oggetti in modo differente
        m_loc=glGetUniformLocation(s.program, "model");
        vp_loc=glGetUniformLocation(s.program, "vp");
        color_loc=glGetUniformLocation(s.program, "color_obj");
        light_pos_loc=glGetUniformLocation(s.program, "light_pos");
        is_sun_loc=glGetUniformLocation(s.program, "is_sun");
        
        //matrice per centrare e normalizzare la mesh della sfera
        mesh_base_norm=scaling(1.0f/mesh.extent)*translation(-mesh.center);
        cam.view_projection(); //calcolo v e vp
    }

    void draw(float time) {
        cam.aspect=(float)win->getSize().x/(float)win->getSize().y; //calcola aspect ratio (larghezza/altezza)
        cam.view_projection();
        glUniformMatrix4fv(vp_loc, 1, GL_FALSE, &cam.vp[0][0]); //invia vp a shader, matrice, non farla trasposta e da indirizzo primo valore
        
        glUniform3f(light_pos_loc, 0.0f, 0.0f, 0.0f); //luce viene posta all'origine (posizione sole)

        //sole e pianeti
        //in generale centra mesh nell'origine e a dimensioni standard, la scala, la fa ruotare attorno al proprio centro, la trasla, definsice rotazione attorno al sole e definsice pianeta in sistema di riferitmento del sole
        //poi invia allo shader la matrice e il colore dell'oggetto, indica se è il sole oppure no e disegna
        glm::mat4 pos_sole=translation(0.0f, 0.0f, 0.0f); //traslo di 0 il sole
        glm::mat4 rot_sole=rotation_y(time*20.0f);
        glm::mat4 scale_sole=scaling(1.0f);
        glm::mat4 mm_sole=pos_sole*rot_sole*scale_sole*mesh_base_norm;
        
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_sole[0][0]);
        glUniform4f(color_loc, 1.0f, 0.8f, 0.0f, 1.0f); 
        glUniform1i(is_sun_loc, 1);
        mesh.draw();

        glm::mat4 orbita_terra=rotation_y(time*40.0f);
        glm::mat4 dist_terra=translation(3.5f, 0.0f, 0.0f); 
        glm::mat4 rot_terra=rotation_y(time*90.0f);
        glm::mat4 scale_terra=scaling(0.3f);
        glm::mat4 centro_terra=pos_sole*orbita_terra*dist_terra; 
        glm::mat4 mm_terra=centro_terra*rot_terra*scale_terra*mesh_base_norm;
        
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_terra[0][0]);
        glUniform4f(color_loc, 0.2f, 0.5f, 1.0f, 1.0f);
        glUniform1i(is_sun_loc, 0);
        mesh.draw();

        glm::mat4 orbita_luna=rotation_y(time*120.0f);
        glm::mat4 dist_luna=translation(0.8f, 0.0f, 0.0f);
        glm::mat4 scale_luna=scaling(0.1f);
        glm::mat4 centro_luna=centro_terra*orbita_luna*dist_luna;
        glm::mat4 mm_luna=centro_luna*scale_luna*mesh_base_norm;
        
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_luna[0][0]);
        glUniform4f(color_loc, 0.7f, 0.7f, 0.7f, 1.0f);
        glUniform1i(is_sun_loc, 0); 
        mesh.draw();
    }
};

//mainprogramma
int main(int argc, char** argv) {
    generaSferaOFF(); 
    
    Setup s;
    Shaders sh; //crea oggetto che gestisce compilazione degli shader e OpenGL
    
    const char* vs= 
        "#version 410 core\n"
        "layout(location=0) in vec3 pos;\n" //dati su pos vertici che riceve in ingresso
        "layout(location=1) in vec3 norm;\n" //dati su normali vertici che riceve in ingresso
        "uniform mat4 model;\n" //matrice dell'oggetto
        "uniform mat4 vp;\n" //matrice della camera
        //out sono output del vertex shader, interpolati durante rasterizzazione
        "out vec3 v_pos;\n"
        "out vec3 v_norm;\n"
        "void main() {\n"
        "    gl_Position=vp*model*vec4(pos, 1.0);\n" //porto pos in vec4, model porta vertice in spazio del mondo, vp in spazio della camera e di clip
        "    v_pos=vec3(model*vec4(pos, 1.0));\n" //calcola posizone del vertice nello spazio mondo
        "    v_norm=mat3(transpose(inverse(model)))*norm;\n" //trasforma la normale con inversa trasposta della model, mantiene normale perpendicolare anche con scalatura
        "}";

    const char* fs= 
        "#version 410 core\n"
        "uniform vec4 color_obj;\n"
        "uniform vec3 light_pos;\n" //contiene pos luce mondo
        "uniform int is_sun;\n"
        //riceve in input gli output omonimi del vs
        "in vec3 v_pos;\n"
        "in vec3 v_norm;\n"
        "out vec4 c;\n" //il colore prodotto dal fs
        "void main() {\n"
        "    if(is_sun==1) {\n"
        "        c=color_obj;\n"
        "    } else {\n" //se è sole pianeti o luna
        "        float ambientStrength=0.1;\n"
        "        vec3 ambient=ambientStrength*color_obj.rgb;\n"
        "        vec3 n=-normalize(v_norm);\n" //normalizzo normale del frammento, - per rivolgerla verso esterno
        "        vec3 lightDir=normalize(light_pos-v_pos);\n" //ottengo vettore dal frammento alla luce, normalizzazione elimina distanza lasciando solo direzione
        "        float diff=max(dot(n, lightDir), 0.0);\n" //luce diffusa: dot confronta orientamento sup. con direzione luce, max per eliminare val. negativi
        "        vec3 diffuse=diff*color_obj.rgb;\n"
        "        c=vec4(ambient+diffuse, 1.0);\n" //sommo luce ambientale+diffusa
        "    }\n"
        "}";
        
    sh.compile_attach_link(&vs, &fs); //creo ogetti vs e compilo, crea fs e compila, li collega ad un programma e fa linking
    sh.use(); //rende attivo il programma
    
    Scene sc("sfera.off", sh, *s.window); //costruttore scena

    glEnable(GL_DEPTH_TEST); //attivo depth test
    sf::Clock clk; //crea un cronometro
    
    while(s.window->isOpen()) { //ciclo ripetuto finchè la finestra è aperta
        while(const std::optional e=s.window->pollEvent()) { //se c'è evento pollEvent lo restituisce, altrimenti termina il ciclo
            if(e->is<sf::Event::Closed>()) { //se l'evento è di tipo closed
                return 0;
            }
            if(const auto* r=e->getIf<sf::Event::Resized>()) { //se è evento di ridimensionamento finestra
                glViewport(0, 0, r->size.x, r->size.y);
            }
            if(const auto* m=e->getIf<sf::Event::MouseMoved>()) { //se è evento di movimento del mouse
                static float px=m->position.x, py=m->position.y; //conservano posizione precendente
                if(sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) { //se mentre si muove il mouse il tasto sinistro è premuto
                    sc.cam.drag(m->position.x-px, m->position.y-py); //passa a drag differenza tra pos attuale e precedente
                }
                //aggiormo posizione precedente anche quando il tasto non è premuto, così se effettuo nuovo trascinamento non c'è salto
                px=m->position.x;
                py=m->position.y;
            }
        }
        
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //elimina colori fotogramma precedente e profondità
        
        sc.draw(clk.getElapsedTime().asSeconds());
        
        s.window->display(); //inverto back buffer con front buffer
    }
    return 0;
}
