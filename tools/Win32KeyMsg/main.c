#include <windows.h>
#include <stdio.h>

//
// Playing with keyboard input on Windows!
//

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            // https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#keystroke-message-flags

            WORD vkCode = LOWORD(wParam);
            WORD keyFlags = HIWORD(lParam);
            WORD repeatCount = LOWORD(lParam);
            WORD scanCode = LOBYTE(keyFlags);

            BOOL isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED;
            BOOL wasKeyDown = (keyFlags & KF_REPEAT) == KF_REPEAT;
            BOOL isKeyReleased = (keyFlags & KF_UP) == KF_UP;
            BOOL isAltDown = (keyFlags & KF_ALTDOWN) == KF_ALTDOWN;

            if (isExtendedKey) {
                scanCode = MAKEWORD(scanCode, 0xE0);
            }


            // if we want to distinguish these keys:
            switch (vkCode)
            {
                case VK_SHIFT:   // converts to VK_LSHIFT or VK_RSHIFT
                case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
                case VK_MENU:    // converts to VK_LMENU or VK_RMENU
                    vkCode = LOWORD(MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX));
                    break;
            }

            printf("%-8svkCode = %02X scanCode = %02X, alt = %d\n",
                isKeyReleased ? "release": "press", vkCode, scanCode, isAltDown);
            break;
        }

        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        {
            if (isprint(wParam)) {
                printf("char '%c'\n", wParam);
            }
            else {
                printf("char %02X\n", wParam);
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrevInst,
    LPSTR arg,
    int nCmdShow)
{
    LPCWSTR ClassName = L"MyWindowClass";
    LPCWSTR WindowTitle = L"My Window";

    WNDCLASSW wc = { };
    wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInst;
    wc.lpszClassName = ClassName;
    wc.lpfnWndProc = WndProc;

    if (!RegisterClassW(&wc))
    {
        return -1;
    }

    CreateWindowW(
        ClassName, WindowTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 500, 500,
        NULL, NULL, NULL, NULL);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
