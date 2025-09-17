#include "framework.h"
#include "main.h"
#include <strsafe.h>
#include <shellapi.h>

#define MAX_LOADSTRING 100
#define ID_TRAY_ICON 1
#define WM_TRAYICONMSG (WM_USER + 1)
#define IDT_TIMER 1

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
WCHAR szCmdLine[MAX_PATH];

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL                CreateTrayIcon(HWND);
BOOL                DestroyTrayIcon(HWND);
BOOL                IsVistaOrLater();
BOOL                IsXPOrLater();
VOID CALLBACK       TimerProc(HWND, UINT, UINT, DWORD);
BOOL                VerifyWindowsVersion(DWORD, DWORD, WORD, WORD);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // No need to proceed if user didn't give any command-line string.
    size_t length;
    if (SUCCEEDED(StringCchLength(lpCmdLine, MAX_PATH, &length))) 
    {
        if (length == 0) return 0;
        StringCchCopyW(szCmdLine, MAX_PATH, lpCmdLine);
    }
    else
    {
        return 0;
    }

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_APP_CLASS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_APP_CLASS));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SRC));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_APP_CLASS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   // Don't display the window of this program. Just keep it hidden.
   //ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // Create one-time timer to create system tray icon and display notification.
        if (!SetTimer(hWnd, IDT_TIMER, USER_TIMER_MINIMUM, (TIMERPROC)TimerProc))
        {
            DestroyWindow(hWnd);
        }
        break;
    case WM_TRAYICONMSG:
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:		// Tray icon double-clicked.
        case WM_LBUTTONDOWN:
        case WM_CONTEXTMENU:
        case WM_RBUTTONDOWN:
        case NIN_SELECT:
        case NIN_KEYSELECT:
        case NIN_BALLOONTIMEOUT:	// Balloon tip timed out.
        case NIN_BALLOONUSERCLICK:	// User clicked the balloon tip. 
            //MessageBox(hWnd, L"closing", L"closing app", MB_OK);
            DestroyTrayIcon(hWnd);
            DestroyWindow(hWnd);
            break;
        default:					// All other messages. Reference: https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

BOOL CreateTrayIcon(HWND hWnd)
{
    NOTIFYICONDATA nid;

    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAY_ICON;
    nid.uVersion = NOTIFYICON_VERSION;
    nid.uCallbackMessage = WM_TRAYICONMSG;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    nid.uTimeout = 15 * 1000;		// 15 seconds. Minimum 10 seconds and maximum 30 seconds.

    // Show notification message silently (for Windows XP or later).
    if (IsXPOrLater()) nid.dwInfoFlags |= NIIF_NOSOUND;
    // Show larger icon (Vista or later) in notification, or regular icon for earlier Windows.
    if (IsVistaOrLater()) nid.dwInfoFlags |= NIIF_LARGE_ICON;

    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), szTitle);
    StringCchCopy(nid.szInfo, ARRAYSIZE(nid.szInfo), szCmdLine);
    StringCchCopy(nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle), L"Notification");

    return Shell_NotifyIcon(NIM_ADD, &nid);
}

BOOL DestroyTrayIcon(HWND hWnd)
{
    NOTIFYICONDATA nid;

    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAY_ICON;

    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

BOOL IsVistaOrLater()
{
    // For Windows Vista.
    return VerifyWindowsVersion(6, 0, 0, 0);
}

BOOL IsXPOrLater()
{
    // For Windows XP.
    return VerifyWindowsVersion(5, 1, 0, 0);
}

BOOL VerifyWindowsVersion(DWORD dwMajorVersion, DWORD dwMinorVersion, WORD wServicePackMajor, WORD wServicePackMinor)
{
    OSVERSIONINFOEX osvi;
    DWORDLONG dwlConditionMask = 0;
    int op = VER_GREATER_EQUAL;

    // Initialize the OSVERSIONINFOEX structure.
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = dwMajorVersion;
    osvi.dwMinorVersion = dwMinorVersion;
    osvi.wServicePackMajor = wServicePackMajor;
    osvi.wServicePackMinor = wServicePackMinor;

    // Initialize the condition mask.
    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, op);
    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMINOR, op);

    // Perform the test.
    return VerifyVersionInfo(&osvi,
        VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        dwlConditionMask);
}

VOID CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime)
{
    // Kill the one-time timer.
    KillTimer(hWnd, IDT_TIMER);

    // Create system tray icon and display notification.
    if (!CreateTrayIcon(hWnd))
    {
        DestroyWindow(hWnd);
    }
}