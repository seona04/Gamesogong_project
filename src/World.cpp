#include "World.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace island {
namespace {

Direction Opposite(Direction direction) {
    switch (direction) {
    case Direction::North: return Direction::South;
    case Direction::East: return Direction::West;
    case Direction::South: return Direction::North;
    case Direction::West: return Direction::East;
    }
    return Direction::North;
}

} // namespace

World::World(std::array<Location, LocationCount> locations,
             std::vector<WorldConnection> connections)
    : locations_(std::move(locations)), connections_(std::move(connections)) {}

World World::CreateHaemuIsland() {
    std::array<Location, LocationCount> locations = {{
        {LocationId::Home, L"부모님의 집", L"언덕 위의 낡은 집과 손볼 곳이 많은 정원입니다.", {155, 205}},
        {LocationId::Harbor, L"해무 포구", L"새벽 시장이 열리는 섬의 중심 포구입니다.", {395, 350}},
        {LocationId::PineForest, L"솔숲길", L"집과 마을을 잇는 숲길입니다. 정원에 쓸 재료를 찾을 수 있습니다.", {245, 475}},
        {LocationId::TidalFlat, L"물비늘 갯벌", L"물이 빠지면 조개와 작은 바다 생물을 살펴볼 수 있습니다.", {585, 450}},
        {LocationId::WindCape, L"바람곶", L"바람 센 절벽과 부모님의 기록이 남은 오래된 등대가 있습니다.", {620, 205}},
        {LocationId::River, L"달빛강", L"솔숲에서 흘러온 맑은 강입니다. 은어와 송어가 물살을 거슬러 오릅니다.", {285, 315}}
    }};

    std::vector<WorldConnection> connections = {
        {LocationId::Home, LocationId::Harbor, Direction::East, RouteAccess::Always},
        {LocationId::Home, LocationId::PineForest, Direction::South, RouteAccess::Always},
        {LocationId::Harbor, LocationId::WindCape, Direction::East, RouteAccess::Always},
        {LocationId::Harbor, LocationId::TidalFlat, Direction::South, RouteAccess::LowTideOnly},
        {LocationId::PineForest, LocationId::TidalFlat, Direction::East, RouteAccess::LowTideOnly},
        {LocationId::PineForest, LocationId::River, Direction::North, RouteAccess::Always},
        {LocationId::River, LocationId::Harbor, Direction::East, RouteAccess::Always}
    };

    return World(std::move(locations), std::move(connections));
}

const Location& World::GetLocation(LocationId id) const {
    const std::size_t index = static_cast<std::size_t>(id);
    if (index >= locations_.size()) {
        throw std::out_of_range("Unknown island location.");
    }
    return locations_[index];
}

std::vector<LocationId> World::GetNeighbors(LocationId id) const {
    std::vector<LocationId> neighbors;
    for (const WorldConnection& connection : connections_) {
        if (connection.first == id) {
            neighbors.push_back(connection.second);
        } else if (connection.second == id) {
            neighbors.push_back(connection.first);
        }
    }
    return neighbors;
}

const WorldConnection* World::FindConnection(LocationId from, LocationId to) const {
    const auto connection = std::find_if(
        connections_.begin(), connections_.end(),
        [from, to](const WorldConnection& candidate) {
            return (candidate.first == from && candidate.second == to) ||
                   (candidate.first == to && candidate.second == from);
        });
    return connection == connections_.end() ? nullptr : &*connection;
}

std::optional<LocationId> World::GetNeighborInDirection(LocationId id, Direction direction) const {
    for (const WorldConnection& connection : connections_) {
        if (connection.first == id && connection.directionFromFirst == direction) {
            return connection.second;
        }
        if (connection.second == id && Opposite(connection.directionFromFirst) == direction) {
            return connection.first;
        }
    }
    return std::nullopt;
}

Direction World::GetTravelDirection(LocationId from, LocationId to) const {
    const WorldConnection* connection = FindConnection(from, to);
    if (connection == nullptr) {
        throw std::invalid_argument("There is no route between those locations.");
    }
    return connection->first == from ? connection->directionFromFirst : Opposite(connection->directionFromFirst);
}

bool World::CanTravel(LocationId from, LocationId to, TidePhase tide) const {
    const WorldConnection* connection = FindConnection(from, to);
    if (connection == nullptr) {
        return false;
    }
    return connection->access == RouteAccess::Always || tide == TidePhase::Low;
}

const wchar_t* ToString(TidePhase tide) {
    switch (tide) {
    case TidePhase::Low: return L"간조";
    case TidePhase::Rising: return L"들물";
    case TidePhase::High: return L"만조";
    case TidePhase::Falling: return L"날물";
    }
    return L"알 수 없음";
}

const wchar_t* ToString(Direction direction) {
    switch (direction) {
    case Direction::North: return L"위쪽";
    case Direction::East: return L"오른쪽";
    case Direction::South: return L"아래쪽";
    case Direction::West: return L"왼쪽";
    }
    return L"";
}

TidePhase NextTide(TidePhase tide) {
    switch (tide) {
    case TidePhase::Low: return TidePhase::Rising;
    case TidePhase::Rising: return TidePhase::High;
    case TidePhase::High: return TidePhase::Falling;
    case TidePhase::Falling: return TidePhase::Low;
    }
    return TidePhase::Low;
}

} // namespace island
