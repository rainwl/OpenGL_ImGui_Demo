#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Defines several possible options for camera movement.
// Used as abstraction to stay away from window-system specific input methods
enum camera_movement {
  k_forward ,
  k_backward ,
  k_left ,
  k_right,
  k_up,
  k_down
};

// Default camera values
constexpr float def_yaw = 0.0f; // -90.0f
constexpr float def_pitch = 0.0f;
constexpr float def_speed = 50.0f;
constexpr float def_sensitivity = 0.1f;
constexpr float def_zoom = 45.0f;

// An abstract camera class that processes input and calculates
// the corresponding Euler Angles,Vectors and Matrices for use in OpenGL
class Camera {
public:
  // constructor with vectors
  explicit Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = def_yaw, float pitch = def_pitch)
    : cam_front(glm::vec3(0.0f, 0.0f, -1.0f)), cam_movement_speed(def_speed), cam_mouse_sensitivity(def_sensitivity), cam_zoom(def_zoom) {
    cam_position = position;
    cam_world_up = up;
    cam_yaw = yaw;
    cam_pitch = pitch;
    update_camera_vectors();
  }

  // constructor with scalar values
  Camera(float pos_x, float pos_y, float pos_z, float up_x, float up_y, float up_z, float yaw, float pitch)
    : cam_front(glm::vec3(0.0f, 0.0f, -1.0f)), cam_movement_speed(def_speed), cam_mouse_sensitivity(def_sensitivity), cam_zoom(def_zoom) {
    cam_position = glm::vec3(pos_x, pos_y, pos_z);
    cam_world_up = glm::vec3(up_x, up_y, up_z);
    cam_yaw = yaw;
    cam_pitch = pitch;
    update_camera_vectors();
  }

  // returns the view matrix calculated using Euler Angles and the LookAt Matrix
  glm::mat4 get_view_matrix() const { return glm::lookAt(cam_position, cam_position + cam_front, cam_up); }

  // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
  void process_keyboard(const camera_movement direction, const float delta_time) {
    const float velocity = cam_movement_speed * delta_time;
    if (direction == k_forward) cam_position += cam_front * velocity;
    if (direction == k_backward) cam_position -= cam_front * velocity;
    if (direction == k_left) cam_position -= cam_right * velocity;
    if (direction == k_right) cam_position += cam_right * velocity;
    if (direction == k_up)   cam_position += cam_up * velocity;
    if (direction == k_down)   cam_position -= cam_up * velocity;
      
      
    
  }

  // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
  void process_mouse_movement(float x_offset, float y_offset, const GLboolean constrain_pitch = true) {
    x_offset *= cam_mouse_sensitivity;
    y_offset *= cam_mouse_sensitivity;

    cam_yaw += x_offset;
    cam_pitch += y_offset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrain_pitch) {
      if (cam_pitch > 89.0f) cam_pitch = 89.0f;
      if (cam_pitch < -89.0f) cam_pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    update_camera_vectors();
  }

  // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
  void process_mouse_scroll(const float y_offset) {
    cam_zoom -= static_cast<float>(y_offset);
    if (cam_zoom < 1.0f) cam_zoom = 1.0f;
    if (cam_zoom > 45.0f) cam_zoom = 45.0f;
  }

  // camera Attributes
  glm::vec3 cam_position;
  glm::vec3 cam_front;
  glm::vec3 cam_up;
  glm::vec3 cam_right;
  glm::vec3 cam_world_up;
  // euler Angles
  float cam_yaw;
  float cam_pitch;
  // camera options
  float cam_movement_speed;
  float cam_mouse_sensitivity;
  float cam_zoom;

private:
  // calculates the front vector from the Camera's (updated) Euler Angles
  void update_camera_vectors() {
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(cam_yaw)) * cos(glm::radians(cam_pitch));
    front.y = sin(glm::radians(cam_pitch));
    front.z = sin(glm::radians(cam_yaw)) * cos(glm::radians(cam_pitch));
    cam_front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    cam_right = glm::normalize(glm::cross(cam_front, cam_world_up));// normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    cam_up = glm::normalize(glm::cross(cam_right, cam_front));
  }
};
#endif
