#pragma once

#define ImTextureID ImU64

#include <map>
#include <string>
#include <vector>

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include "./mutex.hpp"

#define ICON_FK_UNDO u8"\uf0e2"

namespace renodx::utils::user_settings {

static int preset_index = 1;
static const char* preset_strings[] = {
  "Off",
  "Preset #1",
  "Preset #2",
  "Preset #3",
};

static void (*on_preset_off)();

enum class UserSettingValueType : uint8_t {
  FLOAT = 0,
  INTEGER = 1,
  BOOLEAN = 2
};

struct UserSetting {
  const char* key;
  float* binding;
  UserSettingValueType value_type = UserSettingValueType::FLOAT;
  float default_value = 0.f;
  bool can_reset = true;
  const char* label = key;
  const char* section = "";
  const char* tooltip = "";
  std::vector<const char*> labels;
  float min = 0.f;
  float max = 100.f;
  const char* format = "%.0f";
  bool (*is_enabled)() = [] {
    return true;
  };

  float (*parse)(float value) = [](float value) {
    return value;
  };

  [[nodiscard]]
  float GetValue() const {
    switch (this->value_type) {
      default:
      case UserSettingValueType::FLOAT:
        return this->value;
        break;
      case UserSettingValueType::INTEGER:
        return static_cast<float>(this->value_as_int);
        break;
      case UserSettingValueType::BOOLEAN:
        return ((this->value_as_int == 0) ? 0.f : 1.f);
        break;
    }
  }

  UserSetting* Set(float value) {
    this->value = value;
    this->value_as_int = static_cast<int>(value);
    return this;
  }

  UserSetting* Write() {
    *this->binding = this->parse(this->GetValue());
    return this;
  }

  float value = default_value;
  int value_as_int = static_cast<int>(default_value);

  [[nodiscard]]
  float GetMax() const {
    switch (this->value_type) {
      case UserSettingValueType::BOOLEAN:
        return 1.f;
      case UserSettingValueType::INTEGER:
        return this->labels.empty()
               ? this->max
               : (this->labels.size() - 1);
      case UserSettingValueType::FLOAT:
      default:
        return this->max;
    }
  }
};

using UserSettings = std::vector<UserSetting*>;
static UserSettings* user_settings = nullptr;

#define AddDebugSetting(injection, name)          \
  new renodx::utils::user_settings::UserSetting { \
    .key = "debug" #name,                         \
    .binding = &##injection.debug##name,          \
    .default_value = 1.f,                         \
    .label = "Debug" #name,                       \
    .section = "Debug",                           \
    .max = 2.f,                                   \
    .format = "%.2f"                              \
  }

static UserSetting* FindUserSetting(const char* key) {
  for (auto* setting : *user_settings) {
    if (strcmp(setting->key, key) == 0) {
      return setting;
    }
  }
  return nullptr;
}

static bool UpdateUserSetting(const char* key, float value) {
  auto* setting = FindUserSetting(key);
  if (setting == nullptr) return false;
  setting->Set(value)->Write();
  return true;
}

static void LoadSettings(
  reshade::api::effect_runtime* runtime = nullptr,
  const char* section = "renodx-preset1"
) {
  for (auto* setting : *user_settings) {
    switch (setting->value_type) {
      default:
      case UserSettingValueType::FLOAT:
        if (!reshade::get_config_value(runtime, section, setting->key, setting->value)) {
          setting->value = setting->default_value;
        }
        if (setting->value > setting->GetMax()) {
          setting->value = setting->GetMax();
        } else if (setting->value < setting->min) {
          setting->value = setting->min;
        }
        break;
      case UserSettingValueType::BOOLEAN:
      case UserSettingValueType::INTEGER:
        if (!reshade::get_config_value(runtime, section, setting->key, setting->value_as_int)) {
          setting->value_as_int = static_cast<int>(setting->default_value);
        }
        if (setting->value_as_int > setting->GetMax()) {
          setting->value_as_int = setting->GetMax();
        } else if (setting->value_as_int < static_cast<int>(setting->min)) {
          setting->value_as_int = static_cast<int>(setting->min);
        }
        break;
    }
    setting->Write();
  }
}

static void SaveSettings(reshade::api::effect_runtime* runtime, const char* section = "renodx-preset1") {
  for (auto* setting : *user_settings) {
    switch (setting->value_type) {
      default:
      case UserSettingValueType::FLOAT:
        reshade::set_config_value(runtime, section, setting->key, setting->value);
        break;
      case UserSettingValueType::INTEGER:
      case UserSettingValueType::BOOLEAN:
        reshade::set_config_value(runtime, section, setting->key, setting->value_as_int);
        break;
    }
  }
}

// Runs first
// https://pthom.github.io/imgui_manual_online/manual/imgui_manual.html
static void OnRegisterOverlay(reshade::api::effect_runtime* runtime) {
  const std::unique_lock lock(renodx::utils::mutex::global_mutex);
  const bool changed_preset = ImGui::SliderInt(
    "Preset",
    &preset_index,
    0,
    (sizeof(preset_strings) / sizeof(char*)) - 1,
    preset_strings[preset_index],
    ImGuiSliderFlags_NoInput
  );

  if (changed_preset) {
    switch (preset_index) {
      case 0:
        if (on_preset_off != nullptr) {
          on_preset_off();
        }
        break;
      case 1:
        LoadSettings(runtime);
        break;
      case 2:
        LoadSettings(runtime, "renodx-preset2");
        break;
      case 3:
        LoadSettings(runtime, "renodx-preset3");
        break;
    }
  }

  bool any_change = false;
  std::string last_section;
  for (auto* setting : *user_settings) {
    if (last_section != setting->section) {
      ImGui::SeparatorText(setting->section);
      last_section.assign(setting->section);
    }
    const bool is_disabled = preset_index == 0
                          || (setting->is_enabled != nullptr
                              && !setting->is_enabled());
    if (is_disabled) {
      ImGui::BeginDisabled();
    }
    bool changed = false;
    switch (setting->value_type) {
      case UserSettingValueType::FLOAT:
        changed |= ImGui::SliderFloat(
          setting->label,
          &setting->value,
          setting->min,
          setting->max,
          setting->format
        );
        break;
      case UserSettingValueType::INTEGER:
        changed |= ImGui::SliderInt(
          setting->label,
          &setting->value_as_int,
          setting->min,
          setting->GetMax(),
          setting->labels.empty() ? setting->format : setting->labels.at(setting->value_as_int),
          ImGuiSliderFlags_NoInput
        );
        break;
      case UserSettingValueType::BOOLEAN:
        changed |= ImGui::SliderInt(
          setting->label,
          &setting->value_as_int,
          0,
          1,
          setting->labels.empty()
            ? ((setting->value_as_int == 0) ? "Off" : "On")  // NOLINT(readability-avoid-nested-conditional-operator)
            : setting->labels.at(setting->value_as_int),
          ImGuiSliderFlags_NoInput
        );
        break;
    }
    if (strlen(setting->tooltip) != 0) {
      ImGui::SetItemTooltip(setting->tooltip);  // NOLINT(clang-diagnostic-format-security)
    }

    if (preset_index != 0 && setting->can_reset) {
      ImGui::SameLine();
      const bool is_using_default = (setting->GetValue() == setting->default_value);
      ImGui::BeginDisabled(is_using_default);
      ImGui::PushID(&setting->default_value);
      if (is_using_default) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor::HSV(0, 0, 0.6f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor::HSV(0, 0, 0.7f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor::HSV(0, 0, 0.8f)));
      }
      auto* font = ImGui::GetFont();
      auto old_scale = font->Scale;
      auto previous_font_size = ImGui::GetFontSize();
      font->Scale *= 0.75f;
      ImGui::PushFont(font);
      auto current_font_size = ImGui::GetFontSize();

      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, current_font_size * 2);

      ImVec2 cursor_pos = ImGui::GetCursorPos();
      cursor_pos.y += (previous_font_size / 2.f) - (current_font_size / 2.f);
      ImGui::SetCursorPos(cursor_pos);

      if (ImGui::Button(reinterpret_cast<const char*>(ICON_FK_UNDO))) {
        setting->Set(setting->default_value);
        changed = true;
      }

      if (is_using_default) {
        ImGui::PopStyleColor(3);
      }
      font->Scale = old_scale;
      ImGui::PopFont();
      ImGui::PopStyleVar();
      ImGui::PopID();
      ImGui::EndDisabled();
    }

    if (changed) {
      setting->Write();
      any_change = true;
    }
    if (is_disabled) {
      ImGui::EndDisabled();
    }
  }
  if (!changed_preset && any_change) {
    switch (preset_index) {
      case 1:
        SaveSettings(runtime, "renodx-preset1");
        break;
      case 2:
        SaveSettings(runtime, "renodx-preset2");
        break;
      case 3:
        SaveSettings(runtime, "renodx-preset3");
        break;
    }
  }
}

static bool attached = false;

static void Use(DWORD fdw_reason, UserSettings* new_settings, void (*new_on_preset_off)() = nullptr) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (attached) return;
      attached = true;

      user_settings = new_settings;
      on_preset_off = new_on_preset_off;
      LoadSettings();
      reshade::register_overlay("RenoDX", OnRegisterOverlay);

      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_overlay("RenoDX", OnRegisterOverlay);
      break;
  }
}

}  // namespace renodx::utils::user_settings
