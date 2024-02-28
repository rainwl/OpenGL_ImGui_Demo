#include "interactivescene.hpp"

#include "customwidgets.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <iostream>

#include <ecal/ecal.h>
#include <ecal/msg/protobuf/publisher.h>
#include <fusion.pb.h>


#include <cstring>
#include <random>


// Private static const attributes

// Available texures
const std::map<Material::Attribute,std::string> InteractiveScene::AVAILABLE_TEXTURE = {
    {Material::AMBIENT, "Ambient"},
    {Material::DIFFUSE, "Diffuse"},
    {Material::SPECULAR, "Specular"},
    {Material::SHININESS, "Shininess"},
    {Material::NORMAL, "Normal"},
    {Material::DISPLACEMENT, "Displacement"}
};

//Types of lights labels
const char *InteractiveScene::LIGHT_TYPE_LABEL[] = {"Directional", "Point", "Spotlight"};


// Private static attributes

// Repository URL
char InteractiveScene::repository_url[] = "https://github.com/Rebaya17/objviewer/";


// Private statics methods

// GLFW framebuffer size callback
void InteractiveScene::framebufferSizeCallback(GLFWwindow *window, int width, int height) {
  // Execute the scene framebuffer size callback
  Scene::framebufferSizeCallback(window, width, height);

  // Resize the mouse resolution
  static_cast<InteractiveScene *>(glfwGetWindowUserPointer(window))->mouse->setResolution(width, height);
}

// GLFW mouse button callback
void InteractiveScene::mouseButtonCallback(GLFWwindow *window, int, int action, int) {
  // Get the ImGuiIO reference and the capture IO status
  ImGuiIO &io = ImGui::GetIO();
  const bool capture_io = io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput;

  // Disable the mouse if release a mouse button and the GUI don't want to capture IO
  if (!capture_io && (action == GLFW_RELEASE)) { static_cast<InteractiveScene *>(glfwGetWindowUserPointer(window))->setCursorEnabled(false); }
}

// GLFW cursor callback
void InteractiveScene::cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
  // Get the interactive scene
  InteractiveScene *const scene = static_cast<InteractiveScene *>(glfwGetWindowUserPointer(window));
  scene->cursor_position.x = static_cast<float>(xpos);
  scene->cursor_position.y = static_cast<float>(ypos);

  // Get the ImGuiIO reference and the capture IO status
  ImGuiIO &io = ImGui::GetIO();
  const bool capture_io = io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput;

  // If the cursor is disabled and the GUI don't want to capture IO or the main GUI window is not visible
  if ((io.ConfigFlags & ImGuiConfigFlags_NoMouse) && (!capture_io || !scene->show_main_gui)) {
    // Rotate the active camera
    scene->active_camera->rotate(scene->mouse->translate(xpos, ypos));

    // Update the grabbed lights direction
    const glm::vec3 direction = scene->active_camera->getDirection();
    for (std::pair<const std::size_t,Light *> &light_data : scene->light_stock) { if (light_data.second->isGrabbed()) { light_data.second->setDirection(direction); } }
  }
}

// GLFW scroll callback
void InteractiveScene::scrollCallback(GLFWwindow *window, double, double yoffset) {
  // Get the ImGuiIO reference and the capture IO status
  ImGuiIO &io = ImGui::GetIO();
  const bool capture_io = io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput;

  // Update the active camera zoom if the GUI don't want to capture IO
  if (!capture_io) { static_cast<InteractiveScene *>(glfwGetWindowUserPointer(window))->active_camera->zoom(yoffset); }
}

// GLFW key callback
void InteractiveScene::keyCallback(GLFWwindow *window, int key, int, int action, int modifier) {
  // Get the pressed status
  bool pressed = action != GLFW_RELEASE;

  // Get the interactive scene
  InteractiveScene *const scene = static_cast<InteractiveScene *>(glfwGetWindowUserPointer(window));

  // Get the ImGuiIO reference and the capture IO status
  ImGuiIO &io = ImGui::GetIO();
  const bool capture_io = io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput;

  switch (key) {
    // Toggle the main GUI window visibility
    case GLFW_KEY_ESCAPE: if (pressed) {
        scene->show_main_gui = !capture_io || !scene->show_main_gui;
        scene->setCursorEnabled(scene->show_main_gui);
      }
      return;

    // Toggle the about window
    case GLFW_KEY_F1: if (pressed) { scene->setAboutVisible(!scene->show_about); }
      return;

    // Toggle the about ImGui window
    case GLFW_KEY_F11: if (pressed) { scene->setAboutImGuiVisible(!scene->show_about_imgui); }
      return;

    // Toggle the metrics window
    case GLFW_KEY_F12: if (pressed) { scene->setMetricsVisible(!scene->show_metrics); }
      return;

    // Toggle the camera boost
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT: Camera::setBoosted(pressed);
      return;

    // Reload shaders
    case GLFW_KEY_R: if (pressed && (modifier == GLFW_MOD_CONTROL)) { scene->reloadPrograms(); }
  }
}


// Private methods

// Draw the GUI
void InteractiveScene::drawGUI() {
  // Check the visibility of all windows
  if (!show_main_gui && !show_metrics && !show_about && !show_about_imgui) { return; }

  // New ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Show the main GUI window
  if (show_main_gui) { showMainGUIWindow(); }

  // Show the about window
  if (show_about) { InteractiveScene::showAboutWindow(show_about); }

  // Show the metrics built-in window
  if (show_metrics) { ImGui::ShowMetricsWindow(&show_metrics); }

  // Show the about ImGui built-in window
  if (show_about_imgui) { ImGui::ShowAboutWindow(&show_about_imgui); }

  // Update focus
  switch (focus) {
    // Set focus to the most front window
    case InteractiveScene::GUI: ImGui::SetWindowFocus();
      focus = InteractiveScene::NONE;
      break;

    // Set focus to the scene
    case InteractiveScene::SCENE: ImGui::SetWindowFocus(nullptr);
      focus = InteractiveScene::NONE;
      break;

    // Nothing for none
    case InteractiveScene::NONE: break;
  }

  // Render the GUI
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void InteractiveScene::publishData() {
#pragma region Random
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis_0_10(0.0, 10.0);
  std::uniform_real_distribution<float> dis_330_360(330.0, 360.0);
  std::uniform_real_distribution<float> dis_10_15(10.0, 15.0);
  std::uniform_real_distribution<float> dis_0_1(0.0, 1.0);
#pragma endregion
#pragma region mutable_ set_
  // fusion_data_.mutable_endoscope_pos()->set_x(0);
  // fusion_data_.mutable_endoscope_pos()->set_y(0);
  // fusion_data_.mutable_endoscope_pos()->set_z(0);
  //
  // fusion_data_.mutable_endoscope_euler()->set_x(0);
  // fusion_data_.mutable_endoscope_euler()->set_y(0);
  // fusion_data_.mutable_endoscope_euler()->set_z(0);

  // fusion_data_.mutable_tube_pos()->set_x(dis_0_10(gen));
  // fusion_data_.mutable_tube_pos()->set_y(dis_0_10(gen));
  // fusion_data_.mutable_tube_pos()->set_z(dis_0_10(gen));
  //
  // fusion_data_.mutable_tube_euler()->set_x(dis_330_360(gen));
  // fusion_data_.mutable_tube_euler()->set_y(dis_10_15(gen));
  // fusion_data_.mutable_tube_euler()->set_x(dis_10_15(gen));

  fusion_data_.mutable_offset()->set_endoscope_offset(-1);
  fusion_data_.mutable_offset()->set_tube_offset(-3);
  fusion_data_.mutable_offset()->set_instrument_switch(60);
  //fusion_data_.mutable_offset()->set_animation_value(dis_0_1(gen));
  fusion_data_.mutable_offset()->set_pivot_offset(2);

  fusion_data_.mutable_rot_coord()->set_x(0);
  fusion_data_.mutable_rot_coord()->set_y(0.7071068f);
  fusion_data_.mutable_rot_coord()->set_z(0);
  fusion_data_.mutable_rot_coord()->set_w(0.7071068f);

  fusion_data_.mutable_pivot_pos()->set_x(-10);
  fusion_data_.mutable_pivot_pos()->set_y(4.9f);
  fusion_data_.mutable_pivot_pos()->set_z(-0.9f);

  fusion_data_.set_ablation_count(0);

  fusion_data_.mutable_haptic()->set_haptic_state(3);
  fusion_data_.mutable_haptic()->set_haptic_offset(-1);
  fusion_data_.mutable_haptic()->set_haptic_force(2);

  fusion_data_.set_hemostasis_count(0);
  fusion_data_.set_hemostasis_index(0);

  fusion_data_.mutable_soft_tissue()->set_liga_flavum(1);
  fusion_data_.mutable_soft_tissue()->set_disc_yellow_space(1);
  fusion_data_.mutable_soft_tissue()->set_veutro_vessel(1);
  fusion_data_.mutable_soft_tissue()->set_fat(1);
  fusion_data_.mutable_soft_tissue()->set_fibrous_rings(1);
  fusion_data_.mutable_soft_tissue()->set_nucleus_pulposus(1);
  fusion_data_.mutable_soft_tissue()->set_p_longitudinal_liga(1);
  fusion_data_.mutable_soft_tissue()->set_dura_mater(1);
  fusion_data_.mutable_soft_tissue()->set_nerve_root(1);

  fusion_data_.set_nerve_root_dance(0);

  // fusion_data_.mutable_rongeur_pos()->set_x(dis_0_10(gen));
  // fusion_data_.mutable_rongeur_pos()->set_y(dis_0_10(gen));
  // fusion_data_.mutable_rongeur_pos()->set_z(dis_0_10(gen));

  // fusion_data_.mutable_rongeur_rot()->set_x(dis_330_360(gen));
  // fusion_data_.mutable_rongeur_rot()->set_y(dis_10_15(gen));
  // fusion_data_.mutable_rongeur_rot()->set_z(dis_10_15(gen));
#pragma endregion
  const int data_size = fusion_data_.ByteSizeLong();// NOLINT(bugprone-narrowing-conversions, clang-diagnostic-shorten-64-to-32, cppcoreguidelines-narrowing-conversions)
  const auto data = std::make_unique<uint8_t[]>(data_size);
  fusion_data_.SerializePartialToArray(data.get(), data_size);
  const int code = publisher_->Send(data.get(), data_size);// NOLINT(clang-diagnostic-shorten-64-to-32, bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
  if (code != data_size) { std::cout << "failure\n"; }
}


// Draw the main window GUI
void InteractiveScene::showMainGUIWindow() {
  // Setup style
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0F);
  ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width/3), static_cast<float>(height - 40)), ImGuiCond_Always);

  // Create the main GUI window
  // const bool open = ImGui::Begin("Settings", &show_main_gui, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
  const bool open = ImGui::Begin("Debug", &show_main_gui, ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoBackground);
  ImGui::PopStyleVar();

  // Abort if the window is not showing
  if (!show_main_gui || !open) {
    ImGui::End();
    return;
  }
  
  std::size_t remove = 0U;

  // Draw each model node
  for (std::pair<const std::size_t,std::pair<Model *,std::size_t>> &program_data : model_stock) {
    // ID and title strings
    const std::string id = std::to_string(program_data.first);
    // const std::string program_title = "Model " + id + ": " + program_data.second.first->getName();
    const std::string program_title = program_data.second.first->getName();

    // Draw node and catch the selected to remove
    if (ImGui::TreeNodeEx(id.c_str(),ImGuiTreeNodeFlags_DefaultOpen, "%s", program_title.c_str())) {
      if (!modelWidget(program_data.second)) { remove = program_data.first; }
      ImGui::TreePop();
    }
  }
  ImGui::End();
}


// Draw the camera widget
bool InteractiveScene::cameraWidget(Camera *const camera, const std::size_t &id) {
  // Keep camera flag
  bool keep = true;

  // For non active camera
  if (id != 0U) {
    // Select active button
    bool active = camera == active_camera;
    if (ImGui::Checkbox("Active", &active)) { active_camera = camera; }
    // Remove button
    if (camera_stock.size() > 1U) { keep = !ImGui::RemoveButton(); }
  }

  // Projection
  bool orthogonal = camera->isOrthogonal();
  if (ImGui::RadioButton("Perspective", !orthogonal)) { camera->setOrthogonal(false); }
  ImGui::SameLine();
  if (ImGui::RadioButton("Orthogonal", orthogonal)) { camera->setOrthogonal(true); }
  ImGui::SameLine(338.0F);
  ImGui::Text("Projection");

  // Position
  glm::vec3 value = camera->getPosition();
  if (ImGui::DragFloat3("Position", &value.x, 0.01F, 0.0F, 0.0F, "%.4f")) { camera->setPosition(value); }

  // Direction
  value = camera->getDirection();
  if (ImGui::DragFloat3("Direction", &value.x, 0.01F, 0.0F, 0.0F, "%.4f")) { camera->setDirection(value); }

  // Clipping planes
  glm::vec2 clipping = camera->getClipping();
  if (ImGui::DragFloat2("Clipping", &clipping.x, 0.01F, 0.0F, 0.0F, "%.4f")) { camera->setClipping(clipping); }
  ImGui::HelpMarker("(Near, Far)");

  // Field of view
  float fov = camera->getFOV();
  if (ImGui::DragFloat("FOV", &fov, 0.01F, 0.0F, 0.0F, "%.4f")) { camera->setFOV(fov); }

  // Separator for open nodes
  ImGui::Separator();

  // Return the keep camera value
  return keep;
}

// Draw the model widget
bool InteractiveScene::modelWidget(std::pair<Model *,std::size_t> &model_data) {
  // Keep model flag
  bool keep = true;

  // Get the model and program ID
  Model *const model = model_data.first;
  const std::size_t program = model_data.second;

  // Position
  glm::vec3 value = model->Model::getPosition();
  ImGui::Text("Pos");
  ImGui::SameLine();
  if (ImGui::DragFloat3("##Pos", &value.x, 0.01F, 0.0F, 0.0F, "%.2f")) { model->Model::setPosition(value); }

  // Rotation
  value = model->Model::getRotationAngles();
  ImGui::Text("Rot");
  ImGui::SameLine();
  if (ImGui::DragFloat3("##Rot", &value.x, 0.50F, 0.0F, 0.0F, "%.2f")) { model->Model::setRotation(value); }
  // ImGui::HelpMarker("Angles in degrees");

  // Scale
  value = model->Model::getScale();
  ImGui::Text("Sca");
  ImGui::SameLine();
  if (ImGui::DragFloat3("##Sca", &value.x, 0.01F, 0.0F, 0.0F, "%.2f")) { model->setScale(value); }
  
  // Separator for open nodes
  // ImGui::Separator();

  // Return the keep model value
  return keep;
}

// Draw the light widget
bool InteractiveScene::lightWidget(Light *const light) {
  // Keep light flag
  bool keep = true;

  // Type combo
  Light::Type type = light->getType();
  if (ImGui::BeginCombo("Type", InteractiveScene::LIGHT_TYPE_LABEL[type])) {
    for (GLint i = Light::DIRECTIONAL; i <= Light::SPOTLIGHT; i++) {
      // Selected status
      const Light::Type new_type = static_cast<Light::Type>(i);
      bool selected = type == new_type;

      // Show item and select
      if (ImGui::Selectable(InteractiveScene::LIGHT_TYPE_LABEL[new_type], selected)) { light->setType(new_type); }

      // Set default focus to selected
      if (selected) { ImGui::SetItemDefaultFocus(); }
    }
    ImGui::EndCombo();
  }

  // Enabled checkbox
  bool status = light->isEnabled();
  if (ImGui::Checkbox("Enabled", &status)) { light->setEnabled(status); }
  // Grabed checkbox for spotlight lights
  if (type == Light::SPOTLIGHT) {
    status = light->isGrabbed();
    ImGui::SameLine();
    if (ImGui::Checkbox("Grabbed", &status)) { light->setGrabbed(status); }
  }
  // Remove button if there are more than one light in the stock
  if (light_stock.size() > 1U) { keep = !ImGui::RemoveButton(); }

  // Space attributes
  ImGui::BulletText("Spacial attributes");
  ImGui::Indent();

  // Light direction for non point light
  if (type != Light::POINT) {
    glm::vec3 direction = light->getDirection();
    if (ImGui::DragFloat3("Direction", &direction.x, 0.01F, 0.0F, 0.0F, "%.4f")) { light->setDirection(direction); }
  }

  // Non directional lights
  if (type != Light::DIRECTIONAL) {
    // Position
    glm::vec3 vector = light->getPosition();
    if (ImGui::DragFloat3("Position", &vector.x, 0.01F, 0.0F, 0.0F, "%.4f")) { light->setPosition(vector); }

    // Attenuation
    vector = light->getAttenuation();
    if (ImGui::DragFloat3("Attenuation", &vector.x, 0.01F, 0.0F, 0.0F, "%.4f")) { light->setAttenuation(vector); }
    ImGui::HelpMarker("[Constant, Linear, Quadratic]\nIf any value is negative rare\neffects may appear.");

    // Cutoff for spotlight lights
    if (type == Light::SPOTLIGHT) {
      glm::vec2 cutoff = light->getCutoff();
      if (ImGui::DragFloat2("Cutoff", &cutoff.x, 0.01F, 0.0F, 0.0F, "%.4f")) { light->setCutoff(cutoff); }
      ImGui::HelpMarker("[Inner, Outter]\nIf the inner cutoff is greater than the\noutter cutoff rare effects may appear.");
    }
  }
  ImGui::Unindent();

  // Color attributes
  ImGui::BulletText("Color attributes");
  ImGui::Indent();

  // Ambient color
  glm::vec3 color = light->getAmbientColor();
  if (ImGui::ColorEdit3("Ambient", &color.r)) { light->setAmbientColor(color); }

  // Diffuse color
  color = light->getDiffuseColor();
  if (ImGui::ColorEdit3("Diffuse", &color.r)) { light->setDiffuseColor(color); }

  // Specular color
  color = light->getSpecularColor();
  if (ImGui::ColorEdit3("Specular", &color.r)) { light->setSpecularColor(color); }
  ImGui::Unindent();

  // Colors levels
  ImGui::BulletText("Color values");
  ImGui::Indent();

  // Ambient level
  float value = light->getAmbientLevel();
  if (ImGui::DragFloat("Ambient level", &value, 0.0025F, 0.0F, 1.0F, "%.4f")) { light->setAmbientLevel(value); }

  // Diffuse level
  value = light->getDiffuseLevel();
  if (ImGui::DragFloat("Diffuse level", &value, 0.0025F, 0.0F, 1.0F, "%.4f")) { light->setDiffuseLevel(value); }

  // Specular level
  value = light->getSpecularLevel();
  if (ImGui::DragFloat("Specular level", &value, 0.0025F, 0.0F, 1.0F, "%.4f")) { light->setSpecularLevel(value); }

  // Shininess
  value = light->getShininess();
  if (ImGui::DragFloat("Shininess", &value, 0.0025F, 0.0F, 0.0F, "%.4f")) { light->setShininess(value); }
  ImGui::HelpMarker("If the shininess value negative\nrare effects may appear.");
  ImGui::Unindent();

  // Return the keep light value
  return keep;
}

// Draw the program widget
bool InteractiveScene::programWidget(std::pair<GLSLProgram *,std::string> &program_data) {
  // Keep program flag
  bool keep = true;

  // Get the program and shader paths
  GLSLProgram *const program = program_data.first;
  std::string vert = program->getShaderPath(GL_VERTEX_SHADER);
  std::string geom = program->getShaderPath(GL_GEOMETRY_SHADER);
  std::string frag = program->getShaderPath(GL_FRAGMENT_SHADER);

  // Default program flag
  const bool default_program = (program == program_stock[0U].first) || (program == program_stock[1U].first);

  // Program description
  std::string description = program_data.second;
  if (ImGui::InputText("Description", &description, ImGuiInputTextFlags_EnterReturnsTrue)) { program_data.second = description; }
  // Reload button
  if (ImGui::Button("Reload")) { program->link(); }
  // Remove button for non default program
  if (!default_program) { keep = !ImGui::RemoveButton(); }
  // Check the valid status
  if (!program->isValid()) { ImGui::TextColored(ImVec4(0.80F, 0.16F, 0.16F, 1.00F), "Could not link the program"); }

  // Shaders sources paths
  ImGui::BulletText("Shaders");

  // Vertex shader source path
  bool link = ImGui::InputText("Vertex", &vert, ImGuiInputTextFlags_EnterReturnsTrue);

  // Geometry shader source path for non default programs
  if (!default_program) { link |= ImGui::InputText("Geometry", &geom, ImGuiInputTextFlags_EnterReturnsTrue); }

  // Fragment shader source path
  link |= ImGui::InputText("Fragment", &frag, ImGuiInputTextFlags_EnterReturnsTrue);

  // Relink if a path has been modified
  if (link) {
    // Program without geometry shader
    if (geom.empty()) { program->link(vert, frag); }
    // Program with geometry shader
    else { program->link(vert, geom, frag); }
  }

  // Separator for open nodes
  ImGui::Separator();

  // Return the keep program value
  return keep;
}

// Draw a program combo item
bool InteractiveScene::programComboItem(const std::size_t &current, const std::size_t &program) {
  // Selected status
  bool selected = current == program;

  // Get the program title and append the ID for non default programs
  std::string program_title = program_stock[program].second;
  if ((program != 0U) && (program != 1U)) { program_title.append(" (").append(std::to_string(program)).append(")"); }

  // Show item and get the selection status
  const bool selection = ImGui::Selectable(program_title.c_str(), selected);

  // Set default focus to selected
  if (selected) { ImGui::SetItemDefaultFocus(); }

  // Return the selection
  return selection;
}

bool use_rongeur = false;
bool use_tube = false;
bool use_endoscope = false;
// Process keyboard input
void InteractiveScene::processKeyboardInput() {
  // Return if the GUI wants the IO and is visible
  ImGuiIO &io = ImGui::GetIO();
  if ((io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput) && show_main_gui) { return; }

  // Camera movement
  if (glfwGetKey(window, GLFW_KEY_W)) active_camera->travell(Camera::FRONT);
  if (glfwGetKey(window, GLFW_KEY_S)) active_camera->travell(Camera::BACK);
  if (glfwGetKey(window, GLFW_KEY_A) || glfwGetKey(window, GLFW_KEY_LEFT)) active_camera->travell(Camera::LEFT);
  if (glfwGetKey(window, GLFW_KEY_D) || glfwGetKey(window, GLFW_KEY_RIGHT)) active_camera->travell(Camera::RIGHT);
  if (glfwGetKey(window, GLFW_KEY_Q) || glfwGetKey(window, GLFW_KEY_UP)) active_camera->travell(Camera::UP);
  if (glfwGetKey(window, GLFW_KEY_E) || glfwGetKey(window, GLFW_KEY_DOWN)) active_camera->travell(Camera::DOWN);

  if (glfwGetKey(window,GLFW_KEY_1)) {
    use_rongeur = true;
    use_tube = false;
    use_endoscope = false;
  }
  if (glfwGetKey(window,GLFW_KEY_2)) {
    use_rongeur = false;
    use_tube = true;
    use_endoscope = false;
  }
  if (glfwGetKey(window,GLFW_KEY_3)) {
    use_rongeur = false;
    use_tube = false;
    use_endoscope = true;
  }
  if (use_tube) {
#pragma region tube
    Model *model = Scene::getModel(10);// 11 rongeur_upper
    const glm::vec3 gl_pos = model->getPosition();
    glm::vec3 gl_rot = model->getRotationAngles();

    if (glfwGetKey(window,GLFW_KEY_K)) {
      model->setPosition((glm::vec3(gl_pos.x + 0.01f, 0, 0)));
      pos_x_t += 0.4f;
      fusion_data_.mutable_tube_pos()->set_x(pos_x_t);
    }
    if (glfwGetKey(window,GLFW_KEY_I)) {
      model->setPosition((glm::vec3(gl_pos.x - 0.01f, 0, 0)));
      pos_x_t -= 0.4f;
      fusion_data_.mutable_tube_pos()->set_x(pos_x_t);
    }
    if (glfwGetKey(window,GLFW_KEY_L)) {
      model->setPosition((glm::vec3(gl_pos.z + 0.01f, 0, 0)));
      pos_z_t += 0.4f;
      fusion_data_.mutable_tube_pos()->set_z(pos_z_t);
    }
    if (glfwGetKey(window,GLFW_KEY_J)) {
      model->setPosition((glm::vec3(gl_pos.z - 0.01f, 0, 0)));
      pos_z_t -= 0.4f;
      fusion_data_.mutable_tube_pos()->set_z(pos_z_t);
    }
    if (glfwGetKey(window,GLFW_KEY_U)) {
      model->setPosition((glm::vec3(gl_pos.y + 0.01f, 0, 0)));
      pos_y_t += 0.4f;
      fusion_data_.mutable_tube_pos()->set_y(pos_y_t);
    }
    if (glfwGetKey(window,GLFW_KEY_O)) {
      model->setPosition((glm::vec3(gl_pos.y - 0.01f, 0, 0)));
      pos_y_t -= 0.4f;
      fusion_data_.mutable_tube_pos()->set_y(pos_y_t);
    }

    if (glfwGetKey(window,GLFW_KEY_G)) {
      model->setRotation(glm::vec3(gl_rot.x + 0.01f, 0, 0));
      rot_x_t += 1;
      fusion_data_.mutable_tube_euler()->set_x(rot_x_t);
    }
    if (glfwGetKey(window,GLFW_KEY_B)) {
      model->setRotation(glm::vec3(gl_rot.x - 0.01f, 0, 0));
      rot_x_t -= 1;
      fusion_data_.mutable_tube_euler()->set_x(rot_x_t);
    }
    if (glfwGetKey(window,GLFW_KEY_N)) {
      model->setRotation(glm::vec3(gl_rot.y + 0.01f, 0, 0));
      rot_y_t += 1;
      fusion_data_.mutable_tube_euler()->set_y(rot_y_t);
    }
    if (glfwGetKey(window,GLFW_KEY_V)) {
      model->setRotation(glm::vec3(gl_rot.y - 0.01f, 0, 0));
      rot_y_t -= 1;
      fusion_data_.mutable_tube_euler()->set_y(rot_y_t);
    }
    if (glfwGetKey(window,GLFW_KEY_F)) {
      model->setRotation(glm::vec3(gl_rot.z + 0.01f, 0, 0));
      rot_z_t += 1;
      fusion_data_.mutable_tube_euler()->set_z(rot_z_t);
    }
    if (glfwGetKey(window,GLFW_KEY_H)) {
      model->setRotation(glm::vec3(gl_rot.z - 0.01f, 0, 0));
      rot_z_t -= 1;
      fusion_data_.mutable_tube_euler()->set_z(rot_z_t);
    }

    if (glfwGetKey(window,GLFW_KEY_Z)) { fusion_data_.mutable_offset()->set_animation_value(0); }
    if (glfwGetKey(window,GLFW_KEY_X)) { fusion_data_.mutable_offset()->set_animation_value(1); }
#pragma endregion
  }
  if (use_rongeur) {
#pragma region rongeur
    Model *model = Scene::getModel(11);// 11 rongeur_upper
    const glm::vec3 gl_pos = model->getPosition();
    glm::vec3 gl_rot = model->getRotationAngles();

    if (glfwGetKey(window,GLFW_KEY_K)) {
      model->setPosition((glm::vec3(gl_pos.x + 0.01f, 0, 0)));
      pos_x += 0.4f;
      fusion_data_.mutable_rongeur_pos()->set_x(pos_x);
    }
    if (glfwGetKey(window,GLFW_KEY_I)) {
      model->setPosition((glm::vec3(gl_pos.x - 0.01f, 0, 0)));
      pos_x -= 0.4f;
      fusion_data_.mutable_rongeur_pos()->set_x(pos_x);
    }
    if (glfwGetKey(window,GLFW_KEY_L)) {
      model->setPosition((glm::vec3(gl_pos.z + 0.01f, 0, 0)));
      pos_z += 0.4f;
      fusion_data_.mutable_rongeur_pos()->set_z(pos_z);
    }
    if (glfwGetKey(window,GLFW_KEY_J)) {
      model->setPosition((glm::vec3(gl_pos.z - 0.01f, 0, 0)));
      pos_z -= 0.4f;
      fusion_data_.mutable_rongeur_pos()->set_z(pos_z);
    }
    if (glfwGetKey(window,GLFW_KEY_U)) {
      model->setPosition((glm::vec3(gl_pos.y + 0.01f, 0, 0)));
      pos_y += 0.4f;
      fusion_data_.mutable_rongeur_pos()->set_y(pos_y);
    }
    if (glfwGetKey(window,GLFW_KEY_O)) {
      model->setPosition((glm::vec3(gl_pos.y - 0.01f, 0, 0)));
      pos_y -= 0.4f;
      fusion_data_.mutable_rongeur_pos()->set_y(pos_y);
    }

    if (glfwGetKey(window,GLFW_KEY_N)) {
      model->setRotation(glm::vec3(gl_rot.x + 0.01f, 0, 0));
      rot_x += 1;
      fusion_data_.mutable_rongeur_rot()->set_x(rot_x);
    }
    if (glfwGetKey(window,GLFW_KEY_V)) {
      model->setRotation(glm::vec3(gl_rot.x - 0.01f, 0, 0));
      rot_x -= 1;
      fusion_data_.mutable_rongeur_rot()->set_x(rot_x);
    }
    if (glfwGetKey(window,GLFW_KEY_G)) {
      model->setRotation(glm::vec3(gl_rot.y + 0.01f, 0, 0));
      rot_y += 1;
      fusion_data_.mutable_rongeur_rot()->set_y(rot_y);
    }
    if (glfwGetKey(window,GLFW_KEY_B)) {
      model->setRotation(glm::vec3(gl_rot.y - 0.01f, 0, 0));
      rot_y -= 1;
      fusion_data_.mutable_rongeur_rot()->set_y(rot_y);
    }
    if (glfwGetKey(window,GLFW_KEY_F)) {
      model->setRotation(glm::vec3(gl_rot.z + 0.01f, 0, 0));
      rot_z += 1;
      fusion_data_.mutable_rongeur_rot()->set_z(rot_z);
    }
    if (glfwGetKey(window,GLFW_KEY_H)) {
      model->setRotation(glm::vec3(gl_rot.z - 0.01f, 0, 0));
      rot_z -= 1;
      fusion_data_.mutable_rongeur_rot()->set_z(rot_z);
    }
    if (glfwGetKey(window,GLFW_KEY_Z)) { fusion_data_.mutable_offset()->set_animation_value(0); }
    if (glfwGetKey(window,GLFW_KEY_X)) { fusion_data_.mutable_offset()->set_animation_value(1); }
#pragma endregion
  }
  if (use_endoscope) {
    Model *model = Scene::getModel(9);// 11 rongeur_upper
    const glm::vec3 gl_pos = model->getPosition();
    glm::vec3 gl_rot = model->getRotationAngles();

    if (glfwGetKey(window,GLFW_KEY_K)) {
      model->setPosition((glm::vec3(gl_pos.x + 0.01f, 0, 0)));
      pos_x_e += 0.4f;
      fusion_data_.mutable_endoscope_pos()->set_x(pos_x_e);
    }
    if (glfwGetKey(window,GLFW_KEY_I)) {
      model->setPosition((glm::vec3(gl_pos.x - 0.01f, 0, 0)));
      pos_x_e -= 0.4f;
      fusion_data_.mutable_endoscope_pos()->set_x(pos_x_e);
    }
    if (glfwGetKey(window,GLFW_KEY_L)) {
      model->setPosition((glm::vec3(gl_pos.z + 0.01f, 0, 0)));
      pos_z_e += 0.4f;
      fusion_data_.mutable_endoscope_pos()->set_z(pos_z_e);
    }
    if (glfwGetKey(window,GLFW_KEY_J)) {
      model->setPosition((glm::vec3(gl_pos.z - 0.01f, 0, 0)));
      pos_z_e -= 0.4f;
      fusion_data_.mutable_endoscope_pos()->set_z(pos_z_e);
    }
    if (glfwGetKey(window,GLFW_KEY_U)) {
      model->setPosition((glm::vec3(gl_pos.y + 0.01f, 0, 0)));
      pos_y_e += 0.4f;
      fusion_data_.mutable_endoscope_pos()->set_y(pos_y_e);
    }
    if (glfwGetKey(window,GLFW_KEY_O)) {
      model->setPosition((glm::vec3(gl_pos.y - 0.01f, 0, 0)));
      pos_y_e -= 0.4f;
      fusion_data_.mutable_endoscope_pos()->set_y(pos_y_e);
    }

    if (glfwGetKey(window,GLFW_KEY_G)) {
      model->setRotation(glm::vec3(gl_rot.x + 0.01f, 0, 0));
      rot_x_e += 1;
      fusion_data_.mutable_endoscope_euler()->set_x(rot_x_e);
    }
    if (glfwGetKey(window,GLFW_KEY_B)) {
      model->setRotation(glm::vec3(gl_rot.x - 0.01f, 0, 0));
      rot_x_e -= 1;
      fusion_data_.mutable_endoscope_euler()->set_x(rot_x_e);
    }
    if (glfwGetKey(window,GLFW_KEY_N)) {
      model->setRotation(glm::vec3(gl_rot.y + 0.01f, 0, 0));
      rot_y_e += 1;
      fusion_data_.mutable_endoscope_euler()->set_y(rot_y_e);
    }
    if (glfwGetKey(window,GLFW_KEY_V)) {
      model->setRotation(glm::vec3(gl_rot.y - 0.01f, 0, 0));
      rot_y_e -= 1;
      fusion_data_.mutable_endoscope_euler()->set_y(rot_y_e);
    }
    if (glfwGetKey(window,GLFW_KEY_F)) {
      model->setRotation(glm::vec3(gl_rot.z + 0.01f, 0, 0));
      rot_z_e += 1;
      fusion_data_.mutable_endoscope_euler()->set_z(rot_z_e);
    }
    if (glfwGetKey(window,GLFW_KEY_H)) {
      model->setRotation(glm::vec3(gl_rot.z - 0.01f, 0, 0));
      rot_z_e -= 1;
      fusion_data_.mutable_endoscope_euler()->set_z(rot_z_e);
    }

    if (glfwGetKey(window,GLFW_KEY_Z)) { fusion_data_.mutable_offset()->set_animation_value(0); }
    if (glfwGetKey(window,GLFW_KEY_X)) { fusion_data_.mutable_offset()->set_animation_value(1); }
  }

  // Update the grabbed lights positions
  const glm::vec3 position = active_camera->getPosition();
  for (std::pair<const std::size_t,Light *> &light_data : light_stock) { if (light_data.second->isGrabbed()) { light_data.second->setPosition(position); } }
}


// Constructor

// Interactive scene constructor
InteractiveScene::InteractiveScene(const std::string &title, const int &width, const int &height, const int &context_ver_maj, const int &context_ver_min)
  :
  // Scene
  Scene(title, width, height, context_ver_maj, context_ver_min),

  // Mouse
  mouse(new Mouse(width, height)),

  // Cursor
  cursor_enabled(true),
  cursor_position(0.0F),

  // Focus
  focus(InteractiveScene::GUI),

  // Show default GUI windows
  show_main_gui(true),
  show_metrics(false),
  show_about(false),
  show_about_imgui(false),

  // Focus on GUI by default
  focus_gui(true) {
  // Load the GUI if is the first instance
  if ((Scene::instances == 1U) && Scene::initialized_glad) {
    // Set the user pointer to this scene and setup callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, InteractiveScene::framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, InteractiveScene::mouseButtonCallback);
    glfwSetCursorPosCallback(window, InteractiveScene::cursorPosCallback);
    glfwSetScrollCallback(window, InteractiveScene::scrollCallback);
    glfwSetKeyCallback(window, InteractiveScene::keyCallback);

    // Setup the ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup platform and render
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Setup key navigation and disable ini file
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("JetBrainsMono-Regular.ttf", 20, nullptr, io.Fonts->GetGlyphRangesChineseFull());

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = NULL;
#pragma region ImGui Style
    // Setup the global style
    ImGui::StyleColorsLight();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 12;
    style.ChildRounding = 12;
    style.FrameRounding = 12;
    style.PopupRounding = 12;
    style.ScrollbarRounding = 12;
    style.GrabRounding = 12;
    style.TabRounding = 12;

    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.66f, 0.66f, 0.66f, 0.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.47f, 0.47f, 0.47f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.79f, 0.79f, 0.79f, 0.67f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.65f, 0.65f, 0.65f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.66f, 0.66f, 0.66f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.60f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.25f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.36f, 0.36f, 0.36f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.74f, 0.74f, 0.74f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.64f, 0.64f, 0.64f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.24f, 0.24f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.71f, 0.71f, 0.71f, 0.35f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.52f, 0.52f, 0.52f, 0.54f);
    colors[ImGuiCol_Header] = ImVec4(0.67f, 0.67f, 0.67f, 0.31f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.64f, 1.00f, 0.85f, 0.95f);

    // ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0F);
    // ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 2.0F);
#pragma endregion
  }

  // Render the GUI before anything
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  // Init eCAL
  eCAL::Initialize(1, nullptr, "Fusion Publisher");
  eCAL::Process::SetState(proc_sev_healthy, proc_sev_level1, "healthy");
  // const eCAL::CPublisher publisher("fusion");
  publisher_ = new eCAL::CPublisher("fusion");

}


// Getters

// Get the main GUI visible status
bool InteractiveScene::isMainGUIVisible() const { return show_main_gui; }

// Get the metrics window visible status
bool InteractiveScene::isMetricsVisible() const { return show_metrics; }

// Get the about window visible status
bool InteractiveScene::isAboutVisible() const { return show_about; }

// Get the about ImGui window visible status
bool InteractiveScene::isAboutImGuiVisible() const { return show_about_imgui; }


// Get the cursor enabled status
bool InteractiveScene::isCursorEnabled() const { return ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouse; }

// Get the mouse
Mouse *InteractiveScene::getMouse() const { return mouse; }


// Setters

// Set the main GUI visible status
void InteractiveScene::setMainGUIVisible(const bool &status) {
  show_main_gui = status;
  focus = InteractiveScene::GUI;
}

// Set the metrics window visible status
void InteractiveScene::setMetricsVisible(const bool &status) {
  // Update the visible status
  show_metrics = status;

  // Keep focus to scene if is not showing the main GUI window
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouse) { focus = InteractiveScene::SCENE; }
}

// Set the about window visible status
void InteractiveScene::setAboutVisible(const bool &status) {
  // Update the visible status
  show_about = status;

  // Keep focus to scene if is not showing the main GUI window
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouse) { focus = InteractiveScene::SCENE; }
}

// Set the about ImGui window visible status
void InteractiveScene::setAboutImGuiVisible(const bool &status) {
  // Update the visible status
  show_about_imgui = status;

  // Keep focus to scene if is not showing the main GUI window
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouse) { focus = InteractiveScene::SCENE; }
}

// Set the mouse enabled status
void InteractiveScene::setCursorEnabled(const bool &status) {
  // Enable cursor
  if (status) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    focus = InteractiveScene::GUI;
  }

  // Disable cursor
  else {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
    focus = InteractiveScene::SCENE;

    // Update position
    double xpos;
    double ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mouse->setTranslationPoint(xpos, ypos);
  }
}


// Methods

/** Render main loop */
void InteractiveScene::mainLoop() {
  // Check the window status
  if (window == nullptr) {
    std::cerr << "error: there is no window" << std::endl;
    return;
  }

  // Check the default geometry pass valid status
  if (!program_stock[0U].first->isValid()) { std::cerr << "warning: the default geometry pass program has not been set or is not valid" << std::endl; }

  // Check the default lighting pass valid status
  if (!program_stock[1U].first->isValid()) { std::cerr << "warning: the default lighting pass program has not been set or is not valid" << std::endl; }

  // The rendering main loop
  while (glfwWindowShouldClose(window) == GLFW_FALSE) {
    // Clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the scene and GUI
    drawScene();
    drawGUI();
    publishData();
    // Process the keyboard input
    processKeyboardInput();

    // Poll events and swap buffers
    glfwPollEvents();
    glfwSwapBuffers(window);

    // Count frame
    kframes += 0.001;
  }
}


// Destructor

// Interactive scene destructor
InteractiveScene::~InteractiveScene() {
  // Delete the mouse
  delete mouse;

  // Terminate GUI if is the last instance
  if ((Scene::instances == 1U) && Scene::initialized_glad) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }
}


// Public static methods

// Draw the about window GUI
void InteractiveScene::showAboutWindow(bool &show) {
  // Creates the about window and abort if is not showing
  if (!ImGui::Begin("About OBJViewer", &show, ImGuiWindowFlags_NoResize)) {
    ImGui::End();
    return;
  }

  // Title
  ImGui::Text("OBJViewer - Another OBJ models viewer");
  ImGui::Separator();

  // Signature
  ImGui::Text("By Erick Rincones 2019.");
  ImGui::Text("OBJViewer is licensed under the MIT License, see LICENSE for more information.");
  ImGui::Spacing();

  // Repository
  ImGui::Text("GitHub repository:");
  ImGui::HelpMarker("Click to select all and press\nCTRL+V to copy to clipboard");

  ImGui::PushItemWidth(-1.0F);
  ImGui::InputText("###repourl", InteractiveScene::repository_url, sizeof(InteractiveScene::repository_url), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
  ImGui::PopItemWidth();

  // End window
  ImGui::End();
}
