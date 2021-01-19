#include <GLApp.h>
#include <Mat4.h>

#include "Flowfield.h"

class MyGLApp : public GLApp {
public:
  size_t particleCount{1000};
  double angle{0};
  double lastAnimationTime{0};
  std::vector<Vec3> particlePos;
  std::vector<float> data;
  Flowfield flow = Flowfield::genDemo(64, DemoType::SATTLE);
  
  virtual void init() override {
    glEnv.setTitle("Flow Vis Demo 1 (Particle Tracing)");
    GL(glDisable(GL_CULL_FACE));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glClearColor(0,0,0,0));
    initParticles();
  }

  void initParticles() {
    particlePos.resize(particleCount);
    for (size_t i = 0;i<particlePos.size();++i) {
      particlePos[i] = Vec3::random();
    }
    data.resize(particlePos.size()*7);
  }
  
  void advect(double animationTime) {
    const double deltaT = animationTime - lastAnimationTime;
    lastAnimationTime = animationTime;
    
    for (size_t i = 0;i<particlePos.size();++i) {
      particlePos[i] = advect(particlePos[i], deltaT);
    }
  }

  Vec3 advect(const Vec3& particle, double deltaT) {
    if (particle.x() < 0.0 || particle.x() > 1.0 ||
        particle.y() < 0.0 || particle.y() > 1.0 ||
        particle.z() < 0.0 || particle.z() > 1.0) {
      return Vec3::random();
    }
    return particle + flow.interpolate(particle) * float(deltaT);
  }

  void particlePosToRenderData() {
    for (size_t i = 0;i<particlePos.size();++i) {
      data[i*7+0] = particlePos[i].x()*2-1;
      data[i*7+1] = particlePos[i].y()*2-1;
      data[i*7+2] = particlePos[i].z()*2-1;
      
      data[i*7+3] = particlePos[i].x();
      data[i*7+4] = particlePos[i].y();
      data[i*7+5] = particlePos[i].z();
      data[i*7+6] = 1.0f;
    }
  }
  
  virtual void animate(double animationTime) override {
    angle = animationTime;
    advect(animationTime*10);
    particlePosToRenderData();
  }
  
  virtual void draw() override {
    GL(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
    setDrawProjection(Mat4::perspective(45, glEnv.getFramebufferSize().aspect(), 0.0001f, 100));
    setDrawTransform(Mat4::rotationY(float(angle)) * Mat4::rotationX(float(angle/2.0)) * Mat4::lookAt({0,0,5},{0,0,0},{0,1,0}));
    
    drawPoints(data, 4, false);
  }
  
  virtual void keyboard(int key, int scancode, int action, int mods) override {

    if (action == GLFW_PRESS) {
      switch (key) {
        case GLFW_KEY_ESCAPE:
          closeWindow();
          break;
        case GLFW_KEY_I:
          initParticles();
          break;
      }
    }
  }


} myApp;

int main(int argc, char ** argv) {
  myApp.run();
  return EXIT_SUCCESS;
}  
