#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace saving {

struct SavingSettings {
  std::optional<std::string> state_file_path;
  std::optional<std::chrono::milliseconds> period;
};

} // namespace saving