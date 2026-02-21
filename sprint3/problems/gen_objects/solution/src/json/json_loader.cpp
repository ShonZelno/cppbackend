#include "json_loader.h"
#include "logger.h"
#include "json_key_storage.h"
#include "json_model_converter.h"
#include "model_key_storage.h"

#include <fstream>
#include <iostream>
#include <string_view>
#include <sstream>

#include <iostream>

namespace json_loader {

using namespace std::literals;


boost::json::value ReadFile(const std::filesystem::path& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        BOOST_LOG_TRIVIAL(error) << logware::CreateLogMessage("error"sv,
                                        logware::ExceptionLogData(EXIT_FAILURE,
                                            "Error: Can't open file."sv,
                                            "write something here"sv)); // todo: write message and handler.
        throw OpenConfigFileOfModelException();
    }
    
    std::stringstream ss;
    ss << file.rdbuf();
    boost::json::value root = boost::json::parse(ss.str());
    return root;
};

std::pair<model::Game, loot_gen::LootGenerator> LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;
    boost::json::value jsonVal = ReadFile(json_path);
    
    // Загрузка конфигурации генератора трофеев
    double period = 1.0;
    double probability = 1.0;
    try {
        auto const& root_obj = jsonVal.as_object();
        if (root_obj.contains("lootGeneratorConfig")) {
            auto const& loot_config = root_obj.at("lootGeneratorConfig").as_object();
            period = boost::json::value_to<double>(loot_config.at("period"));
            probability = boost::json::value_to<double>(loot_config.at("probability"));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading loot generator config: " << e.what() << std::endl;
    }
    
    loot_gen::LootGenerator generator(
        std::chrono::milliseconds(static_cast<int>(period * 1000)),
        probability
    );
    
    // Загрузка карт
    std::vector<model::Map> maps = boost::json::value_to< std::vector<model::Map> >(jsonVal.as_object().at(model::MAPS));
    
    // Устанавливаем генератор для каждой карты
    for (auto& map : maps) {
        map.SetLootGenerator(generator);
    }
    
    game.AddMaps(maps);
    
    try {
        double default_dog_velocity = boost::json::value_to<double>(jsonVal.as_object().at(model::DEFAULT_DOG_VELOCITY));
        game.SetDefaultDogVelocity(default_dog_velocity);
    } catch(...) {}
    
    return {std::move(game), std::move(generator)};
};

}  // namespace json_loader
