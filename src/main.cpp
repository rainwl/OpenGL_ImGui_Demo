#include "PCH.h"
#include "Glad/glad.h"
#include "GLFW//glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "ImGui/imgui_internal.h"

#include "Camera.h"
#include "Model.h"
#include "Shader.h"
#include "mygui.h"

#include "ecal/ecal.h"
#include "ecal/msg/protobuf/publisher.h"
#include "fusion.pb.h"

#include "stb_image.h"

#pragma region Callback

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double x_pos_in, double y_pos_in);

void scroll_callback(GLFWwindow *window, double x_offset, double y_offset);

void process_input(GLFWwindow *window);

void UpdateModelTransform(Model *model, const glm::vec3 &pivot_pos, const glm::vec3 &dynamic_pos);

glm::vec3 GetCurrentModelZAxis(const glm::vec3 &rotation);

#pragma endregion

#pragma region Settings
// settings
constexpr unsigned int scr_width = 1600;
constexpr unsigned int scr_height = 1200;

// camera
Camera camera(glm::vec3(-240.0f, 0.0f, 0.0f));
float last_x = scr_width / 2.0f;
float last_y = scr_height / 2.0f;
bool first_mouse = true;

// timing
float delta_time = 0.0f;
float last_frame = 0.0f;

int count{0};
#pragma endregion

#pragma region Data
std::vector<float> data;
pb::FusionData::FusionData fusion_data;

Model *tube;
Model *endoscope;
Model *upper;
Model *lower;
Model *axis;
Model *x;
Model *y;
Model *z;
Model *pivot;
Model *dynamic;

glm::vec3 pivot_pos(-100.0f, 49.0f, -9.0f);

glm::vec3 dynamic_pos(0.0f, 0.0f, 0.0f);

#pragma endregion

int main()
{
#pragma region Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(scr_width, scr_height, "PhysicalSimulatedServer", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    // NOLINT(clang-diagnostic-cast-function-type-strict)
    {
        std::cout << "Failed to initialize GLAD" << '\n';
        return -1;
    }
#pragma endregion

#pragma region Init shader
    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);
    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    // build and compile shaders
    Shader shader("./Shader/shader.vs", "./Shader/shader.fs");
#pragma endregion

#pragma region Init models
    // load models
    Model our_model("./resources/objects/backpack/backpack.obj");
    Model bone("./resources/objects/backpack/test_bone_45.obj");

    tube = new Model("./resources/objects/backpack/tubeC.obj");
    endoscope = new Model("./resources/objects/backpack/endoscope.obj");
    upper = new Model("./resources/objects/backpack/upper.obj");
    lower = new Model("./resources/objects/backpack/lower.obj");

    axis = new Model("./resources/objects/backpack/axis.obj");
    x = new Model("./resources/objects/backpack/x.obj");
    y = new Model("./resources/objects/backpack/y.obj");
    z = new Model("./resources/objects/backpack/z.obj");
    pivot = new Model("./resources/objects/backpack/axis.obj");
    dynamic = new Model("./resources/objects/backpack/axis.obj");

    our_model.SetPosition(glm::vec3(1000, 1000, 1000));
    dynamic->SetPosition(dynamic_pos);
    pivot->SetPosition(pivot_pos);

    upper->SetPosition(pivot_pos);
    lower->SetPosition(pivot_pos);
    tube->SetPosition(pivot_pos);
    endoscope->SetPosition(pivot_pos);

    // draw in wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

#pragma endregion

#pragma region Init texture
    GLuint fbo, texture;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, scr_width, scr_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << '\n';
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(nullptr);
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.Fonts->AddFontFromFileTTF("JetBrainsMono-Regular.ttf", 36, nullptr, io.Fonts->GetGlyphRangesChineseFull());

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiViewportFlags_NoDecoration;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiCol_DockingEmptyBg;
    // ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 12;
    style.ChildRounding = 12;
    style.FrameRounding = 12;
    style.PopupRounding = 6;
    style.ScrollbarRounding = 8;
    style.GrabRounding = 12;
    style.TabRounding = 8;

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
    colors[ImGuiCol_DockingPreview] = ImVec4(0.49f, 0.49f, 0.49f, 0.70f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.71f, 0.71f, 0.71f, 0.35f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.52f, 0.52f, 0.52f, 0.54f);
    colors[ImGuiCol_Header] = ImVec4(0.67f, 0.67f, 0.67f, 0.31f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.64f, 1.00f, 0.85f, 0.95f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
#pragma endregion

#pragma region Init eCAL
    eCAL::Initialize(1, nullptr, "Fusion Publisher");
    eCAL::Process::SetState(proc_sev_healthy, proc_sev_level1, "healthy");
    const eCAL::CPublisher publisher("fusion");

#pragma endregion

    while (!glfwWindowShouldClose(window))
    {
#pragma region Init
        const auto current_frame = static_cast<float>(glfwGetTime());
        delta_time = current_frame - last_frame;
        last_frame = current_frame;
        process_input(window);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.7137f, 0.7333f, 0.7686f, 1.0f); // rgb(182, 187, 196)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#pragma endregion

#pragma region MVP
        shader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.cam_zoom),
                                                static_cast<float>(scr_width) / static_cast<float>(scr_height), 0.1f,
                                                1000.0f);
        glm::mat4 view = camera.get_view_matrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // render the loaded model
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f)); // it's a bit too big for our scene, so scale it down
        shader.setMat4("model", model);

#pragma endregion

#pragma region Model

        /////////////////////////////////////////////////////////////////////
        our_model.Draw(shader);
        axis->Draw(shader);
        x->Draw(shader);
        y->Draw(shader);
        z->Draw(shader);
        pivot->Draw(shader);
        bone.Draw(shader);
        /////////////////////////////////////////////////////////////////////
        dynamic->SetPosition(dynamic_pos);
        UpdateModelTransform(tube, pivot_pos, dynamic_pos);
        UpdateModelTransform(endoscope, pivot_pos, dynamic_pos);
        UpdateModelTransform(upper, pivot_pos, dynamic_pos);
        UpdateModelTransform(lower, pivot_pos, dynamic_pos);
        /////////////////////////////////////////////////////////////////////
        dynamic->Draw(shader);
        tube->Draw(shader);
        endoscope->Draw(shader);
        upper->Draw(shader);
        lower->Draw(shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

#pragma endregion

#pragma region ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();

        draw_gui();

        ImGui::Begin("Scene");
        ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(texture)), ImVec2{scr_width, scr_height},
                     ImVec2{0, 1}, ImVec2{1, 0}); // NOLINT(performance-no-int-to-ptr)
        ImGui::End();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#pragma endregion

#pragma region mutable_ set_
        // fusion_data.mutable_endoscope_pos()->set_x(dis_0_10(gen));
        // fusion_data.mutable_endoscope_pos()->set_y(dis_0_10(gen));
        // fusion_data.mutable_endoscope_pos()->set_z(dis_0_10(gen));
        //
        // fusion_data.mutable_endoscope_euler()->set_x(dis_330_360(gen));
        // fusion_data.mutable_endoscope_euler()->set_y(dis_10_15(gen));
        // fusion_data.mutable_endoscope_euler()->set_z(dis_10_15(gen));
        //
        // fusion_data.mutable_tube_pos()->set_x(dis_0_10(gen));
        // fusion_data.mutable_tube_pos()->set_y(dis_0_10(gen));
        // fusion_data.mutable_tube_pos()->set_z(dis_0_10(gen));
        //
        // fusion_data.mutable_tube_euler()->set_x(dis_330_360(gen));
        // fusion_data.mutable_tube_euler()->set_y(dis_10_15(gen));
        // fusion_data.mutable_tube_euler()->set_x(dis_10_15(gen));

        fusion_data.mutable_offset()->set_endoscope_offset(-1);
        fusion_data.mutable_offset()->set_tube_offset(-3);
        fusion_data.mutable_offset()->set_instrument_switch(60);
        // fusion_data.mutable_offset()->set_animation_value(dis_0_1(gen));
        fusion_data.mutable_offset()->set_animation_value(0);
        fusion_data.mutable_offset()->set_pivot_offset(2);

        fusion_data.mutable_rot_coord()->set_x(0);
        fusion_data.mutable_rot_coord()->set_y(0.7071068f);
        fusion_data.mutable_rot_coord()->set_z(0);
        fusion_data.mutable_rot_coord()->set_w(0.7071068f);

        fusion_data.mutable_pivot_pos()->set_x(-10);
        fusion_data.mutable_pivot_pos()->set_y(4.9f);
        fusion_data.mutable_pivot_pos()->set_z(-0.9f);

        fusion_data.set_ablation_count(0);

        fusion_data.mutable_haptic()->set_haptic_state(3);
        fusion_data.mutable_haptic()->set_haptic_offset(-1);
        fusion_data.mutable_haptic()->set_haptic_force(2);

        fusion_data.set_hemostasis_count(0);
        fusion_data.set_hemostasis_index(0);

        fusion_data.mutable_soft_tissue()->set_liga_flavum(1);
        fusion_data.mutable_soft_tissue()->set_disc_yellow_space(1);
        fusion_data.mutable_soft_tissue()->set_veutro_vessel(1);
        fusion_data.mutable_soft_tissue()->set_fat(1);
        fusion_data.mutable_soft_tissue()->set_fibrous_rings(1);
        fusion_data.mutable_soft_tissue()->set_nucleus_pulposus(1);
        fusion_data.mutable_soft_tissue()->set_p_longitudinal_liga(1);
        fusion_data.mutable_soft_tissue()->set_dura_mater(1);
        fusion_data.mutable_soft_tissue()->set_nerve_root(1);

        fusion_data.set_nerve_root_dance(0);

        // fusion_data.mutable_rongeur_pos()->set_x(dis_0_10(gen));
        // fusion_data.mutable_rongeur_pos()->set_y(dis_0_10(gen));
        // fusion_data.mutable_rongeur_pos()->set_z(dis_0_10(gen));
        //
        // fusion_data.mutable_rongeur_rot()->set_x(dis_330_360(gen));
        // fusion_data.mutable_rongeur_rot()->set_y(dis_10_15(gen));
        // fusion_data.mutable_rongeur_rot()->set_z(dis_10_15(gen));
#pragma endregion

#pragma region eCAL pub send

        const int data_size = fusion_data.ByteSizeLong();
        // NOLINT(clang-diagnostic-shorten-64-to-32, bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
        auto data = std::make_unique<uint8_t[]>(data_size); // NOLINT(clang-diagnostic-shadow)
        fusion_data.SerializePartialToArray(data.get(), data_size);

        const int code = publisher.Send(data.get(), data_size);
        // NOLINT(clang-diagnostic-shorten-64-to-32, bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
        if (code != data_size) { std::cout << "failure\n"; }

#pragma endregion

#pragma region End
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
#pragma endregion
    }

#pragma region Finalize
    delete tube;
    glfwTerminate();
    eCAL::Finalize();
    return 0;
#pragma endregion
}

glm::vec3 GetCurrentModelZAxis(const glm::vec3 &rotation)
{
    auto rot_matrix = glm::mat4(1.0f);
    rot_matrix = glm::rotate(rot_matrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    rot_matrix = glm::rotate(rot_matrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    rot_matrix = glm::rotate(rot_matrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));

    const auto z_axis_local = glm::vec4(0, 0, 1, 0);
    const glm::vec3 z_axis_world = rot_matrix * z_axis_local;
    return glm::normalize(z_axis_world);
}

void UpdateModelTransform(Model *model, const glm::vec3 &pivot_pos, const glm::vec3 &dynamic_pos)
{
    const glm::vec3 current_dir = GetCurrentModelZAxis(model->rotation);
    const glm::vec3 target_dir = glm::normalize(pivot_pos - dynamic_pos);

    // Calculates the quaternion of rotation between two directions
    const float dot_product = glm::dot(current_dir, target_dir);
    float angle = acos(glm::clamp(dot_product, -1.0f, 1.0f));

    if (dot_product < -0.9999f)
    {
        // Almost in the opposite direction
        angle = glm::pi<float>(); // 180°
    }

    glm::vec3 rotation_axis = glm::cross(current_dir, target_dir);

    if (glm::length(rotation_axis) < 0.001f)
    {
        // If the current direction and the target direction are almost parallel
        rotation_axis = glm::vec3(0.0f, 0.0f, 1.0f); // can choose any axis perpendicular to these directions
    }
    else { rotation_axis = glm::normalize(rotation_axis); }

    const glm::quat rotation_quat = glm::angleAxis(angle, glm::normalize(rotation_axis));
    const auto current_quat = glm::quat(glm::radians(model->rotation));
    const glm::quat new_quat = rotation_quat * current_quat;

    // Build a quaternion rotation
    model->rotation = glm::degrees(glm::eulerAngles(new_quat));
}

#pragma region Callback implement

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void process_input(GLFWwindow *window)
{
    // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

#pragma region Camera
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.process_keyboard(k_forward, delta_time);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.process_keyboard(k_backward, delta_time);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.process_keyboard(k_left, delta_time);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.process_keyboard(k_right, delta_time);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.process_keyboard(k_up, delta_time);
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) camera.process_keyboard(k_down, delta_time);
#pragma endregion

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) dynamic_pos.x += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) dynamic_pos.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) dynamic_pos.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) dynamic_pos.y -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) dynamic_pos.z += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) dynamic_pos.z -= 1.0f;

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) tube->SetRotation(glm::vec3(60, 20, 50));
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, const int width, const int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow *window, const double x_pos_in, const double y_pos_in)
{
    const auto x_pos = static_cast<float>(x_pos_in);
    const auto y_pos = static_cast<float>(y_pos_in);

    if (first_mouse)
    {
        last_x = x_pos;
        last_y = y_pos;
        first_mouse = false;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        const float x_offset = x_pos - last_x;
        const float y_offset = last_y - y_pos;
        camera.process_mouse_movement(x_offset, y_offset);
    }

    last_x = x_pos;
    last_y = y_pos;
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double x_offset, const double y_offset)
{
    camera.process_mouse_scroll(static_cast<float>(y_offset));
}

#pragma endregion
