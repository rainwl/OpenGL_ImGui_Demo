#include "PCH.h"
#include "Scene.h"

Scene::Scene(const std::string &title, const int &width, const int &height, const int &context_ver_maj, const int &context_ver_min)
  :window(nullptr)
{
  
}
bool Scene::IsValid() { return window != nullptr; }
