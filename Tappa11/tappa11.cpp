#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <cstdlib>

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

//oggetto che rappresenta orbita, rappresentata come una linea
class GPUOrbit {
public:
    GLuint vao, vbo; //dichiaro vertex array object: contiene info su come leggere vertici, e vertex buffer object: buffer GPU che contiene coordinate dei vertici
    int point_count; //per ricordare quanti punti sono stati generati così glDrawArrays sa quanti vertici leggere dal vbo
    //costruttore orbita con raggio (distanza dal sole) e numero di segmenti del cerchio che approssimo con 100 segmenti
    GPUOrbit(float radius, int segments=100) {
        std::vector<float> vertices; //preparo lista delle cordiante dei vertici
        for(int i=0; i<=segments; ++i) { //genero i punti della circonferenza (>= perchè ultimo coincide con primo)
            float theta=2.0f*3.14159265359f*i/segments; //angolo del punto lungo la circonferenza
            vertices.push_back(radius*std::cos(theta)); //coordinata x
            vertices.push_back(0.0f); //coordinata y sempre 0 perchè orbita è piatta
            vertices.push_back(radius*std::sin(theta)); //coordinata z
        }
        point_count=vertices.size()/3; //conto i float nel vector e divido per 3 per sapere numero di vertici
        glGenBuffers(1, &vbo); //OpenGL crea id per il vbo
        glBindBuffer(GL_ARRAY_BUFFER, vbo); //rendo attivo quel buffer facendo bind a GL_ARRAY_BUFFER
        //copia da RAM a GPU: metto i dati sui vertici nel vbo
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glGenVertexArrays(1, &vao); //OpenGL crea id per vao
        glBindVertexArray(vao); //rendo attivo questo vao
        //dico a GPU come leggere dati del VBO: location dell'attr. in vertex shader, ogni vertice ha 3 coordinate, sono float, non normalizzare, distanza tra vertici, offset iniziale
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0); //abilita l'attributo 0
    }
    void draw() { //metodo che disegna orbita
        glBindVertexArray(vao); //riattivo il vao per recuperare configurazione
        glDrawArrays(GL_LINE_STRIP, 0, point_count); //GL_LINE_STRIP deve collegare punti in sequenza, parti da 0 e leggi tot. punti
    }
};

//oggetto che rappresenta stelle come punti
class GPUStars {
public:
    GLuint vao, vbo;
    int star_count;
    GPUStars(int count=600) {
        std::vector<float> vertices;
        for(int i=0; i<count; ++i) {
            float theta=static_cast<float>(rand())/RAND_MAX*2.0f*3.14159265359f; //prendiamo direzione casuale attorno al planetario
            float phi=static_cast<float>(rand())/RAND_MAX*3.14159265359f-3.14159265359f/2.0f; //le distruibiamo in altezza casuale
            float r=40.0f+static_cast<float>(rand())/RAND_MAX*20.0f; //distanza della stella dal centro tra 40 e 60
            vertices.push_back(r*std::cos(phi)*std::cos(theta));
            vertices.push_back(r*std::sin(phi));
            vertices.push_back(r*std::cos(phi)*std::sin(theta));
        }
        star_count=vertices.size()/3;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
    void draw() {
        glBindVertexArray(vao);
        glPointSize(2.0f); 
        glDrawArrays(GL_POINTS, 0, star_count);
    }
};

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
        window=new sf::Window(sf::VideoMode({800, 800}), "Planetario Tappa 11 - Sistema Solare Completo!", sf::Style::Default, sf::State::Windowed, settings);
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
    float phi=30.0f, theta=30.0f, fd=2.0f, od=20.0f;
    
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
        float ncp=std::max(0.1f, od-15.0f);
        float fcp=120.0f;
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
    GPUStars stars;
    GPUOrbit orb_mercurio, orb_venere, orb_terra, orb_marte, orb_giove, orb_luna;
    glm::mat4 mesh_base_norm;
    sf::Window* win;
    GLint m_loc, vp_loc, light_pos_loc, is_sun_loc, tex_loc, color_loc, is_orbit_loc, is_star_loc; //per conservare posizioni delle uniform nello shader
    GLuint t_sole, t_mercurio, t_venere, t_terra, t_luna, t_marte, t_giove; //id restituiti da caricaTexture

    Scene(std::string f, Shaders& s, sf::Window& w) //lista di inizializzazione, costruisco subito gli oggetti
        : mesh(f), stars(800),
          orb_mercurio(1.8f), orb_venere(2.8f), orb_terra(4.0f), orb_marte(5.5f), orb_giove(7.5f), orb_luna(0.8f),
          win(&w) {
        
        //crea uniform nel programma shader, flag per trattare oggetti in modo differente
        m_loc=glGetUniformLocation(s.program, "model");
        vp_loc=glGetUniformLocation(s.program, "vp");
        light_pos_loc=glGetUniformLocation(s.program, "light_pos");
        is_sun_loc=glGetUniformLocation(s.program, "is_sun");
        tex_loc=glGetUniformLocation(s.program, "tex_sampler");
        color_loc=glGetUniformLocation(s.program, "color_obj");
        is_orbit_loc=glGetUniformLocation(s.program, "is_orbit");
        is_star_loc=glGetUniformLocation(s.program, "is_star");

        //carico img texture e viene restituitp il suo id OpenGL
        t_sole=caricaTexture("../Risorse/sole.jpg");
        t_mercurio=caricaTexture("../Risorse/mercurio.jpeg");
        t_venere=caricaTexture("../Risorse/venere.jpg");
        t_terra=caricaTexture("../Risorse/terra.jpg");
        t_luna=caricaTexture("../Risorse/luna.jpg");
        t_marte=caricaTexture("../Risorse/marte.jpg");
        t_giove=caricaTexture("../Risorse/giove.jpg");
        
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

        glm::mat4 pos_sole=translation(0.0f, 0.0f, 0.0f); //traslo di 0 il sole

        //stelle di sfondo
        glUniform1i(is_star_loc, 1); //posizione dello uniform, viene impostato a 1 per comunicare a shader che la prossima draw call rigaurda le stelle
        glm::mat4 mm_stars=translation(0.0f, 0.0f, 0.0f);
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_stars[0][0]); //invia mm_stars a uniform model nella posizone m_loc, è una matrice e non viene trasposta
        stars.draw(); ///disegno le stelle
        glUniform1i(is_star_loc, 0); //disattivo modalità stelle

        //orbite geometriche
        glUniform1i(is_orbit_loc, 1);
        glUniform4f(color_loc, 0.25f, 0.25f, 0.35f, 1.0f); //imposto uniform del colore: rgb e alpha per opacità

        //costruisco orbite
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_mercurio.draw();
        
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_venere.draw();
        
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_terra.draw();
        
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_marte.draw();
        
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_giove.draw();

        //descrivo dove si trova il centro della terra
        glm::mat4 c_terra=pos_sole*rotation_y(simulated_time*30.0f)*translation(4.0f, 0.0f, 0.0f);
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &c_terra[0][0]); //passo c_terra come matrice model, e disegno orbita luna che sarà cetnrata nella terra
        orb_luna.draw();

        glUniform1i(is_orbit_loc, 0); //disattivo modalità orbite

        //sole e pianeti
        //in generale centra mesh nell'origine e a dimensioni standard, la scala, la fa ruotare attorno al proprio centro, la trasla, definsice rotazione attorno al sole e definsice pianeta in sistema di riferitmento del sole
        //poi invia a shader matrice del pianeta, dice se è il sole oppure no, collega la texture del pianeta ad unità texture attiva e disegna
        glm::mat4 mm_sole=pos_sole*rotation_y(simulated_time*10.0f)*scaling(1.2f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_sole[0][0]);
        glUniform1i(is_sun_loc, 1);
        glBindTexture(GL_TEXTURE_2D, t_sole);
        mesh.draw();

        glm::mat4 mm_merc=pos_sole*rotation_y(simulated_time*80.0f)*translation(1.8f, 0.0f, 0.0f)*rotation_y(simulated_time*50.0f)*scaling(0.12f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_merc[0][0]);
        glUniform1i(is_sun_loc, 0);
        glBindTexture(GL_TEXTURE_2D, t_mercurio);
        mesh.draw();

        glm::mat4 mm_ven=pos_sole*rotation_y(simulated_time*50.0f)*translation(2.8f, 0.0f, 0.0f)*rotation_y(simulated_time*30.0f)*scaling(0.22f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_ven[0][0]);
        glUniform1i(is_sun_loc, 0);
        glBindTexture(GL_TEXTURE_2D, t_venere);
        mesh.draw();

        glm::mat4 mm_terra=c_terra*rotation_y(simulated_time*90.0f)*scaling(0.3f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_terra[0][0]);
        glUniform1i(is_sun_loc, 0); 
        glBindTexture(GL_TEXTURE_2D, t_terra);
        mesh.draw();

        glm::mat4 mm_luna=c_terra*rotation_y(simulated_time*120.0f)*translation(0.8f, 0.0f, 0.0f)*scaling(0.09f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_luna[0][0]);
        glUniform1i(is_sun_loc, 0); 
        glBindTexture(GL_TEXTURE_2D, t_luna);
        mesh.draw();

        glm::mat4 mm_marte=pos_sole*rotation_y(simulated_time*24.0f)*translation(5.5f, 0.0f, 0.0f)*rotation_y(simulated_time*70.0f)*scaling(0.2f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_marte[0][0]);
        glUniform1i(is_sun_loc, 0);
        glBindTexture(GL_TEXTURE_2D, t_marte);
        mesh.draw();

        glm::mat4 mm_giove=pos_sole*rotation_y(simulated_time*10.0f)*translation(7.5f, 0.0f, 0.0f)*rotation_y(simulated_time*150.0f)*scaling(0.7f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_giove[0][0]);
        glUniform1i(is_sun_loc, 0);
        glBindTexture(GL_TEXTURE_2D, t_giove);
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
        "uniform int is_star;\n" //flag
        //out sono output del vertex shader, interpolati durante rasterizzazione
        "out vec3 v_pos;\n"
        "out vec3 v_norm;\n"
        "out vec3 v_local_pos;\n"
        "void main() {\n"
        "    gl_Position=vp*model*vec4(pos, 1.0);\n" //porto pos in vec4, model porta vertice in spazio del mondo, vp in spazio della camera e di clip
        "    if(is_star==1) {\n"
        "       gl_PointSize=2.0;\n" //dimensione stelle fissa a 2 pixel
        "    }\n"
        "    v_pos=vec3(model*vec4(pos, 1.0));\n" //calcola posizone del vertice nello spazio mondo
        "    v_norm=mat3(transpose(inverse(model)))*norm;\n" //trasforma la normale con inversa trasposta della model, mantiene normale perpendicolare anche con scalatura
        "    v_local_pos=pos;\n" //conserva posizione originale del vertice
        "}";

    const char* fs= 
        "#version 410 core\n"
        "uniform sampler2D tex_sampler;\n" //indica da quale unità texture deve essere letta la texture 2D
        "uniform vec3 light_pos;\n" //contiene pos luce mondo
        "uniform int is_sun;\n"
        "uniform int is_orbit;\n"
        "uniform int is_star;\n"
        "uniform vec4 color_obj;\n" //contiene colore uniforme orbite
        //riceve in input gli output omonimi del vs
        "in vec3 v_pos;\n"
        "in vec3 v_norm;\n"
        "in vec3 v_local_pos;\n"
        "out vec4 c;\n" //il colore prodotto dal fs
        "void main() {\n"
        "    if(is_star==1) {\n" //se è una stelle imposta questo colore
        "        //stellebiancogialleluminose\n"
        "        c=vec4(1.0, 1.0, 0.9, 1.0);\n"
        "    } else if(is_orbit==1) {\n" //se è un'orbita imposta questo
        "        c=color_obj;\n"
        "    } else {\n" //se è sole pianeti o luna
        "        vec3 p=normalize(v_local_pos);\n" //trasforma pos locale interpolata in una direzione unitaria ddal centro della sfera: (tra -pi e pi), divido per 2*pi e + 0.5, ho intervallo 0-1
        "        float u=0.5+atan(p.z, p.x)/(2.0*3.14159265);\n" //coordinata orizzontale, indica quanto si è a sx. o dx. della texture: (tra -pi/2 e pi/2), divido per pi e -0.5, ho intervallo 0-1
        "        float v=0.5-asin(p.y)/3.14159265;\n" //coordianta verticale della texture
        "        vec4 texColor=texture(tex_sampler, vec2(u, v));\n" //legge unità texture indicata da tex_sampler, trova la texture bindata, la campiona nel punto (u,v) restituisce colore in quel frammento
        "        if(is_sun==1) {\n"
        "            c=texColor;\n" //se è il sole il colore finale è già qeusto, non viene illuminato
        "        } else {\n" //altrimenti applico illuminazione
        "            float ambientStrength=0.05;\n" //luce ambientale
        "            vec3 ambient=ambientStrength*texColor.rgb;\n"
        "            vec3 n=-normalize(v_norm);\n" //normalizzo normale del frammento, - per rivolgerla verso esterno
        "            vec3 lightDir=normalize(light_pos-v_pos);\n" //ottengo vettore dal frammento alla luce, normalizzazione elimina distanza lasciando solo direzione
        "            float diff=max(dot(n, lightDir), 0.0);\n" //luce diffusa: dot confronta orientamento sup. con direzione luce, max per eliminare val. negativi
        "            vec3 diffuse=diff*texColor.rgb;\n"
        "            c=vec4(ambient+diffuse, 1.0);\n" //sommo luce ambientale+diffusa
        "        }\n"
        "    }\n"
        "}";
        
    sh.compile_attach_link(&vs, &fs); //creo ogetti vs e compilo, crea fs e compila, li collega ad un programma e fa linking
    sh.use(); //rende attivo il programma
    
    Scene sc("sfera.off", sh, *s.window); //costruttore scena

    glEnable(GL_DEPTH_TEST); //attivo depth test
    glEnable(0x8642); //abilita GL_PROGRAM_POINT_SIZE, permette di impostare dimensione pixel punti stella
    sf::Clock clk; //crea un cronometro
    
    float simulated_time=0.0f; //contiene il tempo della simulazione
    float time_scale=1.0f; //moltiplicatore della velocità
    bool is_paused=false; //per indicare se simulazione in pausa o no

    std::cout<<"\n[tappa 11] sistema solare completo e stellato avviato!\n";

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
                }
                if(k->code==sf::Keyboard::Key::Up) {
                    time_scale+=0.5f;
                }
                if(k->code==sf::Keyboard::Key::Down) {
                    time_scale-=0.5f;
                }
            }
        }

        if(!is_paused) {
            simulated_time+=dt*time_scale; //se non è in pausa prendi tempo reale e acceleri, rallenti, inverti o rimane invariato
        }

        glClearColor(0.01f, 0.01f, 0.03f, 1.0f); //colore di pulizia per lo sfondo
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //elimina colori fotogramma precedente e profondità
        
        sc.draw(simulated_time); //passa il tempo simulato alla scena
        
        s.window->display(); //inverto back buffer con front buffer
    }
    return 0;
}
