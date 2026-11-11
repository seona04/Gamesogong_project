#pragma once

#include <array>
#include <cstddef>
#include <iosfwd>
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
    char mapSymbol;
    std::string name;
    std::string description;
    MapPosition position;
};

struct WorldConnection {
    LocationId first;
    LocationId second;
    RouteAccess access;
};

class World {
public:
    static World CreateHaemuIsland();

    const Location& GetLocation(LocationId id) const;
    std::vector<LocationId> GetNeighbors(LocationId id) const;
    bool CanTravel(LocationId from, LocationId to, TidePhase tide) const;
    RouteAccess GetRouteAccess(LocationId from, LocationId to) const;
    void PrintMap(std::ostream& output, TidePhase tide, LocationId current) const;

private:
    static constexpr std::size_t LocationCount = static_cast<std::size_t>(LocationId::Count);

    World(std::array<Location, LocationCount> locations,
          std::vector<WorldConnection> connections);

    const WorldConnection* FindConnection(LocationId from, LocationId to) const;

    std::array<Location, LocationCount> locations_;
    std::vector<WorldConnection> connections_;
};

const char* ToString(TidePhase tide);
TidePhase NextTide(TidePhase tide);

} // namespace island
