#include "scene/gui/interactivescene.hpp"
#include "dirsep.h"
#include <glm/vec3.hpp>
#include <iostream>

int main(const int argc, char **argv) {
#pragma region init
  for (int i = 0; i < argc; i++) { std::cout << "argv[" << i << "]: " << argv[i] << std::endl; }

  // Create the scene and check it
  auto *scene = new InteractiveScene("PhysicalSimulatedServer");

  // Exit with error if the scene is not valid
  if (!scene->isValid()) {
    delete scene;
    return 1;
  }

  // Set the background color
  scene->setBackgroundColor(glm::vec3(0.9f, 0.9f, 0.9f));

  // Setup directories
  const std::string bin_path = argv[0];
  const std::string relative = bin_path.substr(0U, bin_path.find_last_of(DIR_SEP) + 1U);

  const std::string model_path = relative + ".." + DIR_SEP + "model" + DIR_SEP;
  const std::string shader_path = relative + ".." + DIR_SEP + "shader" + DIR_SEP;

  // Add the programs
  const std::string common_lp_path = shader_path + "lp_common.vert.glsl";
  scene->setDefaultGeometryPassProgram("[GP] Basic shading", shader_path + "gp_basic.vert.glsl", shader_path + "gp_basic.frag.glsl");
  scene->setDefaultLightingPassProgram("[LP] Normals", common_lp_path, shader_path + "lp_normals.frag.glsl");

  const std::string gp_normal_vert_path = shader_path + "gp_normal.vert.glsl";
  std::size_t normal = scene->addProgram("[GP] Normal mapping", gp_normal_vert_path, shader_path + "gp_normal.frag.glsl");
  std::size_t parallax = scene->addProgram("[GP] Parallax mapping", gp_normal_vert_path, shader_path + "gp_parallax.frag.glsl");

  scene->addProgram("[LP] Positions", common_lp_path, shader_path + "lp_positions.frag.glsl");
  scene->addProgram("[LP] Blinn-Phong", common_lp_path, shader_path + "lp_blinn_phong.frag.glsl");
  scene->addProgram("[LP] Oren-Nayar", common_lp_path, shader_path + "lp_oren_nayar.frag.glsl");
  std::size_t lp_program = scene->addProgram("[LP] Cock-Torrance", common_lp_path, shader_path + "lp_cock_torrance.frag.glsl");

  scene->setLightingPassProgram(lp_program);
#pragma endregion

#pragma region Add models
  const std::size_t endoscope = scene->addModel(model_path + "endoscope" + DIR_SEP + "endoscope.obj");
  const std::size_t tube = scene->addModel(model_path + "tube" + DIR_SEP + "tubeC.obj");
  const std::size_t rongeur_up_ = scene->addModel(model_path + "rongeur_upper" + DIR_SEP + "upper.obj");
  const std::size_t rongeur_low = scene->addModel(model_path + "rongeur_lower" + DIR_SEP + "lower.obj");
    
  // Setup models
  Model *model = scene->getModel(endoscope);

  model = scene->getModel(endoscope);
  model->setScale(glm::vec3(1));
  model->setPosition((glm::vec3(0, 0, 0)));
  
  model = scene->getModel(tube);
  model->setScale(glm::vec3(0.2f));
  model->setPosition((glm::vec3(0, 0, 0)));

  model = scene->getModel(rongeur_up_);
  model->setScale(glm::vec3(0.1f));
  model->setPosition((glm::vec3(0, 0, 0)));

  model = scene->getModel(rongeur_low);
  model->setScale(glm::vec3(0.1f));
  model->setPosition((glm::vec3(0, 0, 0)));

#pragma endregion
  
  scene->mainLoop();
  
  delete scene;
  return 0;
}
