#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <string>
#include <vector>

#include "World.h"

namespace {

constexpr wchar_t WindowClassName[] = L"GamesogongIslandWindow";
constexpr int WorldMapWidth = 720;
constexpr int WorldMapHeight = 580;
constexpr int SidePanelWidth = 330;

island::World g_world = island::World::CreateHaemuIsland();
island::LocationId g_currentLocation = island::LocationId::Home;
island::TidePhase g_tide = island::TidePhase::Low;
std::wstring g_status = L"해무섬에 도착했습니다. 길을 따라 섬을 둘러보세요.";

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

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND:
        return 1;

    case WM_KEYDOWN:
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
            g_status = std::wstring(L"주변을 살펴봅니다. ") + g_world.GetLocation(g_currentLocation).description;
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
