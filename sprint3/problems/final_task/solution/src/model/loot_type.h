#pragma once
#include <string>
#include <limits.h>
#include <optional>

namespace model {

struct LootType {
    std::string name{""};
    std::string file{""};
    std::string type{""};
    std::optional<int> rotation{std::nullopt};
    std::optional<std::string> color{std::nullopt};
    double scale{0.0};
    size_t value{0};
};


}