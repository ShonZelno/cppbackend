#pragma once

#include <filesystem>
#include <utility>

#include "game.h"
#include "loot_generator.h" 

namespace json_loader {

std::pair<model::Game, loot_gen::LootGenerator> LoadGame(const std::filesystem::path& json_path);

class OpenConfigFileOfModelException : public std::exception {
public:
    char const* what () {
        return "Can't open file with json configuration of model.";
    }
};

}  // namespace json_loader
