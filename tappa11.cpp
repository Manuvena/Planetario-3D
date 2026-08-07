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
        //texturebiancadisicurezza
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

//classeorbite
class GPUOrbit {
public:
    GLuint vao, vbo;
    int point_count;
    GPUOrbit(float radius, int segments=100) {
        std::vector<float> vertices;
        for(int i=0; i<=segments; ++i) {
            float theta=2.0f*3.14159265359f*i/segments;
            vertices.push_back(radius*std::cos(theta));
            vertices.push_back(0.0f);
            vertices.push_back(radius*std::sin(theta));
        }
        point_count=vertices.size()/3;
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
        glDrawArrays(GL_LINE_STRIP, 0, point_count);
    }
};

//classestelle
class GPUStars {
public:
    GLuint vao, vbo;
    int star_count;
    GPUStars(int count=600) {
        std::vector<float> vertices;
        for(int i=0; i<count; ++i) {
            float theta=static_cast<float>(rand())/RAND_MAX*2.0f*3.14159265359f;
            float phi=static_cast<float>(rand())/RAND_MAX*3.14159265359f-3.14159265359f/2.0f;
            //sfondolontano
            float r=40.0f+static_cast<float>(rand())/RAND_MAX*20.0f; 
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
        //rendestellebenvisibili
        glPointSize(2.0f); 
        glDrawArrays(GL_POINTS, 0, star_count);
    }
};

//setupfinestra
class Setup {
public:
    sf::Window* window;
    Setup() {
        sf::ContextSettings settings;
        settings.depthBits=32;
        settings.stencilBits=8;
        settings.antiAliasingLevel=4;
        settings.attributeFlags=sf::ContextSettings::Attribute::Core;
        settings.majorVersion=4;
        settings.minorVersion=1;
        window=new sf::Window(sf::VideoMode({800, 800}), "Planetario Tappa 11 - Sistema Solare Completo!", sf::Style::Default, sf::State::Windowed, settings);
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
    float phi=30.0f, theta=30.0f, fd=2.0f, od=20.0f;
    
    void drag(float dx, float dy) {
        phi+=dx*0.5f;
        theta=std::clamp(theta+dy*0.5f, -89.0f, 89.0f);
        view_projection();
    }
    
    void view_projection() {
        glm::mat4 ry=rotation_y(phi);
        glm::mat4 rx=rotation_x(theta);
        glm::mat4 tz=translation(0, 0, -od);
        float ncp=std::max(0.1f, od-15.0f);
        float fcp=120.0f;
        float a=(fcp+ncp)/(ncp-fcp);
        float b=2.0f*fcp*ncp/(ncp-fcp);
        glm::mat4 pr=glm::mat4(fd/aspect, 0, 0, 0, 0, fd, 0, 0, 0, 0, a, -1.0, 0, 0, b, 0);
        v=tz*rx*ry;
        vp=pr*v;
    }
};

//gpumesh
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

//scenapianetiorbitestelle
class Scene {
public:
    Camera cam; 
    GPUMesh mesh; 
    GPUStars stars;
    GPUOrbit orb_mercurio, orb_venere, orb_terra, orb_marte, orb_giove, orb_luna;
    glm::mat4 mesh_base_norm;
    sf::Window* win;
    GLint m_loc, vp_loc, light_pos_loc, is_sun_loc, tex_loc, color_loc, is_orbit_loc, is_star_loc;
    GLuint t_sole, t_mercurio, t_venere, t_terra, t_luna, t_marte, t_giove;

    Scene(std::string f, Shaders& s, sf::Window& w) 
        : mesh(f), stars(800),
          orb_mercurio(1.8f), orb_venere(2.8f), orb_terra(4.0f), orb_marte(5.5f), orb_giove(7.5f), orb_luna(0.8f),
          win(&w) {
        
        m_loc=glGetUniformLocation(s.program, "model");
        vp_loc=glGetUniformLocation(s.program, "vp");
        light_pos_loc=glGetUniformLocation(s.program, "light_pos");
        is_sun_loc=glGetUniformLocation(s.program, "is_sun");
        tex_loc=glGetUniformLocation(s.program, "tex_sampler");
        color_loc=glGetUniformLocation(s.program, "color_obj");
        is_orbit_loc=glGetUniformLocation(s.program, "is_orbit");
        is_star_loc=glGetUniformLocation(s.program, "is_star");

        t_sole=caricaTexture("sole.jpg");
        t_mercurio=caricaTexture("mercurio.jpeg");
        t_venere=caricaTexture("venere.jpg");
        t_terra=caricaTexture("terra.jpg");
        t_luna=caricaTexture("luna.jpg");
        t_marte=caricaTexture("marte.jpg");
        t_giove=caricaTexture("giove.jpg");
        
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

        glm::mat4 pos_sole=translation(0.0f, 0.0f, 0.0f);

        //disegnostellesfondo
        glUniform1i(is_star_loc, 1);
        glm::mat4 mm_stars=translation(0.0f, 0.0f, 0.0f);
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_stars[0][0]);
        stars.draw();
        glUniform1i(is_star_loc, 0);

        //disegnoorbite
        glUniform1i(is_orbit_loc, 1);
        //grigioblutenue
        glUniform4f(color_loc, 0.25f, 0.25f, 0.35f, 1.0f);

        //orbitamercurio
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_mercurio.draw();
        
        //orbitavenere
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_venere.draw();
        
        //orbitaterra
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_terra.draw();
        
        //orbitamarte
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_marte.draw();
        
        //orbitagiove
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &pos_sole[0][0]);
        orb_giove.draw();

        //orbitaluna
        glm::mat4 c_terra=pos_sole*rotation_y(simulated_time*30.0f)*translation(4.0f, 0.0f, 0.0f);
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &c_terra[0][0]); 
        orb_luna.draw();

        glUniform1i(is_orbit_loc, 0);

        //sole
        glm::mat4 mm_sole=pos_sole*rotation_y(simulated_time*10.0f)*scaling(1.2f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_sole[0][0]);
        glUniform1i(is_sun_loc, 1);
        glBindTexture(GL_TEXTURE_2D, t_sole);
        mesh.draw();

        //mercurio
        glm::mat4 mm_merc=pos_sole*rotation_y(simulated_time*80.0f)*translation(1.8f, 0.0f, 0.0f)*rotation_y(simulated_time*50.0f)*scaling(0.12f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_merc[0][0]);
        glUniform1i(is_sun_loc, 0);
        glBindTexture(GL_TEXTURE_2D, t_mercurio);
        mesh.draw();

        //venere
        glm::mat4 mm_ven=pos_sole*rotation_y(simulated_time*50.0f)*translation(2.8f, 0.0f, 0.0f)*rotation_y(simulated_time*30.0f)*scaling(0.22f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_ven[0][0]);
        glUniform1i(is_sun_loc, 0);
        glBindTexture(GL_TEXTURE_2D, t_venere);
        mesh.draw();

        //terra
        glm::mat4 mm_terra=c_terra*rotation_y(simulated_time*90.0f)*scaling(0.3f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_terra[0][0]);
        glUniform1i(is_sun_loc, 0); 
        glBindTexture(GL_TEXTURE_2D, t_terra);
        mesh.draw();

        //luna
        glm::mat4 mm_luna=c_terra*rotation_y(simulated_time*120.0f)*translation(0.8f, 0.0f, 0.0f)*scaling(0.09f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_luna[0][0]);
        glUniform1i(is_sun_loc, 0); 
        glBindTexture(GL_TEXTURE_2D, t_luna);
        mesh.draw();

        //marte
        glm::mat4 mm_marte=pos_sole*rotation_y(simulated_time*24.0f)*translation(5.5f, 0.0f, 0.0f)*rotation_y(simulated_time*70.0f)*scaling(0.2f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_marte[0][0]);
        glUniform1i(is_sun_loc, 0);
        glBindTexture(GL_TEXTURE_2D, t_marte);
        mesh.draw();

        //giove
        glm::mat4 mm_giove=pos_sole*rotation_y(simulated_time*10.0f)*translation(7.5f, 0.0f, 0.0f)*rotation_y(simulated_time*150.0f)*scaling(0.7f)*mesh_base_norm;
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &mm_giove[0][0]);
        glUniform1i(is_sun_loc, 0);
        glBindTexture(GL_TEXTURE_2D, t_giove);
        mesh.draw();
    }
};

//main
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
        "uniform int is_star;\n"
        "out vec3 v_pos;\n"
        "out vec3 v_norm;\n"
        "out vec3 v_local_pos;\n"
        "void main() {\n"
        "    gl_Position=vp*model*vec4(pos, 1.0);\n"
        "    if(is_star==1) {\n"
        "       gl_PointSize=2.0;\n"
        "    }\n"
        "    v_pos=vec3(model*vec4(pos, 1.0));\n"
        "    v_norm=mat3(transpose(inverse(model)))*norm;\n"
        "    v_local_pos=pos;\n"
        "}";

    const char* fs= 
        "#version 410 core\n"
        "uniform sampler2D tex_sampler;\n"
        "uniform vec3 light_pos;\n"
        "uniform int is_sun;\n"
        "uniform int is_orbit;\n"
        "uniform int is_star;\n"
        "uniform vec4 color_obj;\n"
        "in vec3 v_pos;\n"
        "in vec3 v_norm;\n"
        "in vec3 v_local_pos;\n"
        "out vec4 c;\n"
        "void main() {\n"
        "    if(is_star==1) {\n"
        "        //stellebiancogialleluminose\n"
        "        c=vec4(1.0, 1.0, 0.9, 1.0);\n"
        "    } else if(is_orbit==1) {\n"
        "        c=color_obj;\n"
        "    } else {\n"
        "        vec3 p=normalize(v_local_pos);\n"
        "        float u=0.5+atan(p.z, p.x)/(2.0*3.14159265);\n"
        "        float v=0.5-asin(p.y)/3.14159265;\n"
        "        vec4 texColor=texture(tex_sampler, vec2(u, v));\n"
        "        if(is_sun==1) {\n"
        "            c=texColor;\n"
        "        } else {\n"
        "            float ambientStrength=0.05;\n"
        "            vec3 ambient=ambientStrength*texColor.rgb;\n"
        "            vec3 n=-normalize(v_norm);\n"
        "            vec3 lightDir=normalize(light_pos-v_pos);\n"
        "            float diff=max(dot(n, lightDir), 0.0);\n"
        "            vec3 diffuse=diff*texColor.rgb;\n"
        "            c=vec4(ambient+diffuse, 1.0);\n"
        "        }\n"
        "    }\n"
        "}";
        
    sh.compile_attach_link(&vs, &fs);
    sh.use();
    
    Scene sc("sfera.off", sh, *s.window);

    glEnable(GL_DEPTH_TEST);
    glEnable(0x8642);
    sf::Clock clk;
    
    float simulated_time=0.0f;
    float time_scale=1.0f;
    bool is_paused=false;

    std::cout<<"\n[tappa 11] sistema solare completo e stellato avviato!\n";

    while(s.window->isOpen()) {
        float dt=clk.restart().asSeconds();
        
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

            if(const auto* k=e->getIf<sf::Event::KeyPressed>()) {
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
            simulated_time+=dt*time_scale;
        }

        //sfondospazioprofondo
        glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        sc.draw(simulated_time);
        
        s.window->display();
    }
    return 0;
}