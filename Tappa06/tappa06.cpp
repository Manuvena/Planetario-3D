#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

#include "./include/matrices.hh"
#include "./include/mesh.hh"
#include "./include/hotshaders.hh"

//setup
class Setup {
public:
    sf::Window* window;
    Setup() {
        sf::ContextSettings settings;
        settings.depthBits=24;
        settings.stencilBits=8;
        settings.antiAliasingLevel=4;
        settings.attributeFlags=sf::ContextSettings::Attribute::Core;
        settings.majorVersion=4;
        settings.minorVersion=1;
        //aggiornatotitolofinestra
        window=new sf::Window(sf::VideoMode({800, 800}), "Planetario Tappa 06", sf::Style::Default, sf::State::Windowed, settings);
        window->setVerticalSyncEnabled(true);
        if(!window->setActive(true)) {
            exit(1);
        }
        gladLoadGL(sf::Context::getFunction);
    }
    ~Setup() {
        delete window;
    }
};

//cameragestione
class Camera {
public:
    glm::mat4 v, vp;
    float aspect=1.0f;
    float phi=30.0f, theta=30.0f, fd=4.0f, od=4.0f;
    
    void drag(float dx, float dy) {
        phi+=dx*0.5f;
        theta=std::clamp(theta+dy*0.5f, -89.0f, 89.0f);
        view_projection();
    }
    
    void view_projection() {
        glm::mat4 ry=rotation_y(phi);
        glm::mat4 rx=rotation_x(theta);
        glm::mat4 tz=translation(0, 0, -od);
        float ncp=std::max(0.1f, od-10.0f);
        float fcp=od+10.0f;
        float a=(fcp+ncp)/(ncp-fcp);
        float b=2.0f*fcp*ncp/(ncp-fcp);
        glm::mat4 pr=glm::mat4(fd/aspect, 0, 0, 0, 0, fd, 0, 0, 0, 0, a, -1.0, 0, 0, b, 0);
        v=tz*rx*ry;
        vp=pr*v;
    }
};

//gpumeshcaricamento
class GPUMesh {
public:
    glm::vec3 center;
    float extent;
    GLuint vao, vbo, ebo;
    size_t index_count;
    
    GPUMesh(std::string f) {
        Mesh m(f);
        center=m.center;
        extent=m.extent;
        std::vector<float> p;
        std::vector<unsigned int> i;
        m.pack4gpu(p, i);
        index_count=i.size();
        
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, p.size()*sizeof(float), p.data(), GL_STATIC_DRAW);
        
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);
        glEnableVertexAttribArray(0);
        
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, i.size()*sizeof(unsigned int), i.data(), GL_STATIC_DRAW);
    }
    
    void draw() {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    }
};

//scenacoloriemateriali
class Scene {
public:
    Camera cam; 
    GPUMesh mesh; 
    glm::mat4 mesh_base_norm; 
    sf::Window* win;
    GLint m_loc, vp_loc, color_loc;

    Scene(std::string f, Shaders& s, sf::Window& w) : mesh(f), win(&w) {
        m_loc=glGetUniformLocation(s.program, "model");
        vp_loc=glGetUniformLocation(s.program, "vp");
        //cerchiamovariabilenelloshader
        color_loc=glGetUniformLocation(s.program, "color_obj");
        
        mesh_base_norm=scaling(1.0f/mesh.extent)*translation(-mesh.center);

        cam.od=15.0f; 
        cam.fd=2.0f;
        cam.view_projection();
    }

    void draw(float time) {
        cam.aspect=(float)win->getSize().x/(float)win->getSize().y;
        cam.view_projection();
        glUniformMatrix4fv(vp_loc, 1, GL_FALSE, &cam.vp[0][0]);

        //disegnosolegiallo
        glm::mat4 pos_sole=translation(0.0f, 0.0f, 0.0f);
        glm::mat4 rot_sole=rotation_y(time*20.0f); 
        glm::mat4 scale_sole=scaling(1.0f); 
        glm::mat4 mm_sole=pos_sole*rot_sole*scale_sole*mesh_base_norm;
        
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_sole[0][0]);
        //dipingidigiallo
        glUniform4f(color_loc, 1.0f, 0.8f, 0.0f, 1.0f); 
        mesh.draw();

        //disegnoterraazzurra
        glm::mat4 orbita_terra=rotation_y(time*40.0f); 
        glm::mat4 dist_terra=translation(3.5f, 0.0f, 0.0f); 
        glm::mat4 rot_terra=rotation_y(time*90.0f); 
        glm::mat4 scale_terra=scaling(0.3f); 
        glm::mat4 centro_terra=pos_sole*orbita_terra*dist_terra; 
        glm::mat4 mm_terra=centro_terra*rot_terra*scale_terra*mesh_base_norm;

        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_terra[0][0]);
        //dipingidiazzurro
        glUniform4f(color_loc, 0.2f, 0.5f, 1.0f, 1.0f);
        mesh.draw();

        //disegnolunagrigia
        glm::mat4 orbita_luna=rotation_y(time*120.0f); 
        glm::mat4 dist_luna=translation(0.8f, 0.0f, 0.0f); 
        glm::mat4 scale_luna=scaling(0.1f); 
        glm::mat4 centro_luna=centro_terra*orbita_luna*dist_luna;
        glm::mat4 mm_luna=centro_luna*scale_luna*mesh_base_norm;

        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_luna[0][0]);
        //dipingidigrigio
        glUniform4f(color_loc, 0.7f, 0.7f, 0.7f, 1.0f);
        mesh.draw();
    }
};

//mainprogramma
int main(int argc, char** argv) {
    Setup s;
    Shaders sh;
    
    const char* vs="#version 410 core\n layout(location=0) in vec3 pos; uniform mat4 model; uniform mat4 vp; void main() { gl_Position=vp*model*vec4(pos, 1.0); }";
    const char* fs="#version 410 core\n uniform vec4 color_obj; out vec4 c; void main() { c=color_obj; }"; 
    
    sh.compile_attach_link(&vs, &fs); 
    sh.use();

    //usiamopercorsorelativo
    Scene sc("../Risorse/cubo.off", sh, *s.window);

    glEnable(GL_DEPTH_TEST);
    sf::Clock clk;
    
    while(s.window->isOpen()) {
        while(const std::optional e=s.window->pollEvent()) {
            if(e->is<sf::Event::Closed>()) {
                return 0;
            }
            if(const auto* r=e->getIf<sf::Event::Resized>()) {
                glViewport(0, 0, r->size.x, r->size.y);
            }
            if(const auto* m=e->getIf<sf::Event::MouseMoved>()) {
                static float px=m->position.x, py=m->position.y;
                if(sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    sc.cam.drag(m->position.x-px, m->position.y-py);
                }
                px=m->position.x;
                py=m->position.y;
            }
        }
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        sc.draw(clk.getElapsedTime().asSeconds());
        
        s.window->display();
    }
    return 0;
}