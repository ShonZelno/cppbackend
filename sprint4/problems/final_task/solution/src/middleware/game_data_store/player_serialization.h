#pragma once
#include "dog_serialization.h"
#include "player.h"
#include "player_tokens.h"

#include <boost/serialization/vector.hpp>

namespace game_data_ser {

class PlayerSerialization {
public:
  PlayerSerialization() = default;
  PlayerSerialization(app::Player &player, const authentication::Token &token)
      : id_(*player.GetId()), name_(player.GetName()),
        dog_ser_(*player.GetDog().lock()), token_(*token),
        join_time_(player.GetJoinTime().count()){};
  PlayerSerialization(PlayerSerialization &&other) = default;

  [[nodiscard]] app::Player Restore() const;
  [[nodiscard]] model::Dog RestoreDog() const;
  [[nodiscard]] authentication::Token RestoreToken() const;

  size_t GetJoinTime() const { return join_time_; }
  const DogSerialization &GetDogSerialization() const { return dog_ser_; }

  template <typename Archive>
  void serialize(Archive &ar, [[maybe_unused]] const unsigned version) {
    ar &id_;
    ar &name_;
    ar &dog_ser_;
    ar &token_;
    ar &join_time_;
  }

private:
  size_t id_;
  std::string name_;
  DogSerialization dog_ser_;
  std::string token_;
  size_t join_time_;
};

} // namespace game_data_ser