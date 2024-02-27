#pragma once
#include "Scene.h"
#include "gui/Mouse.h"
#include <glm/detail/compute_vector_relational.hpp>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <utility>

class InteractiveScene : public Scene {
 private:
  enum Focus {
    NONE,
    SCENE,
    GUI
  };


 public:
  // Constructor

  /** Interactive Scene constructor */
  InteractiveScene(const std::string &title, const int &width = 800, const int &height = 600, const int &context_ver_maj = 3, const int &context_ver_min = 3);

  
};
