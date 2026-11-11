#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace island {

enum class TidePhase {
    Low,
    Rising,
    High,
    Falling
};

enum class LocationId : std::size_t {
    Home,
    Harbor,
    PineForest,
    TidalFlat,
    WindCape,
    Count
};

enum class Direction {
    North,
    East,
    South,
    West
};

enum class RouteAccess {
    Always,
    LowTideOnly
};

struct MapPosition {
    int x;
    int y;
};

struct Location {
    LocationId id;
    std::wstring name;
    std::wstring description;
    MapPosition position;
};

struct WorldConnection {
    LocationId first;
    LocationId second;
    Direction directionFromFirst;
    RouteAccess access;
};

class World {
public:
    static World CreateHaemuIsland();

    const Location& GetLocation(LocationId id) const;
    std::vector<LocationId> GetNeighbors(LocationId id) const;
    std::optional<LocationId> GetNeighborInDirection(LocationId id, Direction direction) const;
    Direction GetTravelDirection(LocationId from, LocationId to) const;
    bool CanTravel(LocationId from, LocationId to, TidePhase tide) const;

private:
    static constexpr std::size_t LocationCount = static_cast<std::size_t>(LocationId::Count);

    World(std::array<Location, LocationCount> locations,
          std::vector<WorldConnection> connections);

    const WorldConnection* FindConnection(LocationId from, LocationId to) const;

    std::array<Location, LocationCount> locations_;
    std::vector<WorldConnection> connections_;
};

const wchar_t* ToString(TidePhase tide);
const wchar_t* ToString(Direction direction);
TidePhase NextTide(TidePhase tide);

} // namespace island
