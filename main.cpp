#include <windows.h>
#include <string>
#include <cstdint>
#include <iostream>
#include <random>
#include <numbers>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define PI (3.14159265358979323846)
#define TWOPI (3.14159265358979323846 * 2)

#define WINDOW_WIDTH (512)
#define WINDOW_HEIGHT (256)
#define SLEEP_TIME (1)
#define BACKGROUND_COLOR (RGB(0, 0, 0))
#define CELL_COLOR (RGB(255, 255, 255))
#define GRID_WIDTH (256)
#define GRID_HEIGHT (128)
#define CELL_WIDTH (WINDOW_WIDTH / GRID_WIDTH)
#define CELL_HEIGHT (WINDOW_HEIGHT / GRID_HEIGHT)
#define AGENT_DISPLAY_SIZE_CELL_RATIO (0.25)
#define GENERATED_CELLS_WIDTH (GRID_WIDTH)
#define GENERATED_CELLS_HEIGHT (GRID_HEIGHT)
#define AGENT_GENERATION_BOUND_WIDTH (GRID_WIDTH)
#define AGENT_GENERATION_BOUND_HEIGHT (GRID_HEIGHT)
#define SIMULATION_STEP (1)
#define TRAIL_MAX_VALUE (1024)
#define AGENT_COUNT (2048)
#define TRAIL_DECAY (8)
#define TRAIL_DIFFUSION (0.01)
#define AGENT_TRAIL_STRENGTH_NORMAL (128)
#define AGENT_TRAIL_STRENGTH_HIGH (1024)
// #define AGENT_TRAIL_HIGH_STRENGTH_THRESHOLD (128)
#define AGENT_SPEED (0.5)
#define AGENT_TURN_ANGLE (0.05)
#define AGENT_RANDOM_TURN_ANGLE (0.001)
#define AGENT_TRAIL_SEARCH_SIZE (100)
#define AGENT_TRAIL_SEARCH_ANGLE (AGENT_TURN_ANGLE)
#define AGENT_TRAIL_SEARCH_DISTANCE (2.0)
// #define AGENT_WALL_SAFETY_DISTANCE (5.0)
// #define HIGH_VALUE_NEIGHBORS

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
    uint32_t grid[GRID_WIDTH * GRID_HEIGHT];
    int32_t gridDiff[GRID_WIDTH * GRID_HEIGHT];
    Agent agents[AGENT_COUNT];
};

// static double getAngle(double x0, double y0, double x1, double y1)
// {
//     double angle = atan2(y1 - y0, x1 - x0) / TWOPI;
//     return angle > 0 ? angle : angle + 1;
// }

static void advanceAgent(Grid *grid, Agent &agent)
{
    uint32_t leftX = static_cast<uint32_t>(round(cos(TWOPI * (agent.direction - AGENT_TRAIL_SEARCH_ANGLE)) * AGENT_TRAIL_SEARCH_DISTANCE + agent.x) + GRID_WIDTH) % GRID_WIDTH;
    uint32_t leftY = static_cast<uint32_t>(round(sin(TWOPI * (agent.direction - AGENT_TRAIL_SEARCH_ANGLE)) * AGENT_TRAIL_SEARCH_DISTANCE + agent.y) + GRID_HEIGHT) % GRID_HEIGHT;
    uint32_t straightX = static_cast<uint32_t>(round(cos(TWOPI * agent.direction) * AGENT_TRAIL_SEARCH_DISTANCE + agent.x) + GRID_WIDTH) % GRID_WIDTH;
    uint32_t straightY = static_cast<uint32_t>(round(sin(TWOPI * agent.direction) * AGENT_TRAIL_SEARCH_DISTANCE + agent.y) + GRID_HEIGHT) % GRID_HEIGHT;
    uint32_t rightX = static_cast<uint32_t>(round(cos(TWOPI * (agent.direction + AGENT_TRAIL_SEARCH_ANGLE)) * AGENT_TRAIL_SEARCH_DISTANCE + agent.x) + GRID_WIDTH) % GRID_WIDTH;
    uint32_t rightY = static_cast<uint32_t>(round(sin(TWOPI * (agent.direction + AGENT_TRAIL_SEARCH_ANGLE)) * AGENT_TRAIL_SEARCH_DISTANCE + agent.y) + GRID_HEIGHT) % GRID_HEIGHT;

    uint32_t targetStrength = 0;

    targetStrength = max(grid->grid[(leftY * GRID_WIDTH) + leftX], max(grid->grid[(straightY * GRID_WIDTH) + straightX], grid->grid[(rightY * GRID_WIDTH) + rightX]));

    if (targetStrength == grid->grid[(leftY * GRID_WIDTH) + leftX])
    {
        agent.direction -= AGENT_TURN_ANGLE;
    }
    else if (targetStrength == grid->grid[(rightY * GRID_WIDTH) + rightX])
    {
        agent.direction += AGENT_TURN_ANGLE;
    }

    // Add random turn
    agent.direction += (double_distr(gen) * 2 - 1) * AGENT_RANDOM_TURN_ANGLE;

    // Clamp between 0 and 1
    agent.direction = fmod(agent.direction, 1.0);
    if (agent.direction < 0)
    {
        agent.direction += 1.0;
    }

    // Move agent
    agent.x += cos(TWOPI * agent.direction) * AGENT_SPEED;
    agent.y += sin(TWOPI * agent.direction) * AGENT_SPEED;

    if (agent.x < 0)
    {
        agent.x += GRID_WIDTH;
    }
    else if (agent.x >= GRID_WIDTH)
    {
        agent.x -= GRID_WIDTH;
    }
    if (agent.y < 0)
    {
        agent.y += GRID_HEIGHT;
    }
    else if (agent.y >= GRID_HEIGHT)
    {
        agent.y -= GRID_HEIGHT;
    }

    // Add agent trail
#ifdef AGENT_TRAIL_HIGH_STRENGTH_THRESHOLD
    if (targetStrength < AGENT_TRAIL_HIGH_STRENGTH_THRESHOLD)
    {
        grid->gridDiff[(static_cast<uint32_t>(agent.y) * GRID_WIDTH) + static_cast<uint32_t>(agent.x)] += AGENT_TRAIL_STRENGTH_HIGH;
    }
    else
    {
        grid->gridDiff[(static_cast<uint32_t>(agent.y) * GRID_WIDTH) + static_cast<uint32_t>(agent.x)] += AGENT_TRAIL_STRENGTH_NORMAL;
    }
#else
    grid->gridDiff[(static_cast<uint32_t>(agent.y) * GRID_WIDTH) + static_cast<uint32_t>(agent.x)] += AGENT_TRAIL_STRENGTH_NORMAL;
#endif
}

static void advanceGrid(Grid *grid)
{
    memset(grid->gridDiff, 0, GRID_WIDTH * GRID_HEIGHT * sizeof(uint32_t));
    for (uint8_t n = 0; n < SIMULATION_STEP; n++)
    {
        // Advance agents
        for (size_t i = 0; i < AGENT_COUNT; i++)
        {
            advanceAgent(grid, grid->agents[i]);
        }

        // Compute trail diffusion & evaporation
        for (int32_t y = 0; y < GRID_HEIGHT; y++)
        {
            for (int32_t x = 0; x < GRID_WIDTH; x++)
            {
                uint32_t &cellValue = grid->grid[(y * GRID_WIDTH) + x];
                int32_t &cellDiffValue = grid->gridDiff[(y * GRID_WIDTH) + x];

#ifdef TRAIL_DIFFUSION
                // Trail diffusion
                if (cellValue + cellDiffValue > 0)
                {
                    uint32_t diff = static_cast<uint32_t>((static_cast<double>(cellValue) * TRAIL_DIFFUSION) / 4);
                    uint32_t diffused = 0;
                    if (x > 0)
                    {
                        grid->gridDiff[(y * GRID_WIDTH) + (x - 1)] += diff;
                        diffused += diff;
                    }
                    if (x < GRID_WIDTH - 1)
                    {
                        grid->gridDiff[(y * GRID_WIDTH) + (x + 1)] += diff;
                        diffused += diff;
                    }
                    if (y > 0)
                    {
                        grid->gridDiff[((y - 1) * GRID_WIDTH) + x] += diff;
                        diffused += diff;
                    }
                    if (y < GRID_HEIGHT - 1)
                    {
                        grid->gridDiff[((y + 1) * GRID_WIDTH) + x] += diff;
                        diffused += diff;
                    }
                    cellDiffValue -= diffused;
                }
#endif

                // Trail evaporation
                if (cellValue + cellDiffValue > TRAIL_DECAY)
                {
                    cellDiffValue -= TRAIL_DECAY;
                }
                else
                {
                    cellDiffValue = -cellValue;
                }
            }
        }

        // Apply gridDiff and clamp trails to TRAIL_MAX_VALUE
        for (uint32_t i = 0; i < GRID_HEIGHT * GRID_WIDTH; i++)
        {
            grid->grid[i] = min(grid->grid[i] + grid->gridDiff[i], TRAIL_MAX_VALUE);
        }
    }
}

[[maybe_unused]] static void GenerateRandomGrid(Grid &grid)
{
    for (int y = 0; y < GENERATED_CELLS_HEIGHT; y++)
    {
        for (int x = 0; x < GENERATED_CELLS_WIDTH; x++)
        {
            grid.grid[x + (y * GRID_WIDTH)] = static_cast<uint32_t>(((static_cast<double>(int_distr(gen))) / 255.0 * TRAIL_MAX_VALUE));
        }
    }
}

[[maybe_unused]] static void GenerateRandomAgents(Grid &grid)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> int_distr(0, 255);
    for (int i = 0; i < AGENT_COUNT; i++)
    {
        grid.agents[i] = Agent{
            double_distr(gen) * (AGENT_GENERATION_BOUND_WIDTH - 1),
            double_distr(gen) * (AGENT_GENERATION_BOUND_HEIGHT - 1),
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
    RECT rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};

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

        Sleep(SLEEP_TIME);
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
    bmi.bmiHeader.biWidth = WINDOW_WIDTH;
    bmi.bmiHeader.biHeight = -WINDOW_HEIGHT; // Negative = first row is the top of the image.
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
    for (uint32_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < GRID_WIDTH; x++)
        {
            // Convert the trail value into an 8-bit blue intensity.
            uint8_t blue = grid->grid[x + y * GRID_WIDTH] * 255 / TRAIL_MAX_VALUE;

            // Pixels are stored as 0x00BBGGRR.
            // Since we only want blue, the value is simply 0x000000BB.
            uint32_t color = blue;

            // Fill every scanline of this cell.
            for (uint32_t iy = 0; iy < CELL_HEIGHT; iy++)
            {
                // Pointer to the first pixel of this row of the current cell.
                uint32_t *row =
                    pixels +
                    ((((GRID_HEIGHT - 1) - y) * CELL_HEIGHT + iy)) * WINDOW_WIDTH +
                    x * CELL_WIDTH;

                // Fill CELL_WIDTH consecutive pixels with the same color.
                std::fill_n(row, CELL_WIDTH, color);
            }
        }
    }

    // Draw agents directly into the bitmap.
    // This avoids additional GDI calls (SetDCBrushColor + FillRect).
    for (size_t i = 0; i < AGENT_COUNT; i++)
    {
        // Convert agent position from grid coordinates to pixel coordinates.
        uint32_t px = grid->agents[i].x * CELL_WIDTH;
        uint32_t py = (GRID_HEIGHT - 1 - static_cast<uint32_t>(grid->agents[i].y)) * CELL_HEIGHT;

        // Calculate the size of the agent square.
        uint32_t agentSize = CELL_WIDTH * AGENT_DISPLAY_SIZE_CELL_RATIO;
        uint32_t offset = (CELL_WIDTH - agentSize) / 2;

        // Agent color (green).
        // Pixel format is 0x00BBGGRR.
        uint32_t color = 0x0000FF00;

        // Draw the agent square pixel row by pixel row.
        for (uint32_t iy = 0; iy < agentSize; iy++)
        {
            uint32_t *row =
                pixels +
                (py + offset + iy) * WINDOW_WIDTH +
                (px + offset);

            std::fill_n(row, agentSize, color);
        }
    }

    // Copy the completed off-screen bitmap to the window in one operation.
    BitBlt(
        hdc,
        0, 0,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        memDC,
        0, 0,
        SRCCOPY);

    DeleteObject(bitmap);
    DeleteDC(memDC);
}