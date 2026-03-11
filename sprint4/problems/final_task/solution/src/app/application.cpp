#include "application.h"
#include "pg_connection_pool.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/thread/future.hpp>
#include <iostream>

namespace app {

using namespace std::literals;

const model::Game::Maps &Application::ListMap() const noexcept {
  return game_.GetMaps();
};

const std::shared_ptr<model::Map>
Application::FindMap(const model::Map::Id &id) const noexcept {
  return game_.FindMap(id);
};

std::tuple<authentication::Token, Player::Id>
Application::JoinGame(const std::string &player_name,
                      const model::Map::Id &id) {
  auto player = CreatePlayer(player_name);
  player->SetJoinTime(total_game_time_);
  auto token = player_tokens_.AddPlayer(player);
  player->SetToken(token);
  std::shared_ptr<GameSession> game_session = FindGameSessionBy(id);
  if (!game_session) {
    game_session = std::make_shared<GameSession>(
        game_.FindMap(id), tick_period_, game_.GetLootGeneratorConfig(), ioc_,
        std::chrono::milliseconds(static_cast<long long>(
            game_.GetDefaultDogRetirementTime() * 1000.0)));
    AddGameSession(game_session);
    game_session->SetRetiredHandler(
        [self = shared_from_this()](model::Dog::Id dog_id) {
          self->OnDogRetired(dog_id);
        });
    game_session->Run();
  }
  auth_token_to_session_index_[token] = game_session;
  BoundPlayerAndGameSession(player, game_session);
  game_session_to_token_player_pair_[game_session][token] = player;
  return std::tie(token, player->GetId());
};

std::shared_ptr<Player>
Application::CreatePlayer(const std::string &player_name) {
  auto player = std::make_shared<Player>(player_name);
  players_.push_back(player);
  return player;
};

void Application::BoundPlayerAndGameSession(
    std::shared_ptr<Player> player, std::shared_ptr<GameSession> session) {
  session_id_to_players_[session->GetId()].push_back(player);
  player->SetGameSession(session);
  auto dog = session
                 ->CreateDog(player->GetName(), *(session->GetMap()),
                             randomize_spawn_points_)
                 .lock();
  player->SetDog(dog);
  if (dog) {
    dog_id_to_player_[dog->GetId()] = player;
  }
}
const std::vector<std::shared_ptr<Player>> &
Application::GetPlayersFromGameSession(const authentication::Token &token) {
  static const std::vector<std::shared_ptr<Player>> emptyPlayerList;
  auto player = player_tokens_.FindPlayerBy(token);
  auto session_id = player->GetGameSessionId();
  if (!session_id_to_players_.contains(session_id)) {
    return emptyPlayerList;
  }
  return session_id_to_players_[session_id];
};

bool Application::IsExistPlayer(const authentication::Token &token) {
  return static_cast<bool>(player_tokens_.FindPlayerBy(token));
};

void Application::SetPlayerAction(const authentication::Token &token,
                                  model::Direction direction) {
  auto player = player_tokens_.FindPlayerBy(token);
  auto dog = player->GetDog().lock();
  double velocity = player->GetGameSession()->GetMap()->GetDogVelocity();
  dog->SetAction(direction, velocity);
};

bool Application::IsManualTimeManagement() {
  return tick_period_.count() == 0;
};

void Application::UpdateGameState(const std::chrono::milliseconds &delta_time) {
  total_game_time_ += delta_time;
  for (auto session : sessions_) {
    boost::promise<void> res_promise;
    auto res_future = res_promise.get_future();
    net::dispatch(*(session->GetStrand()),
                  [session, &delta_time, &res_promise] {
                    session->UpdateGameState(delta_time);
                    res_promise.set_value();
                  });
    res_future.get();
  }
  {
    boost::promise<void> flush_promise;
    auto flush_future = flush_promise.get_future();
    net::post(ioc_, [&flush_promise]() { flush_promise.set_value(); });
    flush_future.get();
  }
  SaveGameState(delta_time);
};

void Application::AddGameSession(std::shared_ptr<GameSession> session) {
  const size_t index = sessions_.size();
  if (auto [it, inserted] =
          map_id_to_session_index_.emplace(session->GetMap()->GetId(), index);
      !inserted) {
    throw std::invalid_argument("Game session with map id "s +
                                *(session->GetMap()->GetId()) +
                                " already exists"s);
  } else {
    try {
      sessions_.push_back(session);
    } catch (...) {
      map_id_to_session_index_.erase(it);
      throw;
    }
  }
};

std::shared_ptr<GameSession>
Application::FindGameSessionBy(const model::Map::Id &id) const noexcept {
  if (auto it = map_id_to_session_index_.find(id);
      it != map_id_to_session_index_.end()) {
    return sessions_.at(it->second);
  }
  return nullptr;
};

std::shared_ptr<GameSession> Application::FindGameSessionBy(
    const authentication::Token &token) const noexcept {
  if (auto it = auth_token_to_session_index_.find(token);
      it != auth_token_to_session_index_.end()) {
    return it->second;
  }
  return nullptr;
};

const std::vector<std::shared_ptr<GameSession>> &Application::GetSessions() {
  return sessions_;
};

void Application::RestoreGameState(saving::SavingSettings saving_settings) {
  saving_settings_ = std::move(saving_settings);
  RestoreGame();
  if (!(saving_settings_.state_file_path && saving_settings_.period) ||
      IsManualTimeManagement()) {
    return;
  }
  save_game_ticker_ = std::make_shared<time_m::Ticker>(
      ioc_, saving_settings_.period.value(),
      [self = shared_from_this()](const std::chrono::milliseconds &delta_time) {
        self->SaveGame();
      });
  save_game_ticker_->Start();
};

void Application::SaveGameState(const std::chrono::milliseconds &delta_time) {
  static int period =
      saving_settings_.period ? saving_settings_.period.value().count() : 0;
  if (!saving_settings_.period) {
    return;
  }
  period -= delta_time.count();
  if (period <= 0) {
    SaveGame();
    period = saving_settings_.period.value().count();
  }
};

void Application::SaveGame() {
  if (!saving_settings_.state_file_path)
    return;
  std::fstream output_fstream(saving_settings_.state_file_path.value(),
                              std::ios_base::out);
  boost::archive::text_oarchive oarchive{output_fstream};

  // Версия формата (2 — с поддержкой времени)
  int version = 2;
  oarchive << version;
  oarchive << total_game_time_.count();

  auto sessions_ser = GetSerializedData();
  oarchive << sessions_ser;
};

std::vector<game_data_ser::GameSessionSerialization>
Application::GetSerializedData() {
  using game_data_ser::GameSessionSerialization;
  std::vector<GameSessionSerialization> sessions_ser;
  for (auto session_ptr : sessions_) {
    boost::promise<GameSessionSerialization> promise;
    auto res_future = promise.get_future();
    net::dispatch(*(session_ptr->GetStrand()), [self = shared_from_this(),
                                                &promise, session_ptr] {
      promise.set_value(GameSessionSerialization(
          *session_ptr,
          self->game_session_to_token_player_pair_.at(session_ptr)));
    });
    sessions_ser.push_back(std::move(res_future.get()));
  };
  return sessions_ser;
};
void Application::RestoreGame() {
  if (!saving_settings_.state_file_path)
    return;
  std::fstream input_fstream(saving_settings_.state_file_path.value(),
                             std::ios_base::in);
  if (!input_fstream.is_open())
    return;

  boost::archive::text_iarchive iarchive{input_fstream};

  int version;
  iarchive >> version;
  if (version != 2)
    throw std::runtime_error("Unsupported save file version");

  size_t saved_time;
  iarchive >> saved_time;
  total_game_time_ = std::chrono::milliseconds(saved_time);

  std::vector<game_data_ser::GameSessionSerialization> sessions_ser;
  iarchive >> sessions_ser;

  for (auto &item : sessions_ser) {
    auto game_session = std::make_shared<GameSession>(
        game_.FindMap(item.RestoreMapId()), tick_period_,
        game_.GetLootGeneratorConfig(), ioc_,
        std::chrono::milliseconds(static_cast<long long>(
            game_.GetDefaultDogRetirementTime() * 1000.0)));

    for (auto &lost_obj_ser : item.GetLostObjectsSerialize()) {
      game_session->AddLostObject(std::move(lost_obj_ser.Restore()));
    }

    for (auto &player_ser : item.GetPlayersSerialize()) {
      auto player =
          std::make_shared<app::Player>(std::move(player_ser.Restore()));
      player->SetJoinTime(std::chrono::milliseconds(player_ser.GetJoinTime()));

      auto dog =
          std::make_shared<model::Dog>(std::move(player_ser.RestoreDog()));
      dog->SetIdleTime(std::chrono::milliseconds(
          player_ser.GetDogSerialization().GetIdleTime()));

      game_session->AddDog(dog);
      player->SetDog(dog);
      player->SetGameSession(game_session);
      auto token = player_ser.RestoreToken();
      auth_token_to_session_index_[token] = game_session;
      game_session_to_token_player_pair_[game_session][token] = player;
      player_tokens_.AddTokenPlayerPair(token, player);
      auto session_id = game_session->GetId();
      session_id_to_players_[session_id].push_back(player);
    }

    AddGameSession(game_session);
    game_session->SetRetiredHandler(
        [self = shared_from_this()](model::Dog::Id dog_id) {
          self->OnDogRetired(dog_id);
        });
    game_session->Run();
  }
}

void Application::OnDogRetired(model::Dog::Id dog_id) {
  std::string name;
  size_t score;
  double play_time;

  {
    std::lock_guard<std::mutex> lock(application_mutex_);
    auto it = dog_id_to_player_.find(dog_id);
    if (it == dog_id_to_player_.end())
      return;
    auto player = it->second;
    name = player->GetName();
    auto dog = player->GetDog().lock();
    score = dog ? dog->GetScore() : 0;
    play_time =
        std::chrono::duration<double>(total_game_time_ - player->GetJoinTime())
            .count();
  }

  net::post(
      ioc_, [self = shared_from_this(), dog_id, name, score, play_time]() {
        authentication::Token token{""};
        {
          std::lock_guard<std::mutex> lock(self->application_mutex_);
          auto it = self->dog_id_to_player_.find(dog_id);
          if (it == self->dog_id_to_player_.end())
            return;
          auto player = it->second;
          token = player->GetToken();
          auto session = player->GetGameSession();
          if (session) {
            auto &token_map = self->game_session_to_token_player_pair_[session];
            token_map.erase(token);
            self->auth_token_to_session_index_.erase(token);
            if (token_map.empty()) {
              self->game_session_to_token_player_pair_.erase(session);
            }
            auto &players_in_session =
                self->session_id_to_players_[session->GetId()];
            std::erase(players_in_session, player);
          }
          self->dog_id_to_player_.erase(dog_id);
          self->player_tokens_.RemovePlayer(token);
        }
        self->AddRetiredPlayer(name, score, play_time);
      });
}

std::vector<RetiredRecord> Application::GetRecords(size_t start, size_t limit) {
  std::vector<RetiredRecord> result;
  auto conn = db_pool_->GetConnection();
  pqxx::work w(*conn);
  std::string query = "SELECT name, score, play_time FROM retired_players "
                      "ORDER BY score DESC, play_time ASC, name ASC "
                      "OFFSET " +
                      std::to_string(start) + " LIMIT " + std::to_string(limit);
  for (auto [name, score, play_time] :
       w.query<std::string, int, double>(query)) {
    result.push_back({name, static_cast<size_t>(score), play_time});
  }
  db_pool_->ReturnConnection(std::move(conn));
  return result;
}

void Application::AddRetiredPlayer(const std::string &name, size_t score,
                                   double play_time) {
  if (!db_pool_)
    return;
  auto conn = db_pool_->GetConnection();
  pqxx::work w(*conn);
  w.exec_params("INSERT INTO retired_players (name, score, play_time) VALUES "
                "($1, $2, $3)",
                name, score, play_time);
  w.commit();
  db_pool_->ReturnConnection(std::move(conn));
}

} // namespace app