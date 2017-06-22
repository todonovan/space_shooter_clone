#include <windows.h>

/* The main callback for our window. This function will handle all
   messages passed from Windows. The default case must be maintained
   to ensure that messages not explicitly handled are handled by Windows. */
LRESULT CALLBACK AsteroidsWindowCallback(HWND WindHandle,
                                         UINT Message,
                                         WPARAM wParam,
                                         LPARAM lParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        // To do: handle all painting, resize, etc.
        case WM_ACTIVATEAPP:
        {
            OutputDebugString("Activate app");
            return 0;            
        }
        case WM_PAINT:
        {
            static int Color = 0;

            PAINTSTRUCT PaintStruct;
            RECT ClientRect;
            HBRUSH Brush;
            if (Color == 0)
            {
                Brush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                Color = 1;
            }
            else
            {
                Brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
                Color = 0;
            }
            HDC PaintDC = BeginPaint(WindHandle, &PaintStruct);
            GetClientRect(WindHandle, &ClientRect);
            FillRect(PaintDC, &ClientRect, Brush);
            EndPaint(WindHandle, &PaintStruct);
            return 0;
        }
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_UP:
                {
                    OutputDebugString("Up arrow");
                    return 0;
                }
                default:
                {
                    OutputDebugString("Other key");
                    return 0;
                }
            }
        }
        default:
        {
            Result = DefWindowProc(WindHandle, Message, wParam, lParam);
        }
    }

    return Result;
}

/* The graphical entry point for Windows. */
int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CommandLine,
                     int CommandShow)
{
    WNDCLASS WindowClass = {}; // Ensure the struct is zeroed.
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = &AsteroidsWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "AsteroidsWindow";

    // RegisterClass returns an ATOM, which we likely will not need to store.
    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(0, "AsteroidsWindow", "Asteroids", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

        if (WindowHandle)
        {
            for (;;)
            {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else
                {
                    break;
                }
            }
        }
    }   

    return 0;
}