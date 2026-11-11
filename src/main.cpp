#include "World.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {

bool TryReadChoice(const std::string& line, int& choice) {
    std::istringstream input(line);
    if (!(input >> choice)) {
        return false;
    }
    input >> std::ws;
    return input.eof();
}

void PrintLocation(const island::World& world, island::LocationId locationId) {
    const island::Location& location = world.GetLocation(locationId);
    std::cout << "\n== " << location.name << " ==\n";
    std::cout << location.description << "\n";
}

void PrintRoutes(const island::World& world,
                 island::LocationId& current,
                 island::TidePhase tide) {
    const std::vector<island::LocationId> neighbors = world.GetNeighbors(current);
    std::cout << "\n이동할 장소를 고르세요:\n";

    for (std::size_t index = 0; index < neighbors.size(); ++index) {
        const island::LocationId destination = neighbors[index];
        std::cout << "  " << index + 1 << ". " << world.GetLocation(destination).name;
        if (world.CanTravel(current, destination, tide)) {
            std::cout << " [이동 가능]\n";
        } else {
            std::cout << " [간조에 개방]\n";
        }
    }

    if (neighbors.empty()) {
        std::cout << "  연결된 길이 없습니다.\n";
        return;
    }

    std::cout << "선택 (0: 취소): ";
    std::string line;
    if (!std::getline(std::cin, line)) {
        return;
    }

    int choice = 0;
    if (!TryReadChoice(line, choice) || choice < 0 || choice > static_cast<int>(neighbors.size())) {
        std::cout << "올바른 번호를 입력하세요.\n";
        return;
    }
    if (choice == 0) {
        return;
    }

    const island::LocationId destination = neighbors[static_cast<std::size_t>(choice - 1)];
    if (!world.CanTravel(current, destination, tide)) {
        std::cout << "물이 빠지는 간조 때 다시 와야 합니다.\n";
        return;
    }

    current = destination;
    PrintLocation(world, destination);
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    const island::World world = island::World::CreateHaemuIsland();
    island::LocationId current = island::LocationId::Home;
    island::TidePhase tide = island::TidePhase::Low;

    std::cout << "파도 끝의 집 - 해무섬 월드 프로토타입\n";
    PrintLocation(world, current);

    while (true) {
        std::cout << "\n[물때: " << island::ToString(tide) << "]\n";
        std::cout << "1. 섬 지도 보기\n";
        std::cout << "2. 주변 길과 이동\n";
        std::cout << "3. 다음 물때로 진행\n";
        std::cout << "0. 게임 종료\n";
        std::cout << "선택: ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        int choice = -1;
        if (!TryReadChoice(line, choice)) {
            std::cout << "숫자로 메뉴를 선택하세요.\n";
            continue;
        }

        switch (choice) {
        case 1:
            world.PrintMap(std::cout, tide, current);
            break;
        case 2:
            PrintRoutes(world, current, tide);
            break;
        case 3:
            tide = island::NextTide(tide);
            std::cout << "물때가 " << island::ToString(tide) << "로 바뀌었습니다.\n";
            break;
        case 0:
            std::cout << "해무섬의 하루를 마칩니다.\n";
            return 0;
        default:
            std::cout << "0부터 3까지 선택하세요.\n";
            break;
        }
    }

    return 0;
}
