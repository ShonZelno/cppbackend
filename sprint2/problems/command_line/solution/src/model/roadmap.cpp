#include "roadmap.h"

#include <cmath>

#include <iostream>
#include <stdint.h>
#include <set>

namespace model
{

    /*Такой масштаб выбран, чтобы в одной клетке не было нескольких дорог без их наложения друг на друга
    (условие непрерывности маршрута если в клетке есть какая-нибудь дорога).*/
    const int SCALE_FACTOR_OF_CELL = 20; // Разбиваем карту на квадраты размером 0.05x0.05 папугаев.

    Roadmap::Roadmap(const Roadmap &other)
    {
        CopyContent(other.roads_);
    };

    Roadmap::Roadmap(Roadmap &&other)
    {
        matrix_map_ = std::move(other.matrix_map_);
        roads_ = std::move(other.roads_);
    };

    Roadmap &Roadmap::operator=(const Roadmap &other)
    {
        if (this != &other)
        {
            CopyContent(other.roads_);
        }
        return *this;
    };

    Roadmap &Roadmap::operator=(Roadmap &&other)
    {
        if (this != &other)
        {
            matrix_map_ = std::move(other.matrix_map_);
            roads_ = std::move(other.roads_);
        }
        return *this;
    };

    void Roadmap::ProcessRoad(const Road &road, size_t index, int64_t scaled_offset, bool is_horizontal)
    {
        // Получаем начальную и конечную точки
        auto start_point = is_horizontal ? (road.GetStart().x < road.GetEnd().x ? road.GetStart() : road.GetEnd()) : (road.GetStart().y < road.GetEnd().y ? road.GetStart() : road.GetEnd());

        auto end_point = is_horizontal ? (road.GetStart().x < road.GetEnd().x ? road.GetEnd() : road.GetStart()) : (road.GetStart().y < road.GetEnd().y ? road.GetEnd() : road.GetStart());

        // Вычисляем start и end координаты
        int64_t start, end, fixed_coord;

        if (is_horizontal)
        {
            start = static_cast<int64_t>(start_point.x);
            end = static_cast<int64_t>(end_point.x);
            fixed_coord = static_cast<int64_t>(road.GetStart().y);
        }
        else
        {
            start = static_cast<int64_t>(start_point.y);
            end = static_cast<int64_t>(end_point.y);
            fixed_coord = static_cast<int64_t>(road.GetStart().x);
        }

        // Масштабируем
        start = start * SCALE_FACTOR_OF_CELL - scaled_offset;
        end = end * SCALE_FACTOR_OF_CELL + scaled_offset;
        fixed_coord = fixed_coord * SCALE_FACTOR_OF_CELL;

        // Заполняем матрицу
        if (is_horizontal)
        {
            for (auto x = start; x <= end; ++x)
            {
                for (auto i = -scaled_offset; i <= scaled_offset; ++i)
                {
                    matrix_map_[x][fixed_coord + i].insert(index);
                }
            }
        }
        else
        {
            for (auto y = start; y <= end; ++y)
            {
                for (auto i = -scaled_offset; i <= scaled_offset; ++i)
                {
                    matrix_map_[fixed_coord + i][y].insert(index);
                }
            }
        }
    }

    void Roadmap::AddRoad(const Road &road)
    {
        const auto SCALLED_OFFSET = static_cast<int64_t>(OFFSET * SCALE_FACTOR_OF_CELL);
        auto index = roads_.size();
        roads_.emplace_back(road);

        if (road.IsHorizontal())
        {
            RoadScalling();
            ProcessRoad(road, index, SCALLED_OFFSET, true); // горизонтальная
        }
        else
        {
            ProcessRoad(road, index, SCALLED_OFFSET, false); // вертикальная
        }
    };

    const Roadmap::Roads &Roadmap::GetRoads() const noexcept
    {
        return roads_;
    };

    std::tuple<Position, Velocity> Roadmap::GetValidMove(const Position &old_position,
                                                         const Position &potential_new_position,
                                                         const Velocity &old_velocity)
    {
        Velocity velocity = {0, 0};
        auto start_roads = GetCoordinatesOfPosition(old_position);
        auto end_roads = GetCoordinatesOfPosition(potential_new_position);
        if (end_roads)
        {
            if (!IsValidPosition(matrix_map_[end_roads.value().x][end_roads.value().y],
                                 potential_new_position))
            {
                end_roads = std::nullopt;
            }
            else if (start_roads == end_roads)
            {
                return std::tie(potential_new_position, old_velocity);
            }
        }
        auto dest = GetDestinationRoadsOfRoute(start_roads, end_roads, old_velocity);
        Position position;
        if (dest && IsValidPosition(matrix_map_[dest.value().x][dest.value().y], potential_new_position))
        {
            position = potential_new_position;
            velocity = old_velocity;
        }
        else
        {
            position = GetFarestPoinOfRoute(dest.value(), old_position, old_velocity);
        }
        return std::tie(position, velocity);
    };

    template <bool IsXAxis>
    std::pair<int64_t, int64_t> Roadmap::CalculateMovement(const Velocity &old_velocity,
                                                           const std::pair<int64_t, int64_t> &start_coord,
                                                           const std::optional<std::pair<int64_t, int64_t>> &end)
    {
        constexpr auto getCoord = [](const std::pair<int64_t, int64_t> &p)
        {
            if constexpr (IsXAxis)
                return p.x;
            else
                return p.y;
        };

        constexpr auto makePair = [](int64_t main, int64_t other)
        {
            if constexpr (IsXAxis)
                return std::pair{main, other};
            else
                return std::pair{other, main};
        };

        int direction = std::signbit(IsXAxis ? old_velocity.vx : old_velocity.vy) ? -1 : 1;
        int64_t start = IsXAxis ? start_coord.x : start_coord.y;
        int64_t other = IsXAxis ? start_coord.y : start_coord.x;

        int64_t end_limit{0};
        if (end)
        {
            int64_t end_coord = getCoord(end.value()) * SCALE_FACTOR_OF_CELL;
            end_limit = (direction > 0) ? (end_coord < LLONG_MAX ? end_coord + 1 : LLONG_MAX) : end_coord - 1;
        }
        else
        {
            end_limit = (direction > 0) ? LLONG_MAX : -(OFFSET * SCALE_FACTOR_OF_CELL) - 1;
        }

        std::pair<int64_t, int64_t> current_coord = start_coord;
        for (int64_t index = start; index != end_limit; index += direction)
        {
            auto current = makePair(index, other);

            if (ValidateCoordinates(current) &&
                IsCrossedSets(
                    matrix_map_[start_coord.x][start_coord.y],
                    matrix_map_[current.first][current.second]))
            {
                current_coord = current;
            }
            else
            {
                break;
            }
        }

        return current_coord;
    }

    std::optional<const Roadmap::MatrixMapCoord> Roadmap::GetDestinationRoadsOfRoute(
    std::optional<const MatrixMapCoord> start,
    std::optional<const MatrixMapCoord> end,
    const Velocity& old_velocity)
{
    if (!start) return std::nullopt;
    
    const MatrixMapCoord start_coord = start.value();
    MatrixMapCoord current_coord = start_coord;
    
    bool is_x_axis = old_velocity.vx != 0;
    int64_t velocity_component = is_x_axis ? old_velocity.vx : old_velocity.vy;
    
    if (velocity_component == 0) return std::nullopt;
    
    int direction = std::signbit(velocity_component) ? -1 : 1;
    int64_t end_index{0};
    
    if (end)
    {
        int64_t end_coord = is_x_axis ? end.value().x : end.value().y;
        end_index = end_coord * SCALE_FACTOR_OF_CELL;
        end_index = (direction > 0) ? 
            (end_index < LLONG_MAX ? end_index + 1 : LLONG_MAX) : 
            end_index - 1;
    }
    else
    {
        end_index = (direction > 0) ? LLONG_MAX : 
            -(OFFSET * SCALE_FACTOR_OF_CELL) - 1;
    }
    
    int64_t start_index = is_x_axis ? start_coord.x : start_coord.y;
    
    for (int64_t index = start_index; index != end_index; index += direction)
    {
        MatrixMapCoord check_coord;
        if (is_x_axis)
        {
            check_coord = {index, start_coord.y};
        }
        else
        {
            check_coord = {start_coord.x, index};
        }
        
        if (ValidateCoordinates(check_coord) &&
            IsCrossedSets(matrix_map_[start_coord.x][start_coord.y],
                          matrix_map_[check_coord.x][check_coord.y]))
        {
            if (is_x_axis)
            {
                current_coord.x = index;
            }
            else
            {
                current_coord.y = index;
            }
        }
        else
        {
            break;
        }
    }
    
    return current_coord;
}

    std::optional<const Roadmap::MatrixMapCoord> Roadmap::GetCoordinatesOfPosition(const Position &position)
    {
        if (position.x < -OFFSET - EPSILON || position.y < -OFFSET - EPSILON)
        {
            return std::nullopt;
        }
        int64_t x_index = (position.x >= 0) ? std::floor(position.x * SCALE_FACTOR_OF_CELL) : std::ceil(position.x * SCALE_FACTOR_OF_CELL);
        int64_t y_index = (position.y >= 0) ? std::floor(position.y * SCALE_FACTOR_OF_CELL) : std::ceil(position.y * SCALE_FACTOR_OF_CELL);
        if (matrix_map_.contains(x_index))
        {
            if (matrix_map_[x_index].contains(y_index))
            {
                return MatrixMapCoord{x_index, y_index};
            }
        }
        return std::nullopt;
    };

    bool Roadmap::IsCrossedSets(const std::unordered_set<size_t> &lhs,
                                const std::unordered_set<size_t> &rhs)
    {
        for (auto item : lhs)
        {
            if (rhs.contains(item))
            {
                return true;
            }
        }
        return false;
    };

    bool Roadmap::ValidateCoordinates(const MatrixMapCoord &coordinates)
    {
        if (matrix_map_.contains(coordinates.x))
        {
            return matrix_map_[coordinates.x].contains(coordinates.y);
        }
        return false;
    };

    const Position Roadmap::GetFarestPoinOfRoute(const MatrixMapCoord &roads_coord,
                                                 const Position &old_position,
                                                 const Velocity &old_velocity)
    {
        Position res_position{old_position};
        auto cell_pos = MatrixCoordinateToPosition(roads_coord, old_position);
        auto direction = VelocityToDirection(old_velocity);
        for (auto road_ind : matrix_map_[roads_coord.x][roads_coord.y])
        {
            auto start_position = cell_pos.at(DIRECTION_TO_OPOSITE_DIRECTION.at(direction));
            auto end_position = cell_pos.at(direction);
            if (IsValidPositionOnRoad(roads_[road_ind], start_position))
            {
                if (IsValidPositionOnRoad(roads_[road_ind], end_position))
                {
                    return end_position;
                }
                res_position = start_position;
            }
        }
        return res_position;
    };

    const std::unordered_map<Direction, Position> Roadmap::MatrixCoordinateToPosition(const MatrixMapCoord &coord,
                                                                                      const Position &target_position)
    {
        std::unordered_map<Direction, Position> res;
        int64_t x_inc_e = (coord.x < 0) ? 0 : 1;
        int64_t y_inc_s = (coord.y < 0) ? 0 : 1;
        int64_t x_inc_w = (coord.x < 0) ? -1 : 0;
        int64_t y_inc_n = (coord.y < 0) ? -1 : 0;
        res[Direction::NORTH] = Position{
            target_position.x,
            (static_cast<double>(coord.y + y_inc_n) / static_cast<double>(SCALE_FACTOR_OF_CELL))};
        res[Direction::SOUTH] = Position{
            target_position.x,
            (static_cast<double>(coord.y + y_inc_s) / static_cast<double>(SCALE_FACTOR_OF_CELL))};
        res[Direction::WEST] = Position{
            (static_cast<double>(coord.x + x_inc_w) / static_cast<double>(SCALE_FACTOR_OF_CELL)),
            target_position.y};
        res[Direction::EAST] = Position{
            (static_cast<double>(coord.x + x_inc_e) / static_cast<double>(SCALE_FACTOR_OF_CELL)),
            target_position.y};
        res[Direction::NONE] = Position{target_position.x, target_position.y};
        return res;
    }

    const Direction Roadmap::VelocityToDirection(const Velocity &velocity)
    {
        Velocity vel{0, 0};
        if (velocity.vx != 0)
        {
            vel.vx = std::signbit(velocity.vx) ? -1 : 1;
        }
        if (velocity.vy != 0)
        {
            vel.vy = std::signbit(velocity.vy) ? -1 : 1;
        }
        return VELOCITY_TO_DIRECTION.at(vel);
    }

    bool Roadmap::IsValidPosition(const std::unordered_set<size_t> &roads_ind, const Position &position)
    {
        for (auto road_index : roads_ind)
        {
            if (IsValidPositionOnRoad(roads_[road_index], position))
            {
                return true;
            }
        }
        return false;
    };

    RectBounds Roadmap::CalculateRoadBounds(const Road &road, double offset)
    {
        RectBounds bounds;
        if (road.IsHorizontal())
        {
            bounds.start_x = std::min(road.GetStart().x, road.GetEnd().x) - offset;
            bounds.end_x = std::max(road.GetStart().x, road.GetEnd().x) + offset;
            bounds.start_y = road.GetStart().y - offset;
            bounds.end_y = road.GetStart().y + offset;
        }
        else
        {
            bounds.start_y = std::min(road.GetStart().y, road.GetEnd().y) - offset;
            bounds.end_y = std::max(road.GetStart().y, road.GetEnd().y) + offset;
            bounds.start_x = road.GetStart().x - offset;
            bounds.end_x = road.GetStart().x + offset;
        }

        return bounds;
    }

    bool Roadmap::IsValidPositionOnRoad(const Road &road, const Position &position)
    {
        auto bounds = CalculateRoadBounds(road, OFFSET);
        return ((position.x > bounds.start_x) || (std::abs(position.x - bounds.start_x) < EPSILON)) &&
               ((position.x < bounds.end_x) || (std::abs(position.x - bounds.end_x) < EPSILON)) &&
               ((position.y > bounds.start_y) || (std::abs(position.y - bounds.start_y) < EPSILON)) &&
               ((position.y < bounds.end_y) || (std::abs(position.y - bounds.end_y) < EPSILON));
    };

    void Roadmap::CopyContent(const Roadmap::Roads &roads)
    {
        for (auto &road : roads)
        {
            AddRoad(road);
        }
    };

}