#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

#include "./include/matrices.hh"
#include "./include/mesh.hh"
#include "./include/hotshaders.hh"


class Setup
{
public:
    sf::Window* window; //puntatore all'oggetto finestra per risorse e flessibilità
    //costruttore
    Setup()
    {
        sf::ContextSettings settings; //oggetto settings per configurare gpu con impostazioni per finestra
        settings.depthBits= 24;
        settings.antiAliasingLevel= 4;
        settings.attributeFlags= sf::ContextSettings::Core;
        settings.majorVersion= 4;
        settings.minorVersion= 1;
        window= new sf::Window(sf::VideoMode({800, 800}), "Planetario Tappa 04", sf::State::Windowed, settings);
        window->setVerticalSyncEnabled(true);
        gladLoadGL(sf::Context::getFunction);
    }
};


class Camera
{
public:
    glm::mat4 v, vp; //matrice di vista e di vista-proiezione
    float aspect= 1.0f; //aspect ratio(larghezza/altezza)
    float phi= 30.0f; //angolo orizzontale a 30 gradi
    float theta= 30.0f; //angolo verticale a 30 gradi
    float fd= 4.0f; //distanza focale
    float od= 4.0f; //distanza dell'oggetto
    //funzione che prende lo spostamento del mouse
    void drag(float dx, float dy)
    {
        phi += dx*0.5f; //aggiorna angolo orizzontale
        theta= std::clamp(theta+dy*0.5f, -89.0f, 89.0f); //aggiorna angolo verticale bloccato tra -89 e 89 gradi
        view_projection(); //chiama aggiornamento
    }
    void view_projection()
    {
        //creiamo 3 matrici per posizione telecamera e cosa vede
        glm::mat4 ry= rotation_y(phi);
        glm::mat4 rx= rotation_x(theta);
        glm::mat4 tz= translation(0, 0, -od);

        float ncp= std::max(0.1f, od-1.0f); //piano di taglio vicino
        float fcp= od+1.0f; //piano di taglio lontano
        //costanti prospettiche
        float a= (fcp+ncp)/(ncp-fcp);
        float b= 2.0f*fcp*ncp/(ncp-fcp);
        //matrice di proiezione prospettica e fd/aspect per evitare stretching
        glm::mat4 pr= glm::mat4(fd/aspect, 0, 0, 0, 0, fd, 0, 0, 0, 0, a, -1.0, 0, 0, b, 0);
        //creiamo matrice di vista e di vista-proiezione
        v= tz*rx*ry;
        vp= pr*v;
    }
};

class GPUMesh //per la gestione della VRAM
{
public:
    glm::vec3 center;
    float extent;
    GLuint vao, vbo, ebo;
    size_t index_count;

    GPUMesh(std::string f)
    {
        Mesh m(f); //usiamo calsse Mesh per leggere il file .off
        //copiamo centro e gradndezza
        center= m.center;
        extent= m.extent;

        std::vector<float> p;
        std::vector<unsigned int> i;
        m.pack4gpu(p, i);
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
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    }
};

class Scene
{
public:
    Camera cam;
    GPUMesh mesh;
    glm::mat4 mm;
    sf::Window* win;
    GLint m_loc, vp_loc;
    Scene(std::string f, Shaders& s, sf::Window& w) : mesh(f), win(&w)
    {
        m_loc= glGetUniformLocation(s.program, "model");
        vp_loc= glGetUniformLocation(s.program, "vp");
        mm= scaling(1.0f/mesh.extent)*translation(-mesh.center);
    }
    void draw(float time)
    {
        cam.aspect= (float)win->getSize().x / (float)win->getSize().y;
        cam.view_projection();
        glm::mat4 mm_final= rotation_y(time*40.0f)*rotation_x(time*20.0f)*mm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_final[0][0]);
        glUniformMatrix4fv(vp_loc, 1, GL_FALSE, &cam.vp[0][0]);
        mesh.draw();
    }
};

int main(int argc, char** argv)
{
    Setup s;
    Shaders sh;
    const char* vs= "#version 410 core\n layout(location=0) in vec3 pos; uniform mat4 model; uniform mat4 vp; void main() { gl_Position = vp * model * vec4(pos, 1.0); }";
    const char* fs= "#version 410 core\n out vec4 c; void main() { c = vec4(0.2, 0.7, 0.5, 1.0); }";
    sh.compile_attach_link(&vs, &fs);
    sh.use();

    Scene sc("cubo.off", sh, *s.window);

    glEnable(GL_DEPTH_TEST);
    sf::Clock clk;
    while(s.window->isOpen())
    {
        while(const std::optional e= s.window->pollEvent())
        {
            if(e->is<sf::Event::Closed>())
                return 0;
            if(const auto* r= e->getIf<sf::Event::Resized>()) 
                glViewport(0, 0, r->size.x, r->size.y);
            if(const auto* m= e->getIf<sf::Event::MouseMoved>())
            {
                static float px= m->position.x;
                static float py= m->position.y;
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
                    sc.cam.drag(m->position.x-px, m->position.y-py);
                px= m->position.x;
                py= m->position.y;
            } 
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        sc.draw(clk.getElapsedTime().asSeconds());
        s.window->display();
    }
}