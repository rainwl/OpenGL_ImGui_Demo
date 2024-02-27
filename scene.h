#pragma once
#include "Camera.h"

#include "../glad/glad.h"
#include <GLFW/glfw3.h>

class scene {
protected:
  GLFWwindow *window;
public:
  bool IsValid();
};
