#include <stdint.h>
#include <string>
#include <Windows.h>

//TODO: This is a global for now
static bool bRunning = false;
static BITMAPINFO BitmapInfo;
static void* BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static int bytesPerPixel = 4;

void RenderIt(int xOffset, int yOffset)
{
    int pitch = BitmapWidth*bytesPerPixel;

    uint8_t* row = (uint8_t*)BitmapMemory;
    for (int y = 0; y < BitmapHeight; ++y)
    {
        uint32_t* pixel = (uint32_t*)row;
        for (int x = 0; x < BitmapWidth; ++x)
        {
            uint8_t blue = (x+xOffset);
            uint8_t green = (y+yOffset);

            *pixel++ = ((green << 8) | blue);
        }

        row += pitch;
    }
}

void ResizeDIBSection(int Width, int Height)
{
    if (BitmapMemory)
    {
       VirtualFree(BitmapMemory, 0, MEM_RELEASE); 
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bytesPerPixel = 4;
    int bitmapMemorySize = (BitmapWidth * BitmapHeight) * bytesPerPixel;

    BitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    RenderIt(0,0);
}

void UpdateOurWindow(HDC DeviceContext, const RECT& ClientRect, int X, int Y, int Width, int Height)
{
    int windowWidth = ClientRect.right - ClientRect.left;
    int windowHeight = ClientRect.bottom - ClientRect.top;

    int out = StretchDIBits( 
        DeviceContext, 
        0, 0, BitmapWidth, BitmapHeight,
        0, 0, windowWidth, windowHeight,
        BitmapMemory,
        &BitmapInfo,
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
            RECT clientRect;
            GetClientRect(Window, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;
            ResizeDIBSection(width, height);
        } break;
        
        case WM_DESTROY:
        {
            //TODO:: Handle as error. recreate Window?
            bRunning = false;
        } break;
        
        case WM_CLOSE:
        {
            //TODO:: Handle with message to the user?
            bRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(Window, &paint);
            
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            
            PatBlt(deviceContext, x, y, width, height, BLACKNESS);
            EndPaint(Window, &paint);

            RECT clientRect;
            GetClientRect(Window, &clientRect);

            UpdateOurWindow(deviceContext, clientRect, x, y, width, height);
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
    WNDCLASSEXA windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.hInstance = Instance; //NOTE: Can also use: GetModuleHandle(0);
    windowClass.lpszClassName = "Engine 2";
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
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
            int xOffset = 0;
            int yOffset = 0;

            bRunning = true;
            while(bRunning)
            {
                MSG message;
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if(message.message == WM_QUIT)
                    {
                        bRunning = false;
                    }

                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                RenderIt(xOffset++, yOffset++);

                HDC deviceContext = GetDC(window);
                {
                    RECT clientRect;
                    GetClientRect(window, &clientRect);

                    int windowWidth = clientRect.right - clientRect.left;
                    int windowHeight = clientRect.bottom - clientRect.top;

                    UpdateOurWindow(deviceContext, clientRect, 0, 0, windowWidth, windowHeight);
                }
                ReleaseDC(window, deviceContext);
            }
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