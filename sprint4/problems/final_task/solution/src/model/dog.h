#pragma once
#include "collision_detector.h"
#include "geom.h"
#include "lost_object.h"
#include "model_invariants.h"
#include "support_types.h"
#include "tagged.h"

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace model {

class Dog {
  inline static size_t max_id_cont_ = 0;

public:
  using Id = util::Tagged<size_t, Dog>;
  using BagType = std::vector<std::shared_ptr<LostObject>>;

  Dog(std::string name, size_t bag_capacity)
      : id_(Id{Dog::max_id_cont_++}), name_(name), bag_capacity_(bag_capacity) {
  }

  Dog(Id id, std::string name, size_t bag_capacity)
      : id_(id), name_(name), bag_capacity_(bag_capacity) {
    if (*id_ >= Dog::max_id_cont_) {
      Dog::max_id_cont_ = *id_ + 1;
    }
  }

  Dog(const Dog &other) = default;
  Dog(Dog &&other) = default;
  Dog &operator=(const Dog &other) = default;
  Dog &operator=(Dog &&other) = default;
  virtual ~Dog() = default;

  const Id &GetId() const;
  const std::string &GetName() const;

  void SetDirection(Direction direction);
  const Direction GetDirection() const;

  void SetPosition(geom::Point2D position);
  const geom::Point2D &GetPosition() const;

  void SetVelocity(Velocity velocity);
  const Velocity &GetVelocity() const;

  void SetAction(Direction direction, double velocity);
  geom::Point2D
  CalculateNewPosition(const std::chrono::milliseconds &delta_time);

  const BagType &GetBag() const;
  size_t GetBagCapacity() const;
  void CollectLostObject(std::shared_ptr<LostObject> loot);
  bool IsFullBag() const;
  bool IsEmptyBag() const;
  void DropLostObjectsFromBag();

  const size_t GetScore() const;

  const collision_detector::Gatherer &AsGatherer() const;

  void UpdateIdleTime(std::chrono::milliseconds delta, bool is_moving);
  bool IsRetired(std::chrono::milliseconds retirement_time) const;
  void ResetIdleTime();

  std::chrono::milliseconds GetIdleTime() const {
    return idle_time_accumulated_;
  }

  void SetIdleTime(std::chrono::milliseconds time) {
    idle_time_accumulated_ = time;
  }

  bool HasMoveCommand() const { return direction_ != Direction::NONE; }

private:
  Id id_;
  std::string name_;
  Direction direction_{Direction::NORTH};
  geom::Point2D position_{0.0, 0.0};
  Velocity velocity_{0.0, 0.0};
  BagType bag_;
  collision_detector::Gatherer gatherer_{{0.0, 0.0}, {0.0, 0.0}, DOG_WIDTH};
  size_t score_{0};
  size_t bag_capacity_{0};
  std::chrono::milliseconds idle_time_accumulated_{0};

  void AddScore(size_t score);
}

} // namespace model