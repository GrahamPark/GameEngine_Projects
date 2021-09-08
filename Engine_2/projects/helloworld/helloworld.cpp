#include <stdint.h>
#include <string>
#include <Windows.h>

struct OffscreenBuffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct WindowDemension
{
    int Width;
    int Height;
};

//TODO: This is a global for now
static bool g_bRunning = false;
static OffscreenBuffer g_GlobalBackBuffer;

void RenderWeirdGradient(const OffscreenBuffer& Buffer, int xOffset, int yOffset)
{
    uint8_t* row = (uint8_t*)Buffer.Memory;
    for (int y = 0; y < Buffer.Height; ++y)
    {
        uint32_t* pixel = (uint32_t*)row;
        for (int x = 0; x < Buffer.Width; ++x)
        {
            uint8_t blue = (x+xOffset);
            uint8_t green = (y+yOffset);
            uint8_t red = x;

            // 0x00RRGGBB in mem
            *pixel++ = ((red << 16) | (green << 8) | blue);
        }

        row += Buffer.Pitch;
    }
}

WindowDemension GetWindowDimension(HWND Window)
{
    WindowDemension dim;
    
    RECT clientRect;
    GetClientRect(Window, &clientRect);
    dim.Width = ( clientRect.right - clientRect.left );
    dim.Height = ( clientRect.bottom - clientRect.top );

    return dim;
}

void ResizeDIBSection(OffscreenBuffer& Buffer, const WindowDemension& WindowDim)
{
    if (Buffer.Memory)
    {
       VirtualFree(Buffer.Memory, 0, MEM_RELEASE); 
    }

    Buffer.Width = WindowDim.Width;
    Buffer.Height = WindowDim.Height;
    Buffer.BytesPerPixel = 4;
    Buffer.Pitch = WindowDim.Width*Buffer.BytesPerPixel;

    Buffer.Info.bmiHeader.biSize = sizeof(Buffer.Info.bmiHeader);
    Buffer.Info.bmiHeader.biWidth = Buffer.Width;
    Buffer.Info.bmiHeader.biHeight = -Buffer.Height; //NOTE:: if biHeight is negative, then this bitmap is top-down(starts from top left) instead of bottom-up(bottom left). 
    Buffer.Info.bmiHeader.biPlanes = 1;
    Buffer.Info.bmiHeader.biBitCount = 32;
    Buffer.Info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = (Buffer.Width * Buffer.Height) * Buffer.BytesPerPixel;
    Buffer.Memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

void DisplayBufferInWindow(HDC DeviceContext, const WindowDemension& WindowDim, const OffscreenBuffer& Buffer)
{
    //TODO:: Aspect ratio correction.
    //TODO:: Play with stretch modes.

    int out = StretchDIBits( 
        DeviceContext, 
        0, 0, WindowDim.Width, WindowDim.Height,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Memory,
        &Buffer.Info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK MainWindowCallback(
    HWND Window, 
    UINT Message,
    WPARAM WParam,
    LPARAM LParam)
{
    LRESULT Result = 0; 

    switch (Message)
    {
        case WM_SIZE:
        {
        } break;
        
        case WM_DESTROY:
        {
            //TODO:: Handle as error. recreate Window?
            g_bRunning = false;
        } break;
        
        case WM_CLOSE:
        {
            //TODO:: Handle with message to the user?
            g_bRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(Window, &paint);
            
            WindowDemension dimension = GetWindowDimension(Window);
            DisplayBufferInWindow(deviceContext, dimension, g_GlobalBackBuffer);

            EndPaint(Window, &paint);
        } break;

        default:
        {
            //Pass by default
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
    ResizeDIBSection(g_GlobalBackBuffer, {1280, 720});

    WNDCLASSEXA windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.hInstance = Instance; //NOTE: Can also use: GetModuleHandle(0);
    windowClass.lpszClassName = "Engine 2";
    windowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    windowClass.lpfnWndProc = MainWindowCallback;

    if(RegisterClassExA(&windowClass))
    {
        HWND window = CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "Engine 2",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0
        );

        if(window)
        {
            //Note:: CS_OWNDC has made it that there is only one device vontext
            HDC deviceContext = GetDC(window);

            int xOffset = 0;
            int yOffset = 0;

            g_bRunning = true;
            while(g_bRunning)
            {
                MSG message;
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if(message.message == WM_QUIT)
                    {
                        g_bRunning = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                RenderWeirdGradient(g_GlobalBackBuffer, xOffset++, yOffset++);
                
                WindowDemension dimension = GetWindowDimension(window);
                DisplayBufferInWindow(deviceContext, dimension, g_GlobalBackBuffer);
            }

            ReleaseDC(window, deviceContext);
        }
        else
        {
            //TODO:: Add Logging
        }
    }
    else
    {
        //TODO:: Add Logging
    }

    return(0);
}