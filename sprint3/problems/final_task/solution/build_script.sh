#!/bin/bash

#                              ================================================================
#                              ||     Справка по созданию и запуску скрипта сборки            ||
#                              ||                                                             ||
#                              || Создаем файл: nano build_script.sh                          ||
#                              || Вставляем этот код в файл, сохраняем и выходим.             ||
#                              || Делаем его исполняемым: chmod +x build_script.sh            ||
#                              || запускаем скрипт sudo ./build_script.sh                     ||
#                              ||                                                             ||
#                              || Режимы запуска:                                             ||
#                              ||   ./build_script.sh — обычный режим (по умолчанию Release)  ||
#                              ||   ./build_script.sh debug — режим Debug                     ||
#                              ||   ./build_script.sh release — режим Release                 ||
#                              ||   ./build_script.sh silent — тихий режим (без вывода Conan) ||
#                              ||   ./build_script.sh silent debug — тихий режим + Debug      ||
#                              ||   ./build_script.sh silent release — тихий режим + Release  ||
#                              ||                                                             ||
#                              =================================================================

# Определяем цветовые коды
BOLD="\033[1m"          # Жирный шрифт
YELLOW="\033[33m"       # Жёлтый
GREEN="\033[32m"        # Зелёный
CYAN="\033[38;5;82m"    # Салатовый
RED="\033[31m"          # Красный
BLINK="\033[5m"         # Мигание
NC="\033[0m"            # Сброс всех настроек

# Комбинированные стили
CYAN_BOLD="${CYAN}${BOLD}"    # Салатовый жирный
RED_BOLD="${RED}${BOLD}"      # Красный жирный
CYAN_BOLD_BLINK="${CYAN}${BOLD}${BLINK}" # Салатовый жирный мигающий

if [ "$1" == "-help" ] || [ "$1" == "--help" ]; then
    echo -e "${CYAN}${BOLD}Режимы запуска:${NC} 
        ${YELLOW}./build_script.sh${NC}${GREEN} — обычный режим (по умолчанию Release)${NC}
        ${YELLOW}./build_script.sh debug${NC}${GREEN} — режим Debug${NC}
        ${YELLOW}./build_script.sh release${NC}${GREEN} — режим Release${NC}
        ${YELLOW}./build_script.sh silent${NC}${GREEN} — тихий режим (без вывода Conan)${NC}
        ${YELLOW}./build_script.sh silent debug${NC}${GREEN} — тихий режим + Debug${NC}
        ${YELLOW}./build_script.sh silent release${NC}${GREEN} — тихий режим + Releas${NC}
    "
    exit 0
fi

# Проверяем наличие параметра скрытия вывода
SILENT=false
if [ "$1" == "silent" ]; then
    SILENT=true
    shift # Убираем параметр из списка
fi

# Проверяем наличие параметра сборки
BUILD_TYPE="Release"

# Если передан параметр, проверяем его
if [ "$1" == "debug" ]; then
    BUILD_TYPE="Debug"
    echo -e "${CYAN_BOLD}Сборка в режиме Debug${NC}"
elif [ "$1" == "release" ]; then
    BUILD_TYPE="Release"
    echo -e "${CYAN_BOLD}Сборка в режиме Release${NC}"
else
    echo -e "${CYAN_BOLD}Использование режима сборки по умолчанию: Release${NC}"
fi

# Проверяем, есть ли у нас права на выполнение
#if [ "$(id -u)" != "0" ]; then
#   echo -e "Пожалуйста, запустите скрипт с sudo или от имени root"
#   exit 1
#fi

# Создаем директорию build и переходим в нее
mkdir -p build
cd build || { echo -e "${RED_BOLD}Ошибка при переходе в директорию build${NC}"; exit 1; }

# Устанавливаем зависимости через Conan
if $SILENT; then
echo -e "${CYAN_BOLD}Тихая установка зависимостей через Conan...${NC}"
conan install .. \
    --build=missing \
    -s build_type=$BUILD_TYPE \
    -s compiler.libcxx=libstdc++11 > /dev/null 2>&1
else
echo -e "${CYAN_BOLD}Установка зависимостей через Conan...${NC}"
conan install .. \
    --build=missing \
    -s build_type=$BUILD_TYPE \
    -s compiler.libcxx=libstdc++11
fi

# Проверяем успешность установки Conan
if [ $? -ne 0 ]; then
    echo -e "${RED_BOLD}Ошибка при установке зависимостей Conan${NC}"
    exit 1
fi

# Конфигурируем проект через CMake
if $SILENT; then
echo -e "${CYAN_BOLD}Тихая конфигурация проекта через CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE > /dev/null 2>&1
else
echo -e "${CYAN_BOLD}Конфигурация проекта через CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE
fi

# Возвращаемся в исходную директорию
cd .. || { echo -e "${RED_BOLD}Ошибка при возврате в исходную директорию${NC}"; exit 1; }

echo -e "${CYAN_BOLD_BLINK}Скрипт выполнен успешно!${NC}"