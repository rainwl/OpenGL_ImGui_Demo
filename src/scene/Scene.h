#pragma once
#include "../glad/glad.h"

#include <string>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
class Scene {
protected:
  GLFWwindow *window;
public:
  Scene(const std::string &title, const int &width = 800, const int &height = 600, const int &context_ver_maj = 3, const int &context_ver_min = 3);

  bool IsValid();
};
