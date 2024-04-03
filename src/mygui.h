#pragma once
#include "imgui.h"

#include <iostream>
#include <string>
#include <vector>

std::vector<int> drag_list;

void draw_gui();
inline void draw_gui() {
  ImGui::Begin(u8"Debug Window");// u8 for chinese,but do not make sense

  for (size_t i = 0; i < 5; ++i) {
    ImGui::Button(std::to_string(i).c_str());
    if (i < 5) {
      ImGui::SameLine();
    }
    if (ImGui::BeginDragDropSource()) {
      ImGui::Text(std::string("Darg: ").append(std::to_string(i).c_str()).c_str());
      ImGui::SetDragDropPayload("DragIndexButton", &i, sizeof(int));
      ImGui::EndDragDropSource();
    }
  }

  ImGui::End();

  ImGui::Begin("Drag Window");

  ImGui::Text("Drag Target");

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DragIndexButton")) {
      drag_list.push_back(*(const int *) payload->Data);
    }

    ImGui::EndDragDropTarget();
  }

  for (size_t i = 0; i < drag_list.size(); ++i) {
    //ImGui::Button(std::to_string(drag_list.at(i)).c_str());

    if (ImGui::Button(std::to_string(drag_list.at(i)).c_str())) {
      drag_list.erase(drag_list.begin() + i);
    }

    if (i + 1 < drag_list.size()) {
      ImGui::SameLine();
    }
  }

  ImGui::End();
}
