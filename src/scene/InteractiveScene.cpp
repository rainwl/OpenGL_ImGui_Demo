#include "PCH.h"
#include "InteractiveScene.h"

InteractiveScene::InteractiveScene(const std::string &title, const int &width, const int &height, const int &context_ver_maj, const int &context_ver_min)
  : Scene(title, width, height, context_ver_maj, context_ver_min) {}
