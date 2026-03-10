#pragma once
#include "game_session.h"
#include "player_tokens.h"
#include "tagged.h"

#include <string>

namespace app {

class Player {
  inline static size_t max_id_cont_ = 0;

public:
  using Id = util::Tagged<size_t, Player>;
  Player(std::string name) : id_(Id{Player::max_id_cont_++}), name_(name){};
  Player(Id id, std::string name) : id_(id), name_(name) {
    if (*id_ >= Player::max_id_cont_) {
      Player::max_id_cont_ = *id_ + 1;
    }
  };
  Player(const Player &other) = default;
  Player(Player &&other) = default;
  Player &operator=(const Player &other) = default;
  Player &operator=(Player &&other) = default;
  virtual ~Player() = default;

  const Id &GetId() const;
  const std::string &GetName() const;
  const GameSession::Id &GetGameSessionId() const;
  std::shared_ptr<GameSession> GetGameSession();
  void SetGameSession(std::shared_ptr<GameSession> session);
  std::weak_ptr<model::Dog> GetDog();
  void SetDog(std::weak_ptr<model::Dog> dog);

  void SetJoinTime(std::chrono::milliseconds time) { join_time_ = time; }
  std::chrono::milliseconds GetJoinTime() const { return join_time_; }
  void SetToken(authentication::Token token) { token_ = token; }
  authentication::Token GetToken() const { return token_; }

private:
  Id id_;
  std::string name_;
  std::shared_ptr<GameSession> session_;
  std::weak_ptr<model::Dog> dog_;
  authentication::Token token_;
  std::chrono::milliseconds join_time_{0};
};

} // namespace app