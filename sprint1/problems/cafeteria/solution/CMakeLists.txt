cmake_minimum_required(VERSION 3.11)
project(cafeteria CXX)

# Явно задаём стандарт C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Сначала подключаем Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Затем ищем зависимости
find_package(Boost 1.81.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Добавляем исполняемый файл
add_executable(cafeteria
  src/main.cpp
  src/cafeteria.h
  src/result.h
  src/hotdog.h
  src/gascooker.h
  src/ingredients.h
  src/clock.h
)

target_link_libraries(cafeteria PRIVATE Threads::Threads)
