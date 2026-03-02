# Создать образ на основе базового слоя gcc (там будет ОС и сам компилятор).
# 11.3 — используемая версия gcc.
FROM gcc:11.3 as build

# Выполнить установку зависимостей внутри контейнера.
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# СОЗДАЕМ DEFAULT ПРОФИЛЬ CONAN (для Conan 1.x)
# RUN conan profile new default --detect --force || true

# копируем conanfile.txt в контейнер и запускаем conan install
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Release

# только после этого копируем остальные исходники
COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www

USER www

# Скопируем приложение со сборочного контейнера в директорию /app.
COPY ./data /app/data
COPY ./static /app/static
COPY --from=build /app/build/bin/game_server /app/
COPY --from=build /app/build/bin/collision_detection_tests /app/
COPY --from=build /app/build/bin/loot_generator_tests /app/
COPY --from=build /app/build/bin/state_serialization_tests /app/


# Запускаем игровой сервер
ENTRYPOINT ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static"]