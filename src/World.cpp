#include "World.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace island {
namespace {

constexpr int MapWidth = 30;
constexpr int MapHeight = 12;

void DrawRoute(std::array<std::string, MapHeight>& map,
               MapPosition start,
               MapPosition end,
               char routeSymbol) {
    int x = start.x;
    int y = start.y;
    const int deltaX = std::abs(end.x - start.x);
    const int stepX = start.x < end.x ? 1 : -1;
    const int deltaY = -std::abs(end.y - start.y);
    const int stepY = start.y < end.y ? 1 : -1;
    int error = deltaX + deltaY;

    while (x != end.x || y != end.y) {
        if ((x != start.x || y != start.y) && map[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] == ' ') {
            map[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = routeSymbol;
        }

        const int doubledError = 2 * error;
        if (doubledError >= deltaY) {
            error += deltaY;
            x += stepX;
        }
        if (doubledError <= deltaX) {
            error += deltaX;
            y += stepY;
        }
    }
}

} // namespace

World::World(std::array<Location, LocationCount> locations,
             std::vector<WorldConnection> connections)
    : locations_(std::move(locations)), connections_(std::move(connections)) {}

World World::CreateHaemuIsland() {
    std::array<Location, LocationCount> locations = {{
        {LocationId::Home, 'H', "부모님의 집", "언덕 위의 낡은 집과 손볼 곳이 많은 정원입니다.", {3, 2}},
        {LocationId::Harbor, 'M', "해무 포구", "새벽 시장이 열리는 섬의 중심 포구입니다.", {12, 5}},
        {LocationId::PineForest, 'F', "솔숲길", "집과 마을을 잇는 숲길입니다. 정원에 쓸 재료를 찾을 수 있습니다.", {5, 9}},
        {LocationId::TidalFlat, 'T', "물비늘 갯벌", "물이 빠지면 조개와 작은 바다 생물을 살펴볼 수 있습니다.", {20, 8}},
        {LocationId::WindCape, 'W', "바람곶", "바람 센 절벽과 오래된 등대가 서 있는 곳입니다.", {25, 2}}
    }};

    std::vector<WorldConnection> connections = {
        {LocationId::Home, LocationId::Harbor, RouteAccess::Always},
        {LocationId::Home, LocationId::PineForest, RouteAccess::Always},
        {LocationId::Harbor, LocationId::PineForest, RouteAccess::Always},
        {LocationId::Harbor, LocationId::TidalFlat, RouteAccess::LowTideOnly},
        {LocationId::PineForest, LocationId::TidalFlat, RouteAccess::LowTideOnly},
        {LocationId::Harbor, LocationId::WindCape, RouteAccess::Always}
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

RouteAccess World::GetRouteAccess(LocationId from, LocationId to) const {
    const WorldConnection* connection = FindConnection(from, to);
    if (connection == nullptr) {
        throw std::invalid_argument("There is no route between those locations.");
    }
    return connection->access;
}

bool World::CanTravel(LocationId from, LocationId to, TidePhase tide) const {
    const WorldConnection* connection = FindConnection(from, to);
    if (connection == nullptr) {
        return false;
    }
    return connection->access == RouteAccess::Always || tide == TidePhase::Low;
}

void World::PrintMap(std::ostream& output, TidePhase tide, LocationId current) const {
    std::array<std::string, MapHeight> map;
    map.fill(std::string(MapWidth, ' '));

    for (const WorldConnection& connection : connections_) {
        const Location& first = GetLocation(connection.first);
        const Location& second = GetLocation(connection.second);
        const char routeSymbol = connection.access == RouteAccess::LowTideOnly ? '~' : '.';
        DrawRoute(map, first.position, second.position, routeSymbol);
    }

    for (const Location& location : locations_) {
        map[static_cast<std::size_t>(location.position.y)][static_cast<std::size_t>(location.position.x)] = location.mapSymbol;
    }

    output << "\n[해무섬 지도] 현재 물때: " << ToString(tide) << "\n";
    output << "    0         1         2         3\n";
    output << "    012345678901234567890123456789\n";
    for (int y = 0; y < MapHeight; ++y) {
        output << std::setw(2) << y << " |" << map[static_cast<std::size_t>(y)] << "|\n";
    }

    output << "기호: H 부모님의 집, M 해무 포구, F 솔숲길, T 물비늘 갯벌, W 바람곶\n";
    output << "경로: . 상시 개방, ~ 간조에만 개방\n";
    output << "현재 위치: " << GetLocation(current).name << "\n";
}

const char* ToString(TidePhase tide) {
    switch (tide) {
    case TidePhase::Low: return "간조";
    case TidePhase::Rising: return "들물";
    case TidePhase::High: return "만조";
    case TidePhase::Falling: return "날물";
    }
    return "알 수 없는 물때";
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
