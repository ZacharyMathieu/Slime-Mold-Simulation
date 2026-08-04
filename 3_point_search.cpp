#include <windows.h>
#include <string>
#include <cstdint>
#include <iostream>
#include <random>
#include <numbers>

namespace Constants
{
    namespace Math
    {
        template <typename T>
        constexpr const T &Min(const T &a, const T &b)
        {
            return (a < b) ? a : b;
        }
        template <typename T>
        constexpr const T &Max(const T &a, const T &b)
        {
            return (a > b) ? a : b;
        }
        inline constexpr double Pi(3.14159265358979323846);
        inline constexpr double TwoPi(3.14159265358979323846 * 2);
    }

    namespace Grid
    {
        inline constexpr uint32_t GRID_WIDTH = 256;
        inline constexpr uint32_t GRID_HEIGHT = 128;

        namespace Generation
        {
            inline constexpr uint32_t GENERATED_CELLS_WIDTH = GRID_WIDTH;
            inline constexpr uint32_t GENERATED_CELLS_HEIGHT = GRID_HEIGHT;
        }
    }

    namespace Display
    {
        inline constexpr uint32_t WindowWidth = 512;
        inline constexpr uint32_t WindowHeight = 256;
        inline constexpr COLORREF BackgroundColor = RGB(0, 0, 0);
        inline constexpr COLORREF CellColor = RGB(255, 255, 255);
        inline constexpr uint32_t CellWidth = WindowWidth / Constants::Grid::GRID_WIDTH;
        inline constexpr uint32_t CellHeight = WindowHeight / Constants::Grid::GRID_HEIGHT;
        inline constexpr double AgentDisplaySizeCellRatio = 0.25;
    }

    namespace Simulation
    {
        inline constexpr uint32_t Step = 1;
        inline constexpr uint32_t SleepTime = 1;
    }

    namespace Trail
    {
        inline constexpr uint32_t MaxValue = 1024;
        inline constexpr uint32_t Decay = 8;
#define TRAIL_DIFFUSION
        inline constexpr double DiffusionRatio = 0.01;
    }

    namespace Agent
    {
        inline constexpr double Speed = 0.5;
        inline constexpr double TurnAngle = 0.05;
        inline constexpr double RandomTurnAngle = 0.001;
#define AGENT_WALL_SAFETY_DISTANCE
        inline constexpr double WallSafetyDistance = 5.0;

        namespace Generation
        {
            inline constexpr uint32_t AGENT_COUNT = 2048;
            inline constexpr uint32_t SPECIES_COUNT = 2;
            inline constexpr uint32_t AGENT_GENERATION_BOUND_WIDTH = Grid::GRID_WIDTH;
            inline constexpr uint32_t AGENT_GENERATION_BOUND_HEIGHT = Grid::GRID_HEIGHT;
        }

        namespace Trail
        {
            inline constexpr uint32_t AGENT_TRAIL_STRENGTH_NORMAL = 128;
            inline constexpr uint32_t AGENT_TRAIL_STRENGTH_HIGH = 1024;
            // #define AGENT_TRAIL_HIGH_STRENGTH_THRESHOLD (128)
        }

        namespace TrailSearch
        {
            inline constexpr double AGENT_TRAIL_SEARCH_SIZE = 100;
            inline constexpr double AGENT_TRAIL_SEARCH_ANGLE = TurnAngle;
            inline constexpr double AGENT_TRAIL_SEARCH_DISTANCE = 2.0;
        }
    }
}

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<uint8_t> int_distr(0, 255);
static std::uniform_real_distribution<double> double_distr(0.0, 1.0);

struct Agent
{
    double x;
    double y;
    double direction;
};

struct Grid
{
    uint32_t grid[Constants::Grid::GRID_WIDTH * Constants::Grid::GRID_HEIGHT * Constants::Agent::Generation::SPECIES_COUNT];
    int32_t gridDiff[Constants::Grid::GRID_WIDTH * Constants::Grid::GRID_HEIGHT];
    Agent agents[Constants::Agent::Generation::AGENT_COUNT];
};

// static double getAngle(double x0, double y0, double x1, double y1)
// {
//     double angle = atan2(y1 - y0, x1 - x0) / TWOPI;
//     return angle > 0 ? angle : angle + 1;
// }

static void advanceAgent(Grid *grid, Agent &agent)
{
    uint32_t leftX = static_cast<uint32_t>(round(cos(Constants::Math::TwoPi * (agent.direction - Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_ANGLE)) * Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_DISTANCE + agent.x) + Constants::Grid::GRID_WIDTH) % Constants::Grid::GRID_WIDTH;
    uint32_t leftY = static_cast<uint32_t>(round(sin(Constants::Math::TwoPi * (agent.direction - Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_ANGLE)) * Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_DISTANCE + agent.y) + Constants::Grid::GRID_HEIGHT) % Constants::Grid::GRID_HEIGHT;
    uint32_t straightX = static_cast<uint32_t>(round(cos(Constants::Math::TwoPi * agent.direction) * Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_DISTANCE + agent.x) + Constants::Grid::GRID_WIDTH) % Constants::Grid::GRID_WIDTH;
    uint32_t straightY = static_cast<uint32_t>(round(sin(Constants::Math::TwoPi * agent.direction) * Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_DISTANCE + agent.y) + Constants::Grid::GRID_HEIGHT) % Constants::Grid::GRID_HEIGHT;
    uint32_t rightX = static_cast<uint32_t>(round(cos(Constants::Math::TwoPi * (agent.direction + Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_ANGLE)) * Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_DISTANCE + agent.x) + Constants::Grid::GRID_WIDTH) % Constants::Grid::GRID_WIDTH;
    uint32_t rightY = static_cast<uint32_t>(round(sin(Constants::Math::TwoPi * (agent.direction + Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_ANGLE)) * Constants::Agent::TrailSearch::AGENT_TRAIL_SEARCH_DISTANCE + agent.y) + Constants::Grid::GRID_HEIGHT) % Constants::Grid::GRID_HEIGHT;

    uint32_t targetStrength = 0;

    targetStrength = Constants::Math::Max(grid->grid[(leftY * Constants::Grid::GRID_WIDTH) + leftX], Constants::Math::Max(grid->grid[(straightY * Constants::Grid::GRID_WIDTH) + straightX], grid->grid[(rightY * Constants::Grid::GRID_WIDTH) + rightX]));

    if (targetStrength == grid->grid[(leftY * Constants::Grid::GRID_WIDTH) + leftX])
    {
        agent.direction -= Constants::Agent::TurnAngle;
    }
    else if (targetStrength == grid->grid[(rightY * Constants::Grid::GRID_WIDTH) + rightX])
    {
        agent.direction += Constants::Agent::TurnAngle;
    }

    // Add random turn
    agent.direction += (double_distr(gen) * 2 - 1) * Constants::Agent::TurnAngle;

    // Clamp between 0 and 1
    agent.direction = fmod(agent.direction, 1.0);
    if (agent.direction < 0)
    {
        agent.direction += 1.0;
    }

    // Move agent
    agent.x += cos(Constants::Math::TwoPi * agent.direction) * Constants::Agent::Speed;
    agent.y += sin(Constants::Math::TwoPi * agent.direction) * Constants::Agent::Speed;

    if (agent.x < 0)
    {
        agent.x += Constants::Grid::GRID_WIDTH;
    }
    else if (agent.x >= Constants::Grid::GRID_WIDTH)
    {
        agent.x -= Constants::Grid::GRID_WIDTH;
    }
    if (agent.y < 0)
    {
        agent.y += Constants::Grid::GRID_HEIGHT;
    }
    else if (agent.y >= Constants::Grid::GRID_HEIGHT)
    {
        agent.y -= Constants::Grid::GRID_HEIGHT;
    }

    // Add agent trail
#ifdef AGENT_TRAIL_HIGH_STRENGTH_THRESHOLD
    if (targetStrength < AGENT_TRAIL_HIGH_STRENGTH_THRESHOLD)
    {
        grid->gridDiff[(static_cast<uint32_t>(agent.y) * Constants::Grid::GRID_WIDTH) + static_cast<uint32_t>(agent.x)] += AGENT_TRAIL_STRENGTH_HIGH;
    }
    else
    {
        grid->gridDiff[(static_cast<uint32_t>(agent.y) * Constants::Grid::GRID_WIDTH) + static_cast<uint32_t>(agent.x)] += AGENT_TRAIL_STRENGTH_NORMAL;
    }
#else
    grid->gridDiff[(static_cast<uint32_t>(agent.y) * Constants::Grid::GRID_WIDTH) + static_cast<uint32_t>(agent.x)] += Constants::Agent::Trail::AGENT_TRAIL_STRENGTH_NORMAL;
#endif
}

static void advanceGrid(Grid *grid)
{
    memset(grid->gridDiff, 0, Constants::Grid::GRID_WIDTH * Constants::Grid::GRID_HEIGHT * sizeof(uint32_t));
    for (uint8_t n = 0; n < Constants::Simulation::Step; n++)
    {
        // Advance agents
        for (size_t i = 0; i < Constants::Agent::Generation::AGENT_COUNT; i++)
        {
            advanceAgent(grid, grid->agents[i]);
        }

        // Compute trail diffusion & evaporation
        for (uint32_t y = 0; y < Constants::Grid::GRID_HEIGHT; y++)
        {
            for (uint32_t x = 0; x < Constants::Grid::GRID_WIDTH; x++)
            {
                uint32_t &cellValue = grid->grid[(y * Constants::Grid::GRID_WIDTH) + x];
                int32_t &cellDiffValue = grid->gridDiff[(y * Constants::Grid::GRID_WIDTH) + x];

#ifdef TRAIL_DIFFUSION
                // Trail diffusion
                if (cellValue + cellDiffValue > 0)
                {
                    uint32_t diff = static_cast<uint32_t>((static_cast<double>(cellValue) * Constants::Trail::DiffusionRatio) / 4);
                    uint32_t diffused = 0;
                    if (x > 0)
                    {
                        grid->gridDiff[(y * Constants::Grid::GRID_WIDTH) + (x - 1)] += diff;
                        diffused += diff;
                    }
                    if (x < Constants::Grid::GRID_WIDTH - 1)
                    {
                        grid->gridDiff[(y * Constants::Grid::GRID_WIDTH) + (x + 1)] += diff;
                        diffused += diff;
                    }
                    if (y > 0)
                    {
                        grid->gridDiff[((y - 1) * Constants::Grid::GRID_WIDTH) + x] += diff;
                        diffused += diff;
                    }
                    if (y < Constants::Grid::GRID_HEIGHT - 1)
                    {
                        grid->gridDiff[((y + 1) * Constants::Grid::GRID_WIDTH) + x] += diff;
                        diffused += diff;
                    }
                    cellDiffValue -= diffused;
                }
#endif

                // Trail evaporation
                if (cellValue + cellDiffValue > Constants::Trail::Decay)
                {
                    cellDiffValue -= Constants::Trail::Decay;
                }
                else
                {
                    cellDiffValue = -cellValue;
                }
            }
        }

        // Apply gridDiff and clamp trails to MaxValue
        for (uint32_t i = 0; i < Constants::Grid::GRID_HEIGHT * Constants::Grid::GRID_WIDTH; i++)
        {
            grid->grid[i] = Constants::Math::Min(grid->grid[i] + grid->gridDiff[i], Constants::Trail::MaxValue);
        }
    }
}

[[maybe_unused]] static void GenerateRandomGrid(Grid &grid)
{
    for (uint32_t y = 0; y < Constants::Grid::Generation::GENERATED_CELLS_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < Constants::Grid::Generation::GENERATED_CELLS_WIDTH; x++)
        {
            grid.grid[x + (y * Constants::Grid::GRID_WIDTH)] = static_cast<uint32_t>(((static_cast<double>(int_distr(gen))) / 255.0 * Constants::Trail::MaxValue));
        }
    }
}

[[maybe_unused]] static void GenerateRandomAgents(Grid &grid)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> int_distr(0, 255);
    for (uint32_t i = 0; i < Constants::Agent::Generation::AGENT_COUNT; i++)
    {
        grid.agents[i] = Agent{
            double_distr(gen) * (Constants::Agent::Generation::AGENT_GENERATION_BOUND_WIDTH - 1),
            double_distr(gen) * (Constants::Agent::Generation::AGENT_GENERATION_BOUND_HEIGHT - 1),
            double_distr(gen),
        };
    }
}

static void drawScene(HDC hdc, Grid *grid);
static void render(HWND hwnd, Grid *grid);
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    constexpr wchar_t kClassName[] = L"FreeDrawWindowClass";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // we paint the whole client area ourselves

    if (!RegisterClassW(&wc))
        return 0;

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exStyle = 0;
    RECT rect = {0, 0, Constants::Display::WindowWidth, Constants::Display::WindowHeight};

    AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    HWND hwnd = CreateWindowExW(
        exStyle,
        kClassName,
        L"Drawing Window",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    bool running = true;

    Grid grid{{}, {}, {}};
    // GenerateRandomGrid(grid);
    GenerateRandomAgents(grid);

    while (running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (!running)
            break;

        render(hwnd, &grid);
        advanceGrid(&grid);

        Sleep(Constants::Simulation::SleepTime);
    }

    return 0;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            DestroyWindow(hwnd);
        return 0;

    case WM_ERASEBKGND:
        return 1; // avoid flicker

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void render(HWND hwnd, Grid *grid)
{
    RECT client{};
    GetClientRect(hwnd, &client);

    const int width = client.right - client.left;
    const int height = client.bottom - client.top;

    if (width <= 0 || height <= 0)
        return;

    HDC windowDC = GetDC(hwnd);
    if (!windowDC)
        return;

    HDC memDC = CreateCompatibleDC(windowDC);
    if (!memDC)
    {
        ReleaseDC(hwnd, windowDC);
        return;
    }

    HBITMAP backBuffer = CreateCompatibleBitmap(windowDC, width, height);
    if (!backBuffer)
    {
        DeleteDC(memDC);
        ReleaseDC(hwnd, windowDC);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(memDC, backBuffer);

    drawScene(memDC, grid);

    BitBlt(windowDC, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(backBuffer);
    DeleteDC(memDC);
    ReleaseDC(hwnd, windowDC);
}

[[maybe_unused]] static void drawText(HDC hdc, std::wstring text, uint32_t &textHeight)
{
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, 0, textHeight, text.c_str(), text.length());

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    textHeight += tm.tmHeight;
}

static void drawScene(HDC hdc, Grid *grid)
{
    // SelectObject(hdc, backgroundBrush);
    // FillRect(hdc, &client, backgroundBrush);

    // Draw grid
    // Create an off-screen drawing context.
    // Everything will be drawn into this instead of directly to the window.
    HDC memDC = CreateCompatibleDC(hdc);

    // Describe the bitmap we want to create.
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = Constants::Display::WindowWidth;
    bmi.bmiHeader.biHeight = -Constants::Display::WindowHeight; // Negative = first row is the top of the image.
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;        // One 32-bit integer per pixel (BGRA).
    bmi.bmiHeader.biCompression = BI_RGB; // Uncompressed bitmap.

    // Pointer to the bitmap's pixel data.
    // CreateDIBSection allocates the bitmap and gives us direct access to its pixels.
    uint32_t *pixels;

    HBITMAP bitmap = CreateDIBSection(
        hdc,
        &bmi,
        DIB_RGB_COLORS,
        (void **)&pixels,
        nullptr,
        0);

    // Attach the bitmap to the off-screen drawing context.
    // Any drawing done with memDC now modifies this bitmap.
    SelectObject(memDC, bitmap);

    // Fill the bitmap one cell at a time.
    for (uint32_t y = 0; y < Constants::Grid::GRID_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < Constants::Grid::GRID_WIDTH; x++)
        {
            // Convert the trail value into an 8-bit blue intensity.
            uint8_t blue = grid->grid[x + y * Constants::Grid::GRID_WIDTH] * 255 / Constants::Trail::MaxValue;

            // Pixels are stored as 0x00BBGGRR.
            // Since we only want blue, the value is simply 0x000000BB.
            uint32_t color = blue;

            // Fill every scanline of this cell.
            for (uint32_t iy = 0; iy < Constants::Display::CellHeight; iy++)
            {
                // Pointer to the first pixel of this row of the current cell.
                uint32_t *row =
                    pixels +
                    ((((Constants::Grid::GRID_HEIGHT - 1) - y) * Constants::Display::CellHeight + iy)) * Constants::Display::WindowWidth +
                    x * Constants::Display::CellWidth;

                // Fill CellWidth consecutive pixels with the same color.
                std::fill_n(row, Constants::Display::CellWidth, color);
            }
        }
    }

    // Draw agents directly into the bitmap.
    // This avoids additional GDI calls (SetDCBrushColor + FillRect).
    for (size_t i = 0; i < Constants::Agent::Generation::AGENT_COUNT; i++)
    {
        // Convert agent position from grid coordinates to pixel coordinates.
        uint32_t px = grid->agents[i].x * Constants::Display::CellWidth;
        uint32_t py = (Constants::Grid::GRID_HEIGHT - 1 - static_cast<uint32_t>(grid->agents[i].y)) * Constants::Display::CellHeight;

        // Calculate the size of the agent square.
        uint32_t agentSize = Constants::Display::CellWidth * Constants::Display::AgentDisplaySizeCellRatio;
        uint32_t offset = (Constants::Display::CellWidth - agentSize) / 2;

        // Agent color (green).
        // Pixel format is 0x00BBGGRR.
        uint32_t color = 0x0000FF00;

        // Draw the agent square pixel row by pixel row.
        for (uint32_t iy = 0; iy < agentSize; iy++)
        {
            uint32_t *row =
                pixels +
                (py + offset + iy) * Constants::Display::WindowWidth + (px + offset);

            std::fill_n(row, agentSize, color);
        }
    }

    // Copy the completed off-screen bitmap to the window in one operation.
    BitBlt(
        hdc,
        0, 0,
        Constants::Display::WindowWidth, Constants::Display::WindowHeight,
        memDC,
        0, 0,
        SRCCOPY);

    DeleteObject(bitmap);
    DeleteDC(memDC);
}