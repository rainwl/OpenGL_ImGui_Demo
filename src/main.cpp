#include "PCH.h"
#include "scene/InteractiveScene.h"
#include <glm/vec3.hpp>
#include <iostream>

int main(const int argc, char **argv) {
  auto *scene = new InteractiveScene("SpineSimServer");
  if (!scene->IsValid()) {
    delete scene;
    return 1;
  }
}