#include "player_serialization.h"

namespace game_data_ser {

app::Player PlayerSerialization::Restore() const {
  // Создаём игрока с id и именем
  app::Player player(app::Player::Id{id_}, name_);
  // Устанавливаем сохранённое время входа
  player.SetJoinTime(std::chrono::milliseconds(join_time_));
  return player;
};

model::Dog PlayerSerialization::RestoreDog() const {
  return dog_ser_.Restore();
};

authentication::Token PlayerSerialization::RestoreToken() const {
  return authentication::Token(token_);
};

} // namespace game_data_ser