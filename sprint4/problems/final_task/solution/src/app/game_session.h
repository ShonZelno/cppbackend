#pragma once
#include "dog.h"
#include "item_dog_provider.h"
#include "loot_generator.h"
#include "loot_generator_config.h"
#include "lost_object.h"
#include "map.h"
#include "model_invariants.h"
#include "tagged.h"
#include "ticker.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <vector>

namespace app {

namespace net = boost::asio;

class GameSession : public std::enable_shared_from_this<GameSession> {
public:
  using SessionStrand = net::strand<net::io_context::executor_type>;
  using Id = util::Tagged<std::string, GameSession>;
  using TimeInterval = std::chrono::milliseconds;
  using LostObjectIdHasher = util::TaggedHasher<model::LostObject::Id>;
  using LostObjects = std::unordered_map<model::LostObject::Id,
                                         std::shared_ptr<model::LostObject>,
                                         LostObjectIdHasher>;
  using DogIdHasher = util::TaggedHasher<model::Dog::Id>;
  using Dogs = std::unordered_map<model::Dog::Id, std::shared_ptr<model::Dog>,
                                  DogIdHasher>;
  using RetiredHandler = std::function<void(model::Dog::Id)>;

  GameSession(std::shared_ptr<model::Map> map,
              const TimeInterval &period_of_update_game_state,
              const model::LootGeneratorConfig &loot_gen_cfg,
              net::io_context &ioc, std::chrono::milliseconds retirement_time)
      : map_(map), ioc_(ioc),
        strand_(std::make_shared<SessionStrand>(net::make_strand(ioc_))),
        id_(*(map->GetId())),
        loot_generator_(
            TimeInterval(static_cast<uint64_t>(loot_gen_cfg.period *
                                               model::MILLISECONDS_IN_SECOND)),
            loot_gen_cfg.probability),
        period_of_update_game_state_(period_of_update_game_state),
        retirement_time_(retirement_time){};
  void Run();

  const Id &GetId() const noexcept;
  const std::shared_ptr<model::Map> GetMap();
  std::shared_ptr<SessionStrand> GetStrand();
  std::weak_ptr<model::Dog> CreateDog(const std::string &dog_name,
                                      const model::Map &map,
                                      bool randomize_spawn_points);
  void UpdateGameState(const TimeInterval &delta_time);
  const LostObjects &GetLostObjects();
  void AddLostObject(model::LostObject lost_object);
  void AddDog(std::shared_ptr<model::Dog> dog);

  void SetRetiredHandler(RetiredHandler handler) {
    on_dog_retired_ = std::move(handler);
  }

private:
  std::shared_ptr<model::Map> map_;
  net::io_context &ioc_;
  std::shared_ptr<SessionStrand> strand_;
  Id id_;
  loot_gen::LootGenerator loot_generator_;
  Dogs dogs_;
  LostObjects lost_objects_;
  TimeInterval period_of_update_game_state_;
  std::shared_ptr<time_m::Ticker> update_game_state_ticker_;
  std::shared_ptr<time_m::Ticker> generate_loot_ticker_;
  std::chrono::milliseconds retirement_time_;
  RetiredHandler on_dog_retired_;

  void GenerateLoot(const TimeInterval &delta_time);
  void CreateLostObject();
  void SetRandomLootType(std::shared_ptr<model::LostObject> loot);
  void LocateLootInRandomPositionOnMap(std::shared_ptr<model::LostObject> loot);
  void LocateDogInRandomPositionOnMap(std::shared_ptr<model::Dog> dog);
  void LocateDogInStartPointOnMap(std::shared_ptr<model::Dog> dog);
  void HandleLoot();
  void CollectLoot(const model::ItemDogProvider &provider, size_t item_id,
                   size_t gatherer_id);
  void DropLoot(const model::ItemDogProvider &provider, size_t item_id,
                size_t gatherer_id);
}

} // namespace app