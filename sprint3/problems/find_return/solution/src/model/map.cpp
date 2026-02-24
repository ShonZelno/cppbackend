#include "map.h"
#include "model_key_storage.h"

#include <stdexcept>
#include <iostream>

namespace model
{
    using namespace std::literals;

    const Map::Id &Map::GetId() const noexcept
    {
        return id_;
    }

    const std::string &Map::GetName() const noexcept
    {
        return name_;
    }

    const Map::Buildings &Map::GetBuildings() const noexcept
    {
        return buildings_;
    }

    const Map::Roads &Map::GetRoads() const noexcept
    {
        return roadmap_.GetRoads();
    }

    const Map::Offices &Map::GetOffices() const noexcept
    {
        return offices_;
    }

    const Map::LootTypes &Map::GetLootTypes() const noexcept
    {
        return loot_types_;
    }

    void Map::AddRoad(const Road &road)
    {
        roadmap_.AddRoad(road);
    }

    void Map::AddRoads(const Roads &roads)
    {
        for (auto item : roads)
        {
            AddRoad(item);
        }
    }

    void Map::AddBuilding(const Building &building)
    {
        buildings_.emplace_back(building);
    }

    void Map::AddBuildings(const Buildings &buildings)
    {
        for (auto item : buildings)
        {
            AddBuilding(item);
        }
    }

    void Map::AddOffice(const Office &office)
    {
        if (warehouse_id_to_index_.contains(office.GetId()))
        {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office &o = offices_.emplace_back(office);
        try
        {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        }
        catch (...)
        {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }

    void Map::AddOffices(const Offices &offices)
    {
        for (auto item : offices)
        {
            AddOffice(item);
        }
    }

    void Map::SetDogVelocity(const double velocity)
    {
        dog_velocity_ = std::abs(velocity);
    };

    double Map::GetDogVelocity() const noexcept
    {
        return dog_velocity_ ? dog_velocity_.value() : INITIAL_DOG_VELOCITY;
    };

    void Map::AddLootType(const LootType &loot_type)
    {
        loot_types_.emplace_back(loot_type);
    };

    void Map::AddLootTypes(const Map::LootTypes &loot_types)
    {
        for (auto item : loot_types)
        {
            AddLootType(item);
        }
    };

    std::tuple<Position, Velocity> Map::GetValidMove(const Position &old_position,
                                                     const Position &potential_new_position,
                                                     const Velocity &old_velocity)
    {
        return roadmap_.GetValidMove(old_position, potential_new_position, old_velocity);
    };

    Position Map::GenerateRandomPosition() const
    {
        return roadmap_.GenerateValidRandomPosition();
    };

    size_t Map::GetNumberOfLootTypes() const
    {
        return loot_types_.size();
    };

    void Map::SetLootGenerator(loot_gen::LootGenerator generator)
    {
        loot_generator_ = std::move(generator);
    }

    void Map::GenerateLoot(std::chrono::milliseconds time_delta, size_t dogs_count)
    {

        unsigned new_loot = loot_generator_.Generate(
            time_delta,
            lost_objects_.size(),
            dogs_count);

        for (unsigned i = 0; i < new_loot; ++i)
        {
            lost_objects_.push_back(GenerateRandomLoot());
        }
    }

    const std::vector<LostObject> &Map::GetLostObjects() const
    {
        return lost_objects_;
    }

    void Map::ClearLostObjects()
    {
        lost_objects_.clear();
    }

    LostObject Map::GenerateRandomLoot()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        // Генерируем случайную позицию на дорогах
        Position pos = GenerateRandomPosition();

        // Выбираем случайный тип трофея
        size_t num_types = loot_types_.size();
        if (num_types == 0)
        {
            throw EmptyLootTypesOnMapException(*GetId());
        }

        std::uniform_int_distribution<> type_dist(0, num_types - 1);

        return LostObject(static_cast<size_t>(type_dist(gen)), pos.x, pos.y);
    }

    void Map::SetBagCapacity(size_t bag_capacity)
    {
        bag_capacity_ = bag_capacity;
    };

    size_t Map::GetBagCapacity() const noexcept
    {
        return bag_capacity_ ? bag_capacity_.value() : INITIAL_BAG_CAPACITY;
    };

} // namespace model
