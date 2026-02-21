cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Boost 1.82.0 REQUIRED)
find_package(Boost 1.82.0 COMPONENTS log log_setup program_options)
find_package(Catch2 REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Статическая библиотека с моделью
add_library(GameModelLib STATIC 
    src/model/map.cpp
    src/model/game.cpp
    src/model/dog.cpp
    src/model/roadmap.cpp
    src/model/support_types.cpp
    src/model/loot_generator.cpp
    src/model/lost_object.cpp
    src/utils/request_handlers_utils.cpp
    src/utils/filesystem_utils.cpp
    src/utils/random_generators.cpp
)

# Добавляем пути для поиска заголовочных файлов
target_include_directories(GameModelLib 
    PUBLIC 
        src
        src/model
        src/utils
    PRIVATE
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(GameModelLib PRIVATE CONAN_PKG::boost)

# Основная программа
add_executable(game_server
    src/main.cpp    
    src/json/json_loader.cpp
    src/json/json_converter.cpp
    src/json/json_model_converter.cpp
    src/request_handlers/request_handler.cpp
    src/boost_json.cpp
    src/logging/logging_data_storage.cpp
    src/logging/logger.cpp
    src/authentication/player_tokens.cpp
    src/app/application.cpp
    src/app/game_session.cpp
    src/app/player.cpp
    src/server/http_server.cpp
    src/program_options/program_options.cpp
    src/time_management/ticker.cpp
    src/error_handling/error_report.cpp
)

target_include_directories(game_server PRIVATE
    src
    src/json
    src/request_handlers
    src/logging
    src/authentication
    src/app
    src/server
    src/program_options
    src/time_management
    src/error_handling
)

target_link_libraries(game_server 
    PRIVATE 
        Threads::Threads 
        CONAN_PKG::boost 
        GameModelLib
)

# Тесты
add_executable(catch_tests
    tests/loot_generator_tests.cpp
)

target_include_directories(catch_tests PRIVATE
    src
    src/model
    tests
)

target_link_libraries(catch_tests 
    PRIVATE 
        CONAN_PKG::catch2
        GameModelLib 
)

# Boost.Beast будет использовать std::string_view вместо boost::string_view
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

# Включаем тестирование
include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)