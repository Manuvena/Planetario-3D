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

//funzionidisupporto
void generaSferaOFF() {
    std::ofstream out("sfera.off");
    int parallels=40, meridians=40;
    int num_vertices=parallels*meridians+2;
    int num_faces=parallels*meridians*2;
    
    out<<"OFF\n"<<num_vertices<<" "<<num_faces<<" 0\n";
    out<<"0.0 -1.0 0.0\n";
    
    for(int i=1; i<=parallels; i++) {
        float theta=3.14159265359f*i/(parallels+1.0f);
        for(int j=0; j<meridians; j++) {
            float phi=2.0f*3.14159265359f*j/meridians;
            out<<(std::sin(theta)*std::cos(phi))<<" "<<-std::cos(theta)<<" "<<(std::sin(theta)*std::sin(phi))<<"\n";
        }
    }
    
    out<<"0.0 1.0 0.0\n";
    
    for(int j=0; j<meridians; j++) {
        out<<"3 0 "<<(1+(j+1)%meridians)<<" "<<(1+j)<<"\n";
    }
    
    for(int i=0; i<parallels-1; i++) {
        for(int j=0; j<meridians; j++) {
            int next_j=(j+1)%meridians;
            int v0=1+i*meridians+j, v1=1+i*meridians+next_j;
            int v2=1+(i+1)*meridians+next_j, v3=1+(i+1)*meridians+j;
            out<<"3 "<<v0<<" "<<v1<<" "<<v3<<"\n";
            out<<"3 "<<v1<<" "<<v2<<" "<<v3<<"\n";
        }
    }
    
    int nord=num_vertices-1, start=1+(parallels-1)*meridians;
    for(int j=0; j<meridians; j++) {
        out<<"3 "<<nord<<" "<<(start+j)<<" "<<(start+(j+1)%meridians)<<"\n";
    }
    
    out.close();
}

//caricamentotexture
GLuint caricaTexture(const std::string& path) {
    sf::Image img;
    if(!img.loadFromFile(path)) {
        std::cout<<"[ERRORE] Impossibile trovare '"<<path<<"'. Uso texture bianca.\n";
        img.resize({1, 1}, sf::Color::White); 
    }
    img.flipVertically(); 
    
    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex_id;
}

//setupfinestra
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
        window=new sf::Window(sf::VideoMode({800, 800}), "Planetario Tappa 09 - Interattivita!", sf::Style::Default, sf::State::Windowed, settings);
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

//gestionecreazionecamera
class Camera {
public:
    glm::mat4 v, vp;
    float aspect=1.0f;
    float phi=30.0f, theta=30.0f, fd=2.0f, od=15.0f; 
    
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

//gestionecreazionemesh
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
        
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, i.size()*sizeof(unsigned int), i.data(), GL_STATIC_DRAW);
    }
    
    void draw() {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    }
};

//gestionescena
class Scene {
public:
    Camera cam;
    GPUMesh mesh;
    glm::mat4 mesh_base_norm;
    sf::Window* win;
    GLint m_loc, vp_loc, light_pos_loc, is_sun_loc, tex_loc;
    GLuint tex_sole, tex_terra, tex_luna;

    Scene(std::string f, Shaders& s, sf::Window& w) : mesh(f), win(&w) {
        m_loc=glGetUniformLocation(s.program, "model");
        vp_loc=glGetUniformLocation(s.program, "vp");
        light_pos_loc=glGetUniformLocation(s.program, "light_pos");
        is_sun_loc=glGetUniformLocation(s.program, "is_sun");
        tex_loc=glGetUniformLocation(s.program, "tex_sampler");

        tex_sole=caricaTexture("../Risorse/sole.jpg");
        tex_terra=caricaTexture("../Risorse/terra.jpg");
        tex_luna=caricaTexture("../Risorse/luna.jpg");
        
        mesh_base_norm=scaling(1.0f/mesh.extent)*translation(-mesh.center);
        cam.view_projection();
    }

    void draw(float simulated_time) {
        cam.aspect=(float)win->getSize().x/(float)win->getSize().y;
        cam.view_projection();
        glUniformMatrix4fv(vp_loc, 1, GL_FALSE, &cam.vp[0][0]);
        
        glUniform3f(light_pos_loc, 0.0f, 0.0f, 0.0f);
        glUniform1i(tex_loc, 0);
        glActiveTexture(GL_TEXTURE0);

        //disegnosole
        glm::mat4 pos_sole=translation(0.0f, 0.0f, 0.0f);
        glm::mat4 mm_sole=pos_sole*rotation_y(simulated_time*20.0f)*scaling(1.0f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_sole[0][0]);
        glUniform1i(is_sun_loc, 1);
        glBindTexture(GL_TEXTURE_2D, tex_sole);
        mesh.draw();

        //disegnoterra
        glm::mat4 centro_terra=pos_sole*rotation_y(simulated_time*40.0f)*translation(3.5f, 0.0f, 0.0f); 
        glm::mat4 mm_terra=centro_terra*rotation_y(simulated_time*90.0f)*scaling(0.3f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_terra[0][0]);
        glUniform1i(is_sun_loc, 0); 
        glBindTexture(GL_TEXTURE_2D, tex_terra);
        mesh.draw();

        //disegnoluna
        glm::mat4 centro_luna=centro_terra*rotation_y(simulated_time*120.0f)*translation(0.8f, 0.0f, 0.0f);
        glm::mat4 mm_luna=centro_luna*scaling(0.1f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_luna[0][0]);
        glUniform1i(is_sun_loc, 0); 
        glBindTexture(GL_TEXTURE_2D, tex_luna);
        mesh.draw();
    }
};

//maincongestioneinputdatastiera
int main(int argc, char** argv) {
    generaSferaOFF(); 
    
    Setup s;
    Shaders sh;
    
    const char* vs= 
        "#version 410 core\n"
        "layout(location=0) in vec3 pos;\n"
        "layout(location=1) in vec3 norm;\n"
        "uniform mat4 model;\n"
        "uniform mat4 vp;\n"
        "out vec3 v_pos;\n"
        "out vec3 v_norm;\n"
        "out vec3 v_local_pos;\n"
        "void main() {\n"
        "    gl_Position=vp*model*vec4(pos, 1.0);\n"
        "    v_pos=vec3(model*vec4(pos, 1.0));\n"
        "    v_norm=mat3(transpose(inverse(model)))*norm;\n"
        "    v_local_pos=pos;\n"
        "}";

    const char* fs= 
        "#version 410 core\n"
        "uniform sampler2D tex_sampler;\n"
        "uniform vec3 light_pos;\n"
        "uniform int is_sun;\n"
        "in vec3 v_pos;\n"
        "in vec3 v_norm;\n"
        "in vec3 v_local_pos;\n"
        "out vec4 c;\n"
        "void main() {\n"
        "    vec3 p=normalize(v_local_pos);\n"
        "    float u=0.5+atan(p.z, p.x)/(2.0*3.14159265);\n"
        "    float v=0.5-asin(p.y)/3.14159265;\n"
        "    vec4 texColor=texture(tex_sampler, vec2(u, v));\n"
        "    if(is_sun==1) {\n"
        "        c=texColor;\n" 
        "    } else {\n"
        "        float ambientStrength=0.05;\n" 
        "        vec3 ambient=ambientStrength*texColor.rgb;\n"
        "        vec3 n=-normalize(v_norm);\n" 
        "        vec3 lightDir=normalize(light_pos-v_pos);\n"
        "        float diff=max(dot(n, lightDir), 0.0);\n" 
        "        vec3 diffuse=diff*texColor.rgb;\n"
        "        c=vec4(ambient+diffuse, 1.0);\n"
        "    }\n"
        "}";
        
    sh.compile_attach_link(&vs, &fs);
    sh.use();
    
    Scene sc("sfera.off", sh, *s.window);

    glEnable(GL_DEPTH_TEST);
    sf::Clock clk;
    
    //variabiliperinterattivitadeltempo
    float simulated_time=0.0f;
    float time_scale=1.0f;
    bool is_paused=false;

    std::cout<<"\n========================================\n";
    cout_controls:
    std::cout<<" COMANDI TASTIERA ATTIVI:\n";
    std::cout<<" [BARRA SPAZIO] : Pausa / Riprendi\n";
    std::cout<<" [FRECCIA SU]   : Aumenta velocità\n";
    std::cout<<" [FRECCIA GIU'] : Rallenta / Inverti\n";
    std::cout<<"========================================\n\n";

    while(s.window->isOpen()) {
        float dt=clk.restart().asSeconds();
        
        //gestioneeventitastiera
        while(const std::optional e=s.window->pollEvent()) {
            if(e->is<sf::Event::Closed>()) {
                return 0;
            }
            if(const auto* r=e->getIf<sf::Event::Resized>()) {
                glViewport(0, 0, r->size.x, r->size.y);
            }
            
            //mouserotazionecamera
            if(const auto* m=e->getIf<sf::Event::MouseMoved>()) {
                static float px=m->position.x, py=m->position.y;
                if(sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    sc.cam.drag(m->position.x-px, m->position.y-py);
                }
                px=m->position.x;
                py=m->position.y;
            }

            //tastipremuti
            if(const auto* k=e->getIf<sf::Event::KeyPressed>()) {
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

        //aggiornamentotemposimulato
        if(!is_paused) {
            simulated_time+=dt*time_scale;
        }

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        sc.draw(simulated_time);
        
        s.window->display();
    }
    return 0;
}