#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef UNICODE
#define UNICODE
#endif

#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <vector>
#include <set>
#include <map>
#include <Windows.h>

HWND last_active = 0;
std::set<HWND> wnd_list;
std::set<HWND> focused_wnd;
std::wstring title_list = L"Focus windows to send to then focus this window";

void CheckWindowList(HWND hwnd)
{
        HWND cur_active = GetForegroundWindow();

        if (cur_active == 0 || cur_active == last_active) return;
        last_active = cur_active;

        if (cur_active == hwnd) {
                KillTimer(hwnd, 1);
                return;
        }

        if (!(wnd_list.insert(cur_active)).second) return;

        DWORD pid, tid = GetWindowThreadProcessId(cur_active, &pid);
        if (!AttachThreadInput(GetCurrentThreadId(), tid, TRUE)) return;
        HWND cur_focus = GetFocus();
        AttachThreadInput(GetCurrentThreadId(), tid, FALSE);

        if (cur_focus == 0) cur_focus = GetTopWindow(cur_active);

        if (cur_focus == 0) return;

        if (!(focused_wnd.insert(cur_focus)).second) return;

        WCHAR title[256] = L"\r\n";
        GetWindowText(cur_active, title + 2, 256 - 2);
        
        title_list.append(title);

        InvalidateRect(hwnd, 0, TRUE);
}

class Paint
{
public:
        const HWND hwnd;
        const HDC dc;
        PAINTSTRUCT ps;

        Paint(HWND window, PAINTSTRUCT paint_struct = { 0 })
                : ps(paint_struct), hwnd(window), dc(BeginPaint(hwnd, &ps)) {}

        ~Paint()
        {
                EndPaint(hwnd, &ps);
        }
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
        switch (uMsg)
        {
                case WM_DESTROY:
                {
                        PostQuitMessage(0);
                        return 0;
                }
                case WM_TIMER:
                {
                        if (wParam != 1) break;

                        if (last_active != hwnd) CheckWindowList(hwnd);
                        return 0;
                }
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYUP:
                case WM_KEYDOWN:
                {
                        for (auto wnd : focused_wnd)
                        {
                                PostMessage(wnd, uMsg, wParam, lParam);
                        }
                        return 0;
                }
                case WM_PAINT:
                {
                        Paint paint(hwnd);
                        RECT rect;
                        GetClientRect(hwnd, &rect);
                        FillRect(paint.dc, &paint.ps.rcPaint, GetSysColorBrush(DC_BRUSH));
                        DrawText(paint.dc, title_list.c_str(), title_list.size(), &rect, DT_EDITCONTROL);
                        return 0;
                }
                default:
                {
                        return DefWindowProc(hwnd, uMsg, wParam, lParam);
                }
        }
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
        const wchar_t CLASS_NAME[] = L"Sample Window Class";

        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClassEx(&wc);

        HWND hwnd = CreateWindowEx(
                0,
                CLASS_NAME,
                L"Keyboard Broadcast",
                WS_OVERLAPPEDWINDOW | SW_SHOWNOACTIVATE,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 500,
                NULL,
                NULL,
                hInstance,
                NULL
        );

        if (hwnd == NULL) return 0;

        SetTimer(hwnd, 1, 20, 0);

        last_active = GetForegroundWindow();

        ShowWindow(hwnd, SW_SHOWNOACTIVATE);

        MSG msg = { 0 };
        while (GetMessage(&msg, NULL, 0, 0))
        {
                DispatchMessage(&msg);
        }

        return msg.wParam;
}
