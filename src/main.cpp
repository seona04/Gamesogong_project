#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "World.h"

namespace {

constexpr wchar_t WindowClassName[] = L"GamesogongIslandWindow";
constexpr int WorldMapWidth = 720;
constexpr int WorldMapHeight = 580;
constexpr int SidePanelWidth = 330;
constexpr int HomeCanvasWidth = 1200;
constexpr int HomeCanvasHeight = 800;

enum class SceneMode {
    IslandMap,
    HomeExterior,
    HomeInterior
};

island::World g_world = island::World::CreateHaemuIsland();
island::LocationId g_currentLocation = island::LocationId::Home;
island::TidePhase g_tide = island::TidePhase::Low;
SceneMode g_scene = SceneMode::IslandMap;
std::wstring g_status = L"해무섬에 도착했습니다. 길을 따라 섬을 둘러보세요.";
std::wstring g_homeStatus = L"낡은 울타리와 돌보지 못한 화단이 보입니다.";
std::wstring g_interiorStatus = L"부모님의 오래된 집 안입니다.";
int g_homePlayerX = 660;
int g_homePlayerY = 710;
int g_interiorPlayerX = 600;
int g_interiorPlayerY = 620;
bool g_homeFenceRepaired = false;
bool g_flowerPlanterPlaced = false;

struct MapLayout {
    int left;
    int top;
    double scaleX;
    double scaleY;
    int panelLeft;
};

POINT ToScreen(const MapLayout& layout, island::MapPosition position) {
    return POINT{
        layout.left + static_cast<int>(position.x * layout.scaleX),
        layout.top + static_cast<int>(position.y * layout.scaleY)
    };
}

void DrawTextBlock(HDC dc,
                   const std::wstring& text,
                   RECT bounds,
                   COLORREF color = RGB(42, 54, 49),
                   int fontHeight = 20,
                   bool bold = false,
                   UINT format = DT_LEFT | DT_TOP | DT_WORDBREAK) {
    HFONT font = CreateFontW(
        fontHeight, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Malgun Gothic");
    HGDIOBJ oldFont = SelectObject(dc, font);
    const int oldMode = SetBkMode(dc, TRANSPARENT);
    const COLORREF oldColor = SetTextColor(dc, color);
    DrawTextW(dc, text.c_str(), -1, &bounds, format);
    SetTextColor(dc, oldColor);
    SetBkMode(dc, oldMode);
    SelectObject(dc, oldFont);
    DeleteObject(font);
}

void DrawTextAt(HDC dc,
                const std::wstring& text,
                int x,
                int y,
                COLORREF color = RGB(42, 54, 49),
                int fontHeight = 18,
                bool bold = false) {
    RECT bounds{x, y, x + 700, y + fontHeight + 8};
    DrawTextBlock(dc, text, bounds, color, fontHeight, bold, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void FillRectangle(HDC dc,
                   RECT bounds,
                   COLORREF fill,
                   COLORREF outline,
                   int outlineWidth = 1) {
    HPEN pen = CreatePen(PS_SOLID, outlineWidth, outline);
    HBRUSH brush = CreateSolidBrush(fill);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    Rectangle(dc, bounds.left, bounds.top, bounds.right, bounds.bottom);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void FillEllipse(HDC dc,
                 int left,
                 int top,
                 int right,
                 int bottom,
                 COLORREF fill,
                 COLORREF outline,
                 int outlineWidth = 1) {
    HPEN pen = CreatePen(PS_SOLID, outlineWidth, outline);
    HBRUSH brush = CreateSolidBrush(fill);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    Ellipse(dc, left, top, right, bottom);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawRing(HDC dc,
              int left,
              int top,
              int right,
              int bottom,
              COLORREF color,
              int width = 3) {
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Ellipse(dc, left, top, right, bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void FillPolygon(HDC dc,
                 const std::vector<POINT>& points,
                 COLORREF fill,
                 COLORREF outline,
                 int outlineWidth = 1) {
    HPEN pen = CreatePen(PS_SOLID, outlineWidth, outline);
    HBRUSH brush = CreateSolidBrush(fill);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    Polygon(dc, points.data(), static_cast<int>(points.size()));
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawLine(HDC dc,
              POINT start,
              POINT end,
              COLORREF color,
              int width = 2,
              int style = PS_SOLID) {
    HPEN pen = CreatePen(style, width, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    MoveToEx(dc, start.x, start.y, nullptr);
    LineTo(dc, end.x, end.y);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void DrawPineTree(HDC dc, int x, int y, int size) {
    FillRectangle(dc, RECT{x - size / 10, y, x + size / 10, y + size / 3}, RGB(111, 75, 48), RGB(91, 62, 41));
    FillPolygon(dc,
                {{x, y - size}, {x - size / 2, y - size / 4}, {x - size / 4, y - size / 4},
                 {x - size * 2 / 3, y + size / 5}, {x + size * 2 / 3, y + size / 5},
                 {x + size / 4, y - size / 4}, {x + size / 2, y - size / 4}},
                RGB(49, 123, 77), RGB(41, 103, 67), 2);
}

void DrawHouse(HDC dc, POINT center) {
    FillRectangle(dc, RECT{center.x - 35, center.y - 7, center.x + 35, center.y + 38}, RGB(244, 221, 171), RGB(137, 100, 66), 2);
    FillPolygon(dc,
                {{center.x - 43, center.y - 8}, {center.x, center.y - 42}, {center.x + 43, center.y - 8}},
                RGB(165, 85, 65), RGB(125, 63, 52), 2);
    FillRectangle(dc, RECT{center.x - 9, center.y + 12, center.x + 9, center.y + 38}, RGB(116, 79, 57), RGB(94, 64, 47));
    FillRectangle(dc, RECT{center.x - 27, center.y + 2, center.x - 14, center.y + 15}, RGB(155, 208, 219), RGB(115, 158, 164));
    FillRectangle(dc, RECT{center.x + 14, center.y + 2, center.x + 27, center.y + 15}, RGB(155, 208, 219), RGB(115, 158, 164));
    FillRectangle(dc, RECT{center.x - 48, center.y + 43, center.x + 48, center.y + 55}, RGB(111, 158, 80), RGB(111, 158, 80));
    DrawLine(dc, POINT{center.x - 38, center.y + 47}, POINT{center.x + 38, center.y + 47}, RGB(211, 197, 136), 2);
}

void DrawHarbor(HDC dc, POINT center) {
    DrawLine(dc, POINT{center.x - 46, center.y + 31}, POINT{center.x + 53, center.y + 31}, RGB(124, 83, 55), 9);
    DrawLine(dc, POINT{center.x - 24, center.y + 31}, POINT{center.x - 24, center.y + 56}, RGB(124, 83, 55), 5);
    DrawLine(dc, POINT{center.x + 20, center.y + 31}, POINT{center.x + 20, center.y + 56}, RGB(124, 83, 55), 5);
    const COLORREF awningColors[] = {RGB(213, 109, 86), RGB(234, 191, 105), RGB(102, 155, 123)};
    for (int index = 0; index < 3; ++index) {
        const int x = center.x - 47 + index * 34;
        FillRectangle(dc, RECT{x, center.y - 17, x + 29, center.y + 17}, RGB(246, 224, 178), RGB(137, 103, 68), 2);
        FillRectangle(dc, RECT{x - 3, center.y - 24, x + 32, center.y - 14}, awningColors[index], RGB(137, 103, 68), 1);
        FillRectangle(dc, RECT{x + 9, center.y - 5, x + 19, center.y + 17}, RGB(119, 83, 58), RGB(119, 83, 58));
    }
}

void DrawForest(HDC dc, POINT center) {
    DrawPineTree(dc, center.x - 35, center.y + 12, 54);
    DrawPineTree(dc, center.x + 2, center.y - 1, 68);
    DrawPineTree(dc, center.x + 40, center.y + 12, 53);
    FillEllipse(dc, center.x - 63, center.y + 36, center.x + 65, center.y + 59, RGB(145, 190, 105), RGB(145, 190, 105));
}

void DrawTidalFlat(HDC dc, POINT center) {
    FillEllipse(dc, center.x - 55, center.y - 28, center.x + 55, center.y + 35, RGB(216, 203, 151), RGB(188, 170, 119), 2);
    DrawLine(dc, POINT{center.x - 44, center.y - 6}, POINT{center.x + 37, center.y - 6}, RGB(100, 176, 190), 3);
    DrawLine(dc, POINT{center.x - 28, center.y + 9}, POINT{center.x + 48, center.y + 9}, RGB(100, 176, 190), 2);
    FillEllipse(dc, center.x - 21, center.y + 15, center.x - 10, center.y + 26, RGB(230, 141, 118), RGB(194, 112, 96));
    FillEllipse(dc, center.x + 13, center.y + 19, center.x + 25, center.y + 31, RGB(236, 220, 175), RGB(173, 155, 117));
}

void DrawLighthouse(HDC dc, POINT center) {
    FillEllipse(dc, center.x - 54, center.y + 19, center.x + 51, center.y + 55, RGB(111, 132, 101), RGB(91, 113, 83));
    FillRectangle(dc, RECT{center.x - 16, center.y - 34, center.x + 16, center.y + 27}, RGB(239, 230, 197), RGB(140, 122, 92), 2);
    FillRectangle(dc, RECT{center.x - 19, center.y - 39, center.x + 19, center.y - 30}, RGB(174, 94, 75), RGB(136, 71, 61), 2);
    FillRectangle(dc, RECT{center.x - 8, center.y - 27, center.x + 8, center.y - 12}, RGB(251, 211, 116), RGB(181, 143, 72));
    DrawLine(dc, POINT{center.x - 22, center.y - 28}, POINT{center.x - 48, center.y - 42}, RGB(250, 211, 116), 2);
    DrawLine(dc, POINT{center.x + 22, center.y - 28}, POINT{center.x + 48, center.y - 42}, RGB(250, 211, 116), 2);
}

void DrawLandmark(HDC dc, const MapLayout& layout, island::LocationId id) {
    const POINT center = ToScreen(layout, g_world.GetLocation(id).position);
    switch (id) {
    case island::LocationId::Home: DrawHouse(dc, center); break;
    case island::LocationId::Harbor: DrawHarbor(dc, center); break;
    case island::LocationId::PineForest: DrawForest(dc, center); break;
    case island::LocationId::TidalFlat: DrawTidalFlat(dc, center); break;
    case island::LocationId::WindCape: DrawLighthouse(dc, center); break;
    case island::LocationId::Count: break;
    }
}

void DrawIslandBackground(HDC dc, const MapLayout& layout, int clientHeight) {
    const int mapRight = layout.panelLeft - 18;
    FillRectangle(dc, RECT{0, 0, mapRight, clientHeight}, RGB(135, 204, 219), RGB(135, 204, 219));

    for (int row = 0; row < 11; ++row) {
        const int y = 38 + row * 54;
        for (int column = 0; column < 8; ++column) {
            const int x = 20 + column * 82 + ((row % 2) * 29);
            DrawLine(dc, POINT{x, y}, POINT{x + 20, y - 4}, RGB(181, 226, 231), 2);
            DrawLine(dc, POINT{x + 20, y - 4}, POINT{x + 39, y}, RGB(181, 226, 231), 2);
        }
    }

    const auto p = [&layout](int x, int y) { return ToScreen(layout, island::MapPosition{x, y}); };
    std::vector<POINT> shore = {
        p(55, 237), p(92, 171), p(184, 127), p(293, 139), p(384, 118),
        p(489, 139), p(603, 180), p(673, 235), p(690, 301), p(674, 372),
        p(625, 444), p(548, 501), p(447, 538), p(337, 548), p(234, 523),
        p(144, 481), p(82, 416), p(52, 335)
    };
    FillPolygon(dc, shore, RGB(225, 212, 159), RGB(206, 190, 131), 8);

    std::vector<POINT> grass = {
        p(69, 239), p(103, 179), p(190, 140), p(294, 151), p(384, 131),
        p(484, 151), p(590, 189), p(657, 241), p(675, 302), p(660, 366),
        p(615, 432), p(541, 486), p(444, 523), p(339, 533), p(242, 509),
        p(153, 468), p(96, 410), p(67, 335)
    };
    FillPolygon(dc, grass, RGB(139, 185, 111), RGB(97, 148, 91), 2);

    const POINT garden = p(275, 194);
    const POINT grove = p(180, 405);
    const POINT meadow = p(510, 327);
    FillEllipse(dc, garden.x - 38, garden.y - 14, garden.x + 38, garden.y + 14, RGB(123, 175, 105), RGB(123, 175, 105));
    FillEllipse(dc, grove.x - 40, grove.y - 15, grove.x + 40, grove.y + 15, RGB(122, 174, 103), RGB(122, 174, 103));
    FillEllipse(dc, meadow.x - 45, meadow.y - 17, meadow.x + 45, meadow.y + 17, RGB(125, 176, 106), RGB(125, 176, 106));
}

void DrawRoutes(HDC dc, const MapLayout& layout) {
    const std::vector<island::LocationId> ids = {
        island::LocationId::Home,
        island::LocationId::Harbor,
        island::LocationId::PineForest,
        island::LocationId::TidalFlat,
        island::LocationId::WindCape
    };
    const auto positionOf = [&layout](island::LocationId id) {
        return ToScreen(layout, g_world.GetLocation(id).position);
    };

    for (island::LocationId from : ids) {
        for (island::LocationId to : g_world.GetNeighbors(from)) {
            if (static_cast<std::size_t>(from) >= static_cast<std::size_t>(to)) {
                continue;
            }
            const bool lowTideRoute = !g_world.CanTravel(from, to, island::TidePhase::Rising);
            DrawLine(dc, positionOf(from), positionOf(to),
                     lowTideRoute ? RGB(53, 145, 165) : RGB(143, 111, 72),
                     lowTideRoute ? 3 : 7,
                     lowTideRoute ? PS_DASH : PS_SOLID);
        }
    }
}

void DrawPlayer(HDC dc, POINT location) {
    const int x = location.x + 27;
    const int y = location.y + 48;
    FillEllipse(dc, x - 18, y + 18, x + 18, y + 25, RGB(89, 119, 89), RGB(89, 119, 89));
    FillEllipse(dc, x - 12, y - 12, x + 12, y + 12, RGB(248, 221, 177), RGB(129, 97, 69), 2);
    FillRectangle(dc, RECT{x - 11, y + 8, x + 11, y + 29}, RGB(75, 111, 150), RGB(54, 81, 112), 2);
    DrawLine(dc, POINT{x - 5, y + 28}, POINT{x - 8, y + 40}, RGB(72, 63, 55), 4);
    DrawLine(dc, POINT{x + 5, y + 28}, POINT{x + 8, y + 40}, RGB(72, 63, 55), 4);
}

void DrawHomeCharacter(HDC dc, int x, int y) {
    FillEllipse(dc, x - 20, y - 3, x + 20, y + 7, RGB(72, 110, 70), RGB(72, 110, 70));
    FillRectangle(dc, RECT{x - 13, y - 17, x - 3, y - 1}, RGB(78, 61, 52), RGB(58, 46, 42));
    FillRectangle(dc, RECT{x + 3, y - 17, x + 13, y - 1}, RGB(78, 61, 52), RGB(58, 46, 42));
    FillRectangle(dc, RECT{x - 15, y - 42, x + 15, y - 16}, RGB(71, 119, 132), RGB(47, 83, 94), 2);
    FillEllipse(dc, x - 12, y - 61, x + 12, y - 37, RGB(238, 197, 151), RGB(112, 78, 59), 2);
    FillEllipse(dc, x - 14, y - 64, x + 14, y - 48, RGB(88, 57, 43), RGB(69, 44, 35), 2);
    FillRectangle(dc, RECT{x - 11, y - 49, x + 11, y - 44}, RGB(88, 57, 43), RGB(88, 57, 43));
    FillRectangle(dc, RECT{x - 16, y - 39, x + 16, y - 33}, RGB(198, 155, 88), RGB(151, 115, 67));
}

void DrawFlower(HDC dc, int x, int y, COLORREF petals) {
    DrawLine(dc, POINT{x, y + 2}, POINT{x - 2, y + 20}, RGB(57, 108, 57), 3);
    FillEllipse(dc, x - 12, y - 12, x - 2, y - 2, petals, RGB(149, 100, 78));
    FillEllipse(dc, x + 2, y - 12, x + 12, y - 2, petals, RGB(149, 100, 78));
    FillEllipse(dc, x - 8, y - 21, x + 3, y - 10, petals, RGB(149, 100, 78));
    FillEllipse(dc, x - 7, y - 5, x + 4, y + 6, RGB(239, 193, 87), RGB(181, 133, 51));
}

void DrawGardenTree(HDC dc, int x, int y) {
    FillEllipse(dc, x - 66, y - 16, x + 66, y + 29, RGB(64, 112, 67), RGB(64, 112, 67));
    FillRectangle(dc, RECT{x - 12, y - 105, x + 14, y + 3}, RGB(119, 79, 48), RGB(90, 58, 38), 2);
    DrawLine(dc, POINT{x, y - 72}, POINT{x - 34, y - 117}, RGB(105, 69, 43), 9);
    DrawLine(dc, POINT{x + 2, y - 57}, POINT{x + 39, y - 105}, RGB(105, 69, 43), 8);
    FillEllipse(dc, x - 71, y - 183, x + 26, y - 89, RGB(197, 129, 67), RGB(143, 91, 55), 2);
    FillEllipse(dc, x - 20, y - 202, x + 83, y - 103, RGB(218, 157, 74), RGB(161, 105, 52), 2);
    FillEllipse(dc, x - 72, y - 139, x + 65, y - 53, RGB(183, 112, 59), RGB(143, 91, 55), 2);
    for (int index = 0; index < 8; ++index) {
        const int leafX = x - 53 + (index * 31) % 113;
        const int leafY = y - 168 + (index * 23) % 86;
        FillEllipse(dc, leafX - 5, leafY - 5, leafX + 5, leafY + 5,
                    index % 2 == 0 ? RGB(241, 190, 83) : RGB(222, 145, 66),
                    index % 2 == 0 ? RGB(241, 190, 83) : RGB(222, 145, 66));
    }
}

void DrawFlowerBed(HDC dc, int x, int y, bool showFlowers) {
    FillRectangle(dc, RECT{x, y, x + 245, y + 116}, RGB(125, 78, 48), RGB(79, 54, 39), 3);
    FillRectangle(dc, RECT{x + 12, y + 11, x + 233, y + 103}, RGB(90, 62, 43), RGB(90, 62, 43));
    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 5; ++column) {
            const int plantX = x + 38 + column * 42;
            const int plantY = y + 42 + row * 42;
            if (showFlowers) {
                DrawFlower(dc, plantX, plantY, (row + column) % 2 == 0 ? RGB(222, 116, 127) : RGB(238, 212, 131));
            } else {
                DrawLine(dc, POINT{plantX, plantY + 4}, POINT{plantX - 2, plantY + 19}, RGB(100, 119, 65), 3);
                DrawLine(dc, POINT{plantX - 1, plantY + 9}, POINT{plantX - 11, plantY + 3}, RGB(120, 139, 75), 3);
                FillEllipse(dc, plantX - 5, plantY - 1, plantX + 5, plantY + 8, RGB(159, 119, 73), RGB(119, 83, 57));
            }
        }
    }
    DrawLine(dc, POINT{x + 6, y + 7}, POINT{x + 239, y + 7}, RGB(183, 130, 77), 3);
}

void DrawFlowerPlanter(HDC dc, int x, int y) {
    FillRectangle(dc, RECT{x - 40, y - 3, x + 40, y + 24}, RGB(148, 84, 55), RGB(99, 57, 42), 2);
    FillRectangle(dc, RECT{x - 44, y - 9, x + 44, y - 1}, RGB(178, 105, 69), RGB(111, 65, 47), 2);
    for (int index = 0; index < 3; ++index) {
        DrawFlower(dc, x - 24 + index * 24, y - 22 - (index % 2) * 4,
                   index == 1 ? RGB(231, 191, 110) : RGB(191, 110, 150));
    }
}

void DrawFenceHorizontal(HDC dc, int x1, int x2, int y) {
    DrawLine(dc, POINT{x1, y - 14}, POINT{x2, y - 14}, RGB(97, 63, 42), 9);
    DrawLine(dc, POINT{x1, y + 7}, POINT{x2, y + 7}, RGB(126, 83, 52), 9);
    for (int x = x1; x <= x2; x += 46) {
        FillRectangle(dc, RECT{x - 6, y - 32, x + 7, y + 19}, RGB(145, 98, 60), RGB(92, 61, 43), 2);
        FillPolygon(dc, {{x - 8, y - 31}, {x, y - 42}, {x + 9, y - 31}}, RGB(175, 125, 78), RGB(92, 61, 43), 2);
    }
}

void DrawFenceVertical(HDC dc, int x, int y1, int y2, bool leaveRepairGap) {
    const int gapStart = 330;
    const int gapEnd = 409;
    for (int y = y1; y <= y2; y += 46) {
        const bool inGap = leaveRepairGap && !g_homeFenceRepaired && y > gapStart - 40 && y < gapEnd;
        if (inGap) {
            continue;
        }
        DrawLine(dc, POINT{x - 11, y}, POINT{x - 11, y + 46}, RGB(97, 63, 42), 8);
        DrawLine(dc, POINT{x + 8, y}, POINT{x + 8, y + 46}, RGB(126, 83, 52), 8);
        FillRectangle(dc, RECT{x - 27, y + 5, x + 26, y + 16}, RGB(145, 98, 60), RGB(92, 61, 43), 2);
    }
    if (leaveRepairGap && !g_homeFenceRepaired) {
        DrawLine(dc, POINT{x - 7, gapStart + 7}, POINT{x + 25, gapStart + 37}, RGB(126, 83, 52), 9);
        DrawTextBlock(dc, L"수리할 울타리", RECT{x + 24, gapStart + 4, x + 210, gapStart + 40}, RGB(88, 72, 52), 15, true, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

void DrawHomeExterior(HDC dc, RECT client) {
    const int saved = SaveDC(dc);
    SetMapMode(dc, MM_ANISOTROPIC);
    SetWindowExtEx(dc, HomeCanvasWidth, HomeCanvasHeight, nullptr);
    SetViewportExtEx(dc, std::max(1L, client.right), std::max(1L, client.bottom), nullptr);

    FillRectangle(dc, RECT{0, 0, HomeCanvasWidth, HomeCanvasHeight}, RGB(111, 163, 91), RGB(111, 163, 91));
    for (int y = 16; y < 790; y += 38) {
        for (int x = 14; x < 1190; x += 47) {
            const int seed = (x * 7 + y * 11) % 17;
            if (seed < 5) {
                DrawLine(dc, POINT{x, y + 8}, POINT{x - 4, y - 1}, RGB(89, 143, 82), 3);
                DrawLine(dc, POINT{x, y + 8}, POINT{x + 7, y}, RGB(129, 177, 98), 3);
            } else if (seed == 8) {
                FillEllipse(dc, x, y, x + 5, y + 5, RGB(220, 204, 142), RGB(220, 204, 142));
            }
        }
    }

    DrawFenceHorizontal(dc, 58, 1140, 92);
    DrawFenceVertical(dc, 73, 102, 745, true);
    DrawFenceVertical(dc, 1128, 102, 745, false);
    DrawFenceHorizontal(dc, 58, 476, 760);
    DrawFenceHorizontal(dc, 724, 1140, 760);
    FillRectangle(dc, RECT{480, 744, 722, 778}, RGB(102, 152, 88), RGB(102, 152, 88));
    FillRectangle(dc, RECT{476, 735, 491, 780}, RGB(137, 92, 55), RGB(89, 62, 42), 2);
    FillRectangle(dc, RECT{710, 735, 725, 780}, RGB(137, 92, 55), RGB(89, 62, 42), 2);

    FillPolygon(dc, {{514, 448}, {687, 448}, {778, 800}, {422, 800}}, RGB(194, 157, 104), RGB(163, 127, 82), 2);
    for (int y = 470; y < 790; y += 39) {
        const int inset = (y - 448) / 4;
        DrawLine(dc, POINT{570 - inset, y}, POINT{630 + inset, y}, RGB(165, 129, 87), 3);
        for (int x = 545 - inset; x < 650 + inset; x += 42) {
            FillEllipse(dc, x, y + 7, x + 13, y + 13, RGB(211, 181, 132), RGB(211, 181, 132));
        }
    }
    FillPolygon(dc, {{220, 548}, {430, 520}, {497, 560}, {398, 604}, {214, 600}}, RGB(184, 148, 100), RGB(156, 122, 84), 2);

    DrawFlowerBed(dc, 112, 429, false);
    DrawFlowerBed(dc, 112, 575, false);
    DrawFlowerBed(dc, 377, 588, false);
    FillEllipse(dc, 212, 610, 316, 672, RGB(107, 139, 75), RGB(83, 117, 64), 2);
    if (g_flowerPlanterPlaced) {
        DrawFlowerPlanter(dc, 263, 621);
    } else {
        DrawTextBlock(dc, L"빈 화단", RECT{222, 627, 305, 657}, RGB(243, 225, 178), 15, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    DrawGardenTree(dc, 1000, 492);
    FillRectangle(dc, RECT{923, 496, 972, 534}, RGB(143, 99, 62), RGB(89, 63, 43), 2);
    FillRectangle(dc, RECT{929, 455, 966, 497}, RGB(193, 139, 79), RGB(104, 73, 49), 2);
    FillRectangle(dc, RECT{936, 463, 959, 483}, RGB(222, 213, 173), RGB(127, 101, 67), 2);
    FillRectangle(dc, RECT{1032, 602, 1103, 667}, RGB(119, 80, 51), RGB(80, 56, 39), 3);
    FillRectangle(dc, RECT{1025, 591, 1110, 607}, RGB(152, 102, 62), RGB(85, 60, 41), 2);
    DrawTextBlock(dc, L"퇴비통", RECT{1034, 625, 1101, 653}, RGB(244, 219, 174), 14, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    FillEllipse(dc, 330, 401, 870, 474, RGB(76, 120, 72), RGB(76, 120, 72));
    FillRectangle(dc, RECT{354, 288, 846, 443}, RGB(190, 140, 88), RGB(91, 61, 42), 4);
    for (int y = 307; y < 435; y += 27) {
        DrawLine(dc, POINT{358, y}, POINT{842, y}, RGB(153, 105, 66), 3);
        for (int x = 385; x < 835; x += 74) {
            DrawLine(dc, POINT{x, y - 12}, POINT{x, y + 12}, RGB(214, 167, 109), 2);
        }
    }
    FillRectangle(dc, RECT{337, 277, 863, 298}, RGB(101, 67, 47), RGB(78, 53, 40), 3);
    FillRectangle(dc, RECT{337, 437, 863, 453}, RGB(116, 77, 51), RGB(76, 52, 40), 2);
    FillPolygon(dc, {{302, 270}, {416, 117}, {600, 67}, {784, 117}, {900, 270}, {852, 326}, {348, 326}}, RGB(139, 61, 55), RGB(90, 45, 43), 4);
    FillPolygon(dc, {{302, 270}, {416, 117}, {600, 67}, {600, 326}, {348, 326}}, RGB(130, 55, 50), RGB(90, 45, 43), 2);
    FillPolygon(dc, {{600, 67}, {784, 117}, {900, 270}, {852, 326}, {600, 326}}, RGB(167, 70, 58), RGB(90, 45, 43), 2);
    for (int row = 0; row < 5; ++row) {
        const int y = 111 + row * 39;
        const int halfWidth = 43 + row * 49;
        const int left = 600 - halfWidth;
        const int right = 600 + halfWidth;
        for (int x = left + 8 + (row % 2) * 18; x < right - 12; x += 43) {
            const COLORREF tile = ((x / 43 + row) % 2 == 0) ? RGB(184, 80, 62) : RGB(157, 67, 56);
            FillRectangle(dc, RECT{x, y, x + 33, y + 23}, tile, RGB(112, 53, 48), 1);
        }
        DrawLine(dc, POINT{left + 4, y + 26}, POINT{right - 4, y + 26}, RGB(112, 53, 48), 2);
    }
    FillRectangle(dc, RECT{766, 123, 809, 226}, RGB(112, 84, 68), RGB(75, 58, 48), 3);
    for (int y = 133; y < 218; y += 17) {
        DrawLine(dc, POINT{768, y}, POINT{807, y}, RGB(157, 122, 92), 2);
    }
    FillRectangle(dc, RECT{426, 337, 516, 416}, RGB(115, 166, 176), RGB(82, 60, 45), 3);
    FillRectangle(dc, RECT{416, 327, 526, 339}, RGB(72, 105, 69), RGB(57, 76, 51), 2);
    FillRectangle(dc, RECT{426, 327, 438, 424}, RGB(85, 123, 76), RGB(57, 76, 51), 2);
    FillRectangle(dc, RECT{504, 327, 516, 424}, RGB(85, 123, 76), RGB(57, 76, 51), 2);
    DrawLine(dc, POINT{471, 339}, POINT{471, 416}, RGB(220, 211, 174), 4);
    DrawLine(dc, POINT{426, 376}, POINT{516, 376}, RGB(220, 211, 174), 4);
    FillRectangle(dc, RECT{686, 337, 776, 416}, RGB(115, 166, 176), RGB(82, 60, 45), 3);
    FillRectangle(dc, RECT{676, 327, 786, 339}, RGB(72, 105, 69), RGB(57, 76, 51), 2);
    FillRectangle(dc, RECT{686, 327, 698, 424}, RGB(85, 123, 76), RGB(57, 76, 51), 2);
    FillRectangle(dc, RECT{764, 327, 776, 424}, RGB(85, 123, 76), RGB(57, 76, 51), 2);
    DrawLine(dc, POINT{731, 339}, POINT{731, 416}, RGB(220, 211, 174), 4);
    DrawLine(dc, POINT{686, 376}, POINT{776, 376}, RGB(220, 211, 174), 4);
    FillRectangle(dc, RECT{555, 321, 645, 444}, RGB(105, 67, 47), RGB(63, 45, 36), 4);
    FillRectangle(dc, RECT{565, 331, 635, 444}, RGB(142, 91, 57), RGB(88, 57, 42), 2);
    FillRectangle(dc, RECT{577, 343, 623, 380}, RGB(178, 119, 73), RGB(100, 65, 46), 2);
    FillEllipse(dc, 615, 391, 624, 400, RGB(228, 194, 112), RGB(147, 111, 59), 1);
    FillRectangle(dc, RECT{540, 440, 660, 457}, RGB(132, 85, 55), RGB(83, 57, 42), 2);
    FillRectangle(dc, RECT{554, 456, 646, 469}, RGB(157, 105, 66), RGB(90, 61, 43), 2);
    FillRectangle(dc, RECT{555, 292, 645, 315}, RGB(119, 83, 55), RGB(78, 55, 41), 2);
    DrawTextBlock(dc, L"해무섬의 집", RECT{557, 294, 643, 312}, RGB(248, 226, 186), 13, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DrawFlower(dc, 335, 493, RGB(216, 115, 132));
    DrawFlower(dc, 865, 400, RGB(233, 200, 114));
    DrawFlower(dc, 890, 573, RGB(181, 120, 168));
    DrawFlower(dc, 460, 654, RGB(220, 128, 141));
    if (g_homeFenceRepaired) {
        DrawLine(dc, POINT{62, 330}, POINT{62, 406}, RGB(97, 63, 42), 8);
        DrawLine(dc, POINT{80, 330}, POINT{80, 406}, RGB(126, 83, 52), 8);
        FillRectangle(dc, RECT{48, 345, 94, 357}, RGB(145, 98, 60), RGB(92, 61, 43), 2);
        FillRectangle(dc, RECT{48, 380, 94, 392}, RGB(145, 98, 60), RGB(92, 61, 43), 2);
    }

    DrawHomeCharacter(dc, g_homePlayerX, g_homePlayerY);
    FillRectangle(dc, RECT{23, 20, 333, 95}, RGB(249, 242, 213), RGB(136, 108, 75), 3);
    DrawTextBlock(dc, L"부모님의 집과 정원", RECT{42, 31, 318, 61}, RGB(72, 79, 53), 22, true, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"집 외관", RECT{42, 63, 318, 88}, RGB(126, 102, 70), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    FillRectangle(dc, RECT{901, 22, 1176, 252}, RGB(250, 246, 222), RGB(141, 113, 77), 3);
    DrawTextBlock(dc, L"정원 살림", RECT{923, 37, 1155, 66}, RGB(77, 92, 61), 21, true, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"방향키 / WASD  이동", RECT{923, 77, 1156, 101}, RGB(83, 82, 65), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"R  울타리 수리", RECT{923, 107, 1156, 131}, RGB(83, 82, 65), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"F  꽃 심기", RECT{923, 137, 1156, 161}, RGB(83, 82, 65), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"E / Enter  문 열기", RECT{923, 167, 1156, 191}, RGB(83, 82, 65), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"M  섬 지도", RECT{923, 197, 1156, 221}, RGB(83, 82, 65), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    std::wstring prompt;
    if (std::abs(g_homePlayerX - 600) < 95 && std::abs(g_homePlayerY - 470) < 95) {
        prompt = L"E를 눌러 집 안으로 들어가기";
    } else if (!g_homeFenceRepaired && std::abs(g_homePlayerX - 82) < 105 && std::abs(g_homePlayerY - 370) < 115) {
        prompt = L"R을 눌러 울타리 고치기";
    } else if (!g_flowerPlanterPlaced && std::abs(g_homePlayerX - 263) < 115 && std::abs(g_homePlayerY - 635) < 95) {
        prompt = L"F를 눌러 부모님의 꽃 심기";
    } else {
        prompt = g_homeStatus;
    }
    FillRectangle(dc, RECT{354, 729, 846, 785}, RGB(250, 246, 222), RGB(141, 113, 77), 2);
    DrawTextBlock(dc, prompt, RECT{371, 741, 829, 775}, RGB(107, 77, 54), 16, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RestoreDC(dc, saved);
}

void DrawHomeInterior(HDC dc, RECT client) {
    const int saved = SaveDC(dc);
    SetMapMode(dc, MM_ANISOTROPIC);
    SetWindowExtEx(dc, HomeCanvasWidth, HomeCanvasHeight, nullptr);
    SetViewportExtEx(dc, std::max(1L, client.right), std::max(1L, client.bottom), nullptr);

    FillRectangle(dc, RECT{0, 0, HomeCanvasWidth, HomeCanvasHeight}, RGB(57, 75, 66), RGB(57, 75, 66));
    FillRectangle(dc, RECT{69, 58, 1131, 744}, RGB(113, 76, 54), RGB(66, 47, 38), 6);
    FillRectangle(dc, RECT{90, 82, 1110, 231}, RGB(221, 191, 142), RGB(128, 89, 57), 3);
    for (int y = 102; y < 223; y += 28) {
        DrawLine(dc, POINT{94, y}, POINT{1106, y}, RGB(203, 166, 119), 2);
    }
    FillRectangle(dc, RECT{90, 231, 1110, 718}, RGB(157, 111, 72), RGB(96, 66, 49), 3);
    for (int y = 238; y < 712; y += 37) {
        DrawLine(dc, POINT{94, y}, POINT{1106, y}, RGB(132, 91, 61), 3);
        for (int x = 170 + ((y / 37) % 2) * 40; x < 1090; x += 175) {
            DrawLine(dc, POINT{x, y + 2}, POINT{x, y + 33}, RGB(143, 99, 67), 2);
        }
    }

    FillRectangle(dc, RECT{164, 300, 428, 552}, RGB(112, 73, 54), RGB(70, 49, 40), 4);
    FillRectangle(dc, RECT{180, 314, 412, 520}, RGB(220, 198, 162), RGB(150, 111, 77), 3);
    FillRectangle(dc, RECT{181, 314, 411, 360}, RGB(127, 165, 164), RGB(93, 126, 126), 2);
    FillRectangle(dc, RECT{202, 375, 390, 504}, RGB(236, 222, 187), RGB(167, 133, 94), 2);
    FillRectangle(dc, RECT{194, 361, 399, 386}, RGB(192, 104, 91), RGB(128, 75, 67), 2);
    FillEllipse(dc, 325, 328, 383, 371, RGB(231, 210, 174), RGB(172, 135, 95), 2);
    FillRectangle(dc, RECT{146, 548, 446, 576}, RGB(143, 91, 58), RGB(76, 53, 41), 3);

    FillRectangle(dc, RECT{499, 331, 713, 488}, RGB(120, 77, 50), RGB(69, 48, 37), 4);
    FillRectangle(dc, RECT{520, 350, 692, 466}, RGB(211, 170, 120), RGB(127, 89, 58), 3);
    FillRectangle(dc, RECT{545, 364, 667, 452}, RGB(190, 144, 101), RGB(125, 88, 60), 2);
    DrawLine(dc, POINT{554, 378}, POINT{659, 438}, RGB(137, 96, 67), 2);
    DrawLine(dc, POINT{659, 378}, POINT{554, 438}, RGB(137, 96, 67), 2);
    FillEllipse(dc, 564, 339, 649, 370, RGB(238, 225, 193), RGB(162, 126, 88), 2);
    DrawTextBlock(dc, L"물때 기록장", RECT{567, 343, 646, 365}, RGB(95, 66, 49), 13, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    FillRectangle(dc, RECT{486, 492, 515, 548}, RGB(108, 71, 50), RGB(73, 51, 39), 3);
    FillRectangle(dc, RECT{697, 492, 726, 548}, RGB(108, 71, 50), RGB(73, 51, 39), 3);

    FillRectangle(dc, RECT{822, 285, 1044, 474}, RGB(111, 71, 49), RGB(68, 47, 36), 4);
    FillRectangle(dc, RECT{839, 302, 1027, 452}, RGB(178, 128, 81), RGB(108, 74, 50), 3);
    FillRectangle(dc, RECT{858, 321, 915, 445}, RGB(113, 76, 54), RGB(81, 58, 43), 2);
    FillRectangle(dc, RECT{931, 321, 1007, 374}, RGB(227, 210, 166), RGB(129, 96, 67), 2);
    FillRectangle(dc, RECT{931, 391, 1007, 445}, RGB(227, 210, 166), RGB(129, 96, 67), 2);
    FillEllipse(dc, 957, 331, 973, 347, RGB(198, 122, 82), RGB(126, 75, 54));
    FillEllipse(dc, 958, 400, 973, 416, RGB(198, 122, 82), RGB(126, 75, 54));

    FillRectangle(dc, RECT{860, 526, 1027, 656}, RGB(95, 64, 48), RGB(59, 45, 37), 4);
    FillRectangle(dc, RECT{879, 542, 1008, 639}, RGB(177, 102, 73), RGB(111, 66, 49), 2);
    FillRectangle(dc, RECT{898, 561, 988, 610}, RGB(75, 71, 62), RGB(48, 47, 43), 3);
    FillEllipse(dc, 925, 571, 963, 600, RGB(244, 169, 91), RGB(185, 112, 59), 2);
    DrawLine(dc, POINT{918, 614}, POINT{968, 614}, RGB(229, 195, 136), 3);

    FillRectangle(dc, RECT{487, 669, 713, 733}, RGB(83, 58, 43), RGB(52, 41, 34), 3);
    FillRectangle(dc, RECT{526, 663, 674, 729}, RGB(143, 99, 65), RGB(87, 60, 44), 2);
    FillRectangle(dc, RECT{545, 676, 655, 729}, RGB(179, 131, 84), RGB(92, 63, 45), 2);
    FillEllipse(dc, 588, 685, 612, 709, RGB(237, 194, 122), RGB(128, 89, 57), 2);

    DrawHomeCharacter(dc, g_interiorPlayerX, g_interiorPlayerY);
    FillRectangle(dc, RECT{22, 20, 360, 96}, RGB(249, 242, 213), RGB(136, 108, 75), 3);
    DrawTextBlock(dc, L"부모님의 집 · 실내", RECT{41, 33, 340, 63}, RGB(72, 79, 53), 22, true, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"낡았지만 따뜻한 집", RECT{41, 65, 340, 88}, RGB(126, 102, 70), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    FillRectangle(dc, RECT{901, 22, 1176, 210}, RGB(250, 246, 222), RGB(141, 113, 77), 3);
    DrawTextBlock(dc, L"조작", RECT{923, 38, 1154, 67}, RGB(77, 92, 61), 20, true, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"방향키 / WASD  이동", RECT{923, 80, 1156, 104}, RGB(83, 82, 65), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"E / Enter  물때 기록장 보기", RECT{923, 112, 1156, 136}, RGB(83, 82, 65), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawTextBlock(dc, L"Esc  정원으로 나가기", RECT{923, 144, 1156, 168}, RGB(83, 82, 65), 15, false, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    FillRectangle(dc, RECT{357, 737, 843, 785}, RGB(250, 246, 222), RGB(141, 113, 77), 2);
    DrawTextBlock(dc, g_interiorStatus, RECT{373, 746, 827, 776}, RGB(107, 77, 54), 15, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RestoreDC(dc, saved);
}

void DrawLocationLabel(HDC dc, const MapLayout& layout, island::LocationId id) {
    const island::Location& location = g_world.GetLocation(id);
    const POINT center = ToScreen(layout, location.position);
    RECT label{center.x - 71, center.y + 67, center.x + 71, center.y + 92};
    DrawTextBlock(dc, location.name, label, RGB(48, 60, 49), 17, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawNearbyRoutes(HDC dc, int panelLeft, int& y) {
    DrawTextAt(dc, L"주변 장소", panelLeft, y, RGB(57, 85, 67), 19, true);
    y += 30;

    for (island::LocationId neighbor : g_world.GetNeighbors(g_currentLocation)) {
        const bool accessible = g_world.CanTravel(g_currentLocation, neighbor, g_tide);
        const island::Direction direction = g_world.GetTravelDirection(g_currentLocation, neighbor);
        std::wstring line = std::wstring(island::ToString(direction)) + L"  " + g_world.GetLocation(neighbor).name;
        line += accessible ? L"  · 이동 가능" : L"  · 간조에 개방";
        DrawTextAt(dc, line, panelLeft, y, accessible ? RGB(49, 101, 72) : RGB(126, 106, 75), 16, false);
        y += 25;
    }
}

void DrawSidePanel(HDC dc, RECT client, const MapLayout& layout) {
    const RECT panel{layout.panelLeft, 24, client.right - 24, client.bottom - 24};
    FillRectangle(dc, panel, RGB(250, 247, 233), RGB(194, 185, 158), 2);

    int x = panel.left + 24;
    int y = panel.top + 20;
    DrawTextAt(dc, L"파도 끝의 집", x, y, RGB(47, 92, 82), 27, true);
    y += 39;
    DrawTextAt(dc, L"해무섬 · 2D 월드 지도", x, y, RGB(105, 119, 103), 16, false);
    y += 40;

    const island::Location& current = g_world.GetLocation(g_currentLocation);
    DrawTextAt(dc, L"현재 장소", x, y, RGB(126, 111, 75), 15, true);
    y += 23;
    DrawTextAt(dc, current.name, x, y, RGB(48, 73, 59), 22, true);
    y += 33;

    RECT description{x, y, panel.right - 20, y + 70};
    DrawTextBlock(dc, current.description, description, RGB(77, 88, 77), 16, false, DT_LEFT | DT_TOP | DT_WORDBREAK);
    y += 83;

    DrawTextAt(dc, L"현재 물때", x, y, RGB(126, 111, 75), 15, true);
    y += 23;
    const COLORREF tideColor = g_tide == island::TidePhase::Low ? RGB(55, 133, 148) : RGB(91, 119, 95);
    DrawTextAt(dc, std::wstring(L"◈  ") + island::ToString(g_tide), x, y, tideColor, 20, true);
    y += 39;

    DrawNearbyRoutes(dc, x, y);
    y += 8;
    DrawTextAt(dc, L"조작", x, y, RGB(57, 85, 67), 18, true);
    y += 27;
    DrawTextAt(dc, L"방향키 / WASD  장소 이동", x, y, RGB(77, 88, 77), 15, false);
    y += 23;
    DrawTextAt(dc, L"Space  다음 물때", x, y, RGB(77, 88, 77), 15, false);
    y += 23;
    DrawTextAt(dc, L"Enter  장소 살펴보기", x, y, RGB(77, 88, 77), 15, false);
    y += 23;
    DrawTextAt(dc, L"Esc  게임 종료", x, y, RGB(77, 88, 77), 15, false);

    RECT status{x, panel.bottom - 72, panel.right - 20, panel.bottom - 20};
    DrawTextBlock(dc, g_status, status, RGB(154, 93, 71), 15, true, DT_LEFT | DT_TOP | DT_WORDBREAK);
}

void DrawGame(HDC dc, RECT client) {
    if (g_scene == SceneMode::HomeExterior) {
        DrawHomeExterior(dc, client);
        return;
    }
    if (g_scene == SceneMode::HomeInterior) {
        DrawHomeInterior(dc, client);
        return;
    }

    const int panelLeft = client.right - SidePanelWidth - 24;
    const int mapAreaWidth = panelLeft - 40;
    const int mapAreaHeight = client.bottom - 105;
    MapLayout layout{
        20,
        72,
        static_cast<double>(mapAreaWidth) / WorldMapWidth,
        static_cast<double>(mapAreaHeight) / WorldMapHeight,
        panelLeft
    };

    DrawIslandBackground(dc, layout, client.bottom);
    DrawRoutes(dc, layout);

    const std::vector<island::LocationId> ids = {
        island::LocationId::Home,
        island::LocationId::Harbor,
        island::LocationId::PineForest,
        island::LocationId::TidalFlat,
        island::LocationId::WindCape
    };
    for (island::LocationId id : ids) {
        DrawLandmark(dc, layout, id);
    }
    for (island::LocationId id : ids) {
        DrawLocationLabel(dc, layout, id);
    }

    const POINT playerLocation = ToScreen(layout, g_world.GetLocation(g_currentLocation).position);
    DrawRing(dc, playerLocation.x - 58, playerLocation.y - 55,
             playerLocation.x + 58, playerLocation.y + 64,
             RGB(227, 166, 70), 3);
    DrawLandmark(dc, layout, g_currentLocation);
    DrawPlayer(dc, POINT{playerLocation.x + 12, playerLocation.y - 40});

    RECT mapTitle{30, 20, panelLeft - 20, 58};
    DrawTextBlock(dc, L"청람 군도 · 해무섬", mapTitle, RGB(40, 91, 103), 23, true, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawSidePanel(dc, client, layout);
}

void TryTravel(HWND window, island::Direction direction) {
    const std::optional<island::LocationId> destination = g_world.GetNeighborInDirection(g_currentLocation, direction);
    if (!destination.has_value()) {
        g_status = L"그 방향으로 이어지는 길이 없습니다.";
    } else if (!g_world.CanTravel(g_currentLocation, *destination, g_tide)) {
        g_status = L"물이 빠지는 간조가 되면 갯벌 길이 열립니다.";
    } else {
        g_currentLocation = *destination;
        g_status = g_world.GetLocation(g_currentLocation).description;
    }
    InvalidateRect(window, nullptr, TRUE);
}

void MoveHomePlayer(HWND window, int deltaX, int deltaY) {
    const int nextX = std::clamp(g_homePlayerX + deltaX, 110, 1090);
    const int nextY = std::clamp(g_homePlayerY + deltaY, 125, 730);
    const auto overlaps = [nextX, nextY](int left, int top, int right, int bottom) {
        return nextX > left - 14 && nextX < right + 14 &&
               nextY > top - 10 && nextY < bottom + 8;
    };
    const bool insideHouse = overlaps(300, 65, 900, 475);
    const bool insideFlowerBeds = overlaps(112, 429, 357, 545) ||
                                  overlaps(112, 575, 357, 691) ||
                                  overlaps(377, 588, 622, 704);
    const bool nearGardenProps = overlaps(923, 455, 972, 534) ||
                                 overlaps(920, 288, 1085, 540) ||
                                 overlaps(1025, 591, 1110, 667);
    if (insideHouse || insideFlowerBeds || nearGardenProps) {
        InvalidateRect(window, nullptr, TRUE);
        return;
    }
    g_homePlayerX = nextX;
    g_homePlayerY = nextY;
    InvalidateRect(window, nullptr, TRUE);
}

void MoveInteriorPlayer(HWND window, int deltaX, int deltaY) {
    const int nextX = std::clamp(g_interiorPlayerX + deltaX, 112, 1088);
    const int nextY = std::clamp(g_interiorPlayerY + deltaY, 132, 684);
    const bool besideBed = nextX > 135 && nextX < 455 && nextY > 292 && nextY < 585;
    const bool atTable = nextX > 480 && nextX < 730 && nextY > 320 && nextY < 555;
    const bool atCabinet = nextX > 810 && nextX < 1055 && nextY > 275 && nextY < 485;
    const bool atStove = nextX > 850 && nextX < 1040 && nextY > 515 && nextY < 665;
    if (besideBed || atTable || atCabinet || atStove) {
        InvalidateRect(window, nullptr, TRUE);
        return;
    }
    g_interiorPlayerX = nextX;
    g_interiorPlayerY = nextY;
    InvalidateRect(window, nullptr, TRUE);
}

void EnterHomeInterior(HWND window) {
    g_scene = SceneMode::HomeInterior;
    g_interiorPlayerX = 600;
    g_interiorPlayerY = 620;
    g_interiorStatus = L"집 안의 물건을 살펴볼 수 있습니다.";
    InvalidateRect(window, nullptr, TRUE);
}

void ReturnToHomeExterior(HWND window) {
    g_scene = SceneMode::HomeExterior;
    g_homePlayerX = 600;
    g_homePlayerY = 488;
    g_homeStatus = L"정원으로 나왔습니다.";
    InvalidateRect(window, nullptr, TRUE);
}

void ReturnToIslandMap(HWND window) {
    g_scene = SceneMode::IslandMap;
    g_currentLocation = island::LocationId::Home;
    g_status = L"부모님의 집 앞에 있습니다. Enter를 눌러 정원으로 들어가세요.";
    InvalidateRect(window, nullptr, TRUE);
}

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND:
        return 1;

    case WM_KEYDOWN:
        if (g_scene == SceneMode::HomeExterior) {
            switch (wParam) {
            case VK_UP: case 'W': MoveHomePlayer(window, 0, -22); return 0;
            case VK_RIGHT: case 'D': MoveHomePlayer(window, 22, 0); return 0;
            case VK_DOWN: case 'S': MoveHomePlayer(window, 0, 22); return 0;
            case VK_LEFT: case 'A': MoveHomePlayer(window, -22, 0); return 0;
            case 'R':
                if (!g_homeFenceRepaired && std::abs(g_homePlayerX - 82) < 125 && std::abs(g_homePlayerY - 370) < 130) {
                    g_homeFenceRepaired = true;
                    g_homeStatus = L"울타리를 고쳤습니다. 정원이 조금씩 살아납니다.";
                } else {
                    g_homeStatus = L"울타리 수리 지점 가까이에서 R을 눌러주세요.";
                }
                InvalidateRect(window, nullptr, TRUE);
                return 0;
            case 'F':
                if (!g_flowerPlanterPlaced && std::abs(g_homePlayerX - 263) < 130 && std::abs(g_homePlayerY - 635) < 115) {
                    g_flowerPlanterPlaced = true;
                    g_homeStatus = L"부모님이 아끼던 꽃을 화단에 심었습니다.";
                } else {
                    g_homeStatus = L"빈 화단 가까이에서 F를 눌러주세요.";
                }
                InvalidateRect(window, nullptr, TRUE);
                return 0;
            case 'E': case VK_RETURN:
                if (std::abs(g_homePlayerX - 600) < 105 && g_homePlayerY >= 438 && g_homePlayerY < 555) {
                    EnterHomeInterior(window);
                } else {
                    g_homeStatus = L"현관 계단 가까이에서 E 또는 Enter를 눌러주세요.";
                    InvalidateRect(window, nullptr, TRUE);
                }
                return 0;
            case 'M':
                ReturnToIslandMap(window);
                return 0;
            case VK_ESCAPE:
                ReturnToIslandMap(window);
                return 0;
            default:
                return 0;
            }
        }
        if (g_scene == SceneMode::HomeInterior) {
            switch (wParam) {
            case VK_UP: case 'W': MoveInteriorPlayer(window, 0, -20); return 0;
            case VK_RIGHT: case 'D': MoveInteriorPlayer(window, 20, 0); return 0;
            case VK_DOWN: case 'S': MoveInteriorPlayer(window, 0, 20); return 0;
            case VK_LEFT: case 'A': MoveInteriorPlayer(window, -20, 0); return 0;
            case 'E': case VK_RETURN:
                if (std::abs(g_interiorPlayerX - 600) < 150 && std::abs(g_interiorPlayerY - 580) < 100) {
                    g_interiorStatus = L"부모님이 매일 기록한 물때와 낚시터가 적혀 있습니다.";
                } else if (std::abs(g_interiorPlayerX - 600) < 130 && g_interiorPlayerY > 600) {
                    ReturnToHomeExterior(window);
                    return 0;
                } else {
                    g_interiorStatus = L"가까이 다가가면 물건을 살펴볼 수 있습니다.";
                }
                InvalidateRect(window, nullptr, TRUE);
                return 0;
            case VK_ESCAPE:
                ReturnToHomeExterior(window);
                return 0;
            default:
                return 0;
            }
        }
        switch (wParam) {
        case VK_UP:
        case 'W': TryTravel(window, island::Direction::North); return 0;
        case VK_RIGHT:
        case 'D': TryTravel(window, island::Direction::East); return 0;
        case VK_DOWN:
        case 'S': TryTravel(window, island::Direction::South); return 0;
        case VK_LEFT:
        case 'A': TryTravel(window, island::Direction::West); return 0;
        case VK_SPACE:
            g_tide = island::NextTide(g_tide);
            g_status = std::wstring(L"물때가 ") + island::ToString(g_tide) + L"로 바뀌었습니다.";
            InvalidateRect(window, nullptr, TRUE);
            return 0;
        case VK_RETURN:
            if (g_currentLocation == island::LocationId::Home) {
                g_scene = SceneMode::HomeExterior;
                g_homePlayerX = 660;
                g_homePlayerY = 710;
                g_homeStatus = L"부모님의 정원에 도착했습니다. 낡은 집을 천천히 둘러보세요.";
            } else {
                g_status = std::wstring(L"주변을 살펴봅니다. ") + g_world.GetLocation(g_currentLocation).description;
            }
            InvalidateRect(window, nullptr, TRUE);
            return 0;
        case VK_ESCAPE:
            DestroyWindow(window);
            return 0;
        default:
            break;
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        DrawGame(dc, client);
        EndPaint(window, &paint);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    windowClass.lpszClassName = WindowClassName;

    if (!RegisterClassExW(&windowClass)) {
        return 1;
    }

    HWND window = CreateWindowExW(
        0,
        WindowClassName,
        L"파도 끝의 집 | 해무섬",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1220,
        820,
        nullptr,
        nullptr,
        instance,
        nullptr);
    if (window == nullptr) {
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
