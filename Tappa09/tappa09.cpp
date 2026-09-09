#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
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

//funzione che carica immagine e restituisce id OpenGL della texture
GLuint caricaTexture(const std::string& path) {
    sf::Image img; //immagine lato cpu letta da SFML
    if(!img.loadFromFile(path)) { //se non riesce a leggere immagine crea texture bianca
        std::cout<<"[ERRORE] Impossibile trovare '"<<path<<"'. Uso texture bianca.\n";
        img.resize({1, 1}, sf::Color::White); 
    }
    img.flipVertically(); //si ribalta per allineare origine con quella attesa da OpenGL
    
    GLuint tex_id; //dichairo var che conterrà id della texture
    glGenTextures(1, &tex_id); //chiede a OpenGL un nuovo id per una texture e lo salvo in tex_id
    glBindTexture(GL_TEXTURE_2D, tex_id); //quando parlo di GL_TEXTURE_2D parlo della texture con id tex_id
    //copia i pixel dell'img da RAM a memoria texture della GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D); //genera versioni ridotte della texture per minification 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //filtro di minification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //filtro di magnification
    return tex_id; //restituisce l'id texture
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
        window=new sf::Window(sf::VideoMode({800, 800}), "Planetario Tappa 09 - Interattivita!", sf::Style::Default, sf::State::Windowed, settings);
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
    GLint m_loc, vp_loc, light_pos_loc, is_sun_loc, tex_loc; //per conservare posizioni delle uniform nello shader
    GLuint tex_sole, tex_terra, tex_luna; //id restituiti da caricaTexture

    Scene(std::string f, Shaders& s, sf::Window& w) : mesh(f), win(&w) { //lista di inizializzazione, costruisco subito gli oggetti
        //crea uniform nel programma shader, flag per trattare oggetti in modo differente
        m_loc=glGetUniformLocation(s.program, "model");
        vp_loc=glGetUniformLocation(s.program, "vp");
        light_pos_loc=glGetUniformLocation(s.program, "light_pos");
        is_sun_loc=glGetUniformLocation(s.program, "is_sun");
        tex_loc=glGetUniformLocation(s.program, "tex_sampler");

        //carico img texture e viene restituitp il suo id OpenGL
        tex_sole=caricaTexture("../Risorse/sole.jpg");
        tex_terra=caricaTexture("../Risorse/terra.jpg");
        tex_luna=caricaTexture("../Risorse/luna.jpg");
        
        //matrice per centrare e normalizzare la mesh della sfera
        mesh_base_norm=scaling(1.0f/mesh.extent)*translation(-mesh.center);
        cam.view_projection(); //calcolo v e vp
    }

    void draw(float simulated_time) {
        cam.aspect=(float)win->getSize().x/(float)win->getSize().y; //calcola aspect ratio (larghezza/altezza)
        cam.view_projection();
        glUniformMatrix4fv(vp_loc, 1, GL_FALSE, &cam.vp[0][0]); //invia vp a shader, matrice, non farla trasposta e da indirizzo primo valore
        
        glUniform3f(light_pos_loc, 0.0f, 0.0f, 0.0f); //luce viene posta all'origine (posizione sole)
        glUniform1i(tex_loc, 0); //tex_loc è la posizione dello uniform tex_sampler ottenuta prima, gli passa valore 0, dice a fragment shader di leggere da unità texture numero 0
        glActiveTexture(GL_TEXTURE0); //rende attiva l'unità texture numero 0

        //sole e pianeti
        //in generale centra mesh nell'origine e a dimensioni standard, la scala, la fa ruotare attorno al proprio centro, la trasla, definsice rotazione attorno al sole e definsice pianeta in sistema di riferitmento del sole
        //poi invia a shader matrice del pianeta, dice se è il sole oppure no, collega la texture del pianeta ad unità texture attiva e disegna
        glm::mat4 pos_sole=translation(0.0f, 0.0f, 0.0f); //traslo di 0 il sole
        glm::mat4 mm_sole=pos_sole*rotation_y(simulated_time*20.0f)*scaling(1.0f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_sole[0][0]);
        glUniform1i(is_sun_loc, 1);
        glBindTexture(GL_TEXTURE_2D, tex_sole);
        mesh.draw();

        glm::mat4 centro_terra=pos_sole*rotation_y(simulated_time*40.0f)*translation(3.5f, 0.0f, 0.0f); 
        glm::mat4 mm_terra=centro_terra*rotation_y(simulated_time*90.0f)*scaling(0.3f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_terra[0][0]);
        glUniform1i(is_sun_loc, 0); 
        glBindTexture(GL_TEXTURE_2D, tex_terra);
        mesh.draw();

        glm::mat4 centro_luna=centro_terra*rotation_y(simulated_time*120.0f)*translation(0.8f, 0.0f, 0.0f);
        glm::mat4 mm_luna=centro_luna*scaling(0.1f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_luna[0][0]);
        glUniform1i(is_sun_loc, 0); 
        glBindTexture(GL_TEXTURE_2D, tex_luna);
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
        "out vec3 v_local_pos;\n"
        "void main() {\n"
        "    gl_Position=vp*model*vec4(pos, 1.0);\n" //porto pos in vec4, model porta vertice in spazio del mondo, vp in spazio della camera e di clip
        "    v_pos=vec3(model*vec4(pos, 1.0));\n" //calcola posizone del vertice nello spazio mondo
        "    v_norm=mat3(transpose(inverse(model)))*norm;\n" //trasforma la normale con inversa trasposta della model, mantiene normale perpendicolare anche con scalatura
        "    v_local_pos=pos;\n" //conserva posizione originale del vertice
        "}";

    const char* fs= 
        "#version 410 core\n"
        "uniform sampler2D tex_sampler;\n" //indica da quale unità texture deve essere letta la texture 2D
        "uniform vec3 light_pos;\n" //contiene pos luce mondo
        "uniform int is_sun;\n"
        //riceve in input gli output omonimi del vs
        "in vec3 v_pos;\n"
        "in vec3 v_norm;\n"
        "in vec3 v_local_pos;\n"
        "out vec4 c;\n" //il colore prodotto dal fs
        "void main() {\n"
        "    vec3 p=normalize(v_local_pos);\n" //trasforma pos locale interpolata in una direzione unitaria ddal centro della sfera: (tra -pi e pi), divido per 2*pi e + 0.5, ho intervallo 0-1
        "    float u=0.5+atan(p.z, p.x)/(2.0*3.14159265);\n" //coordinata orizzontale, indica quanto si è a sx. o dx. della texture: (tra -pi/2 e pi/2), divido per pi e -0.5, ho intervallo 0-1
        "    float v=0.5-asin(p.y)/3.14159265;\n" //coordianta verticale della texture
        "    vec4 texColor=texture(tex_sampler, vec2(u, v));\n" //legge unità texture indicata da tex_sampler, trova la texture bindata, la campiona nel punto (u,v) restituisce colore in quel frammento
        "    if(is_sun==1) {\n"
        "        c=texColor;\n" //se è il sole il colore finale è già qeusto, non viene illuminato
        "    } else {\n" //altrimenti applico illuminazione
        "        float ambientStrength=0.05;\n" //luce ambientale
        "        vec3 ambient=ambientStrength*texColor.rgb;\n"
        "        vec3 n=-normalize(v_norm);\n" //normalizzo normale del frammento, - per rivolgerla verso esterno
        "        vec3 lightDir=normalize(light_pos-v_pos);\n" //ottengo vettore dal frammento alla luce, normalizzazione elimina distanza lasciando solo direzione
        "        float diff=max(dot(n, lightDir), 0.0);\n" //luce diffusa: dot confronta orientamento sup. con direzione luce, max per eliminare val. negativi
        "        vec3 diffuse=diff*texColor.rgb;\n"
        "        c=vec4(ambient+diffuse, 1.0);\n" //sommo luce ambientale+diffusa
        "    }\n"
        "}";
        
    sh.compile_attach_link(&vs, &fs); //creo ogetti vs e compilo, crea fs e compila, li collega ad un programma e fa linking
    sh.use(); //rende attivo il programma
    
    Scene sc("sfera.off", sh, *s.window); //costruttore scena

    glEnable(GL_DEPTH_TEST); //attivo depth test
    sf::Clock clk; //crea un cronometro
    
    float simulated_time=0.0f; //contiene il tempo della simulazione
    float time_scale=1.0f; //moltiplicatore della velocità
    bool is_paused=false; //per indicare se simulazione in pausa o no

    std::cout<<"\n========================================\n";
    cout_controls:
    std::cout<<" COMANDI TASTIERA ATTIVI:\n";
    std::cout<<" [BARRA SPAZIO] : Pausa / Riprendi\n";
    std::cout<<" [FRECCIA SU]   : Aumenta velocità\n";
    std::cout<<" [FRECCIA GIU'] : Rallenta / Inverti\n";
    std::cout<<"========================================\n\n";

    while(s.window->isOpen()) { //ciclo ripetuto finchè la finestra è aperta
        float dt=clk.restart().asSeconds(); //restituisce tempo da ultimo riavvio, riavvia cronometro, converte in secondi
        
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

            if(const auto* k=e->getIf<sf::Event::KeyPressed>()) { //se viene premuto un tasto
                if(k->code==sf::Keyboard::Key::Space) {
                    is_paused=!is_paused;
                    std::cout<<(is_paused ? "[TEMPO] Pausa attivata.\n" : "[TEMPO] Ripresa.\n");
                }
                if(k->code==sf::Keyboard::Key::Up) {
                    time_scale+=0.5f;
                    std::cout<<"[TEMPO] Velocità aumentata a: "<<time_scale<<"x\n";
                }
                if(k->code==sf::Keyboard::Key::Down) {
                    time_scale-=0.5f;
                    std::cout<<"[TEMPO] Velocità ridotta a: "<<time_scale<<"x\n";
                }
            }
        }

        if(!is_paused) {
            simulated_time+=dt*time_scale; //se non è in pausa prendi tempo reale e acceleri, rallenti, inverti o rimane invariato
        }

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //elimina colori fotogramma precedente e profondità
        
        sc.draw(simulated_time); //passa il tempo simulato alla scena
        
        s.window->display(); //inverto back buffer con front buffer
    }
    return 0;
}
