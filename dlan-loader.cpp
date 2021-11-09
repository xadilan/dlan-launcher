// dlan-loader.cpp : Defines the entry point for the application.
//
#include "utils.h"
#include "framework.h"
#include "dlan-loader.h"
#include <CommCtrl.h>


#pragma comment(lib, "comctl32.lib")
//
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_DEFAULTPROGRESSCTRL	401
#define ID_SMOOTHPROGRESSCTRL	402
#define ID_LABEL 501
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
CString DLAN_FOLDER_NAME;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI MigrateFiles(LPVOID lpParameter);
void LaunchDlanClient(const CString& dstDir, bool hasSkippedCopy);


HWND ghMainWindow = NULL;
HWND gHSmoothProgressCtrl;
HWND gHStatusLabel;
HWND gHProgressLabel;
ULONGLONG gClientTotalByts = 0;
CString gClientPath;
CString gClientTmpFile;


// https://www.codeproject.com/Tips/250672/CenterWindow-in-WIN32
VOID CenterWindow(HWND hwndWindow)
{
    RECT rectWindow;

    GetWindowRect(hwndWindow, &rectWindow);

    int nWidth = rectWindow.right - rectWindow.left;
    int nHeight = rectWindow.bottom - rectWindow.top;

    int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    int nX = (nScreenWidth - nWidth) / 2;
    int nY = (nScreenHeight - nHeight) / 2;


    // make sure that the dialog box never moves outside of the screen
    if (nX < 0) nX = 0;
    if (nY < 0) nY = 0;
    if (nX + nWidth > nScreenWidth) nX = nScreenWidth - nWidth;
    if (nY + nHeight > nScreenHeight) nY = nScreenHeight - nHeight;

    MoveWindow(hwndWindow, nX, nY, nWidth, nHeight, FALSE);
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    if (!_tcscmp(lpCmdLine, L"-PE")) {
        DLAN_FOLDER_NAME = L"dlan_launcher_pe";
    }
    else {
        DLAN_FOLDER_NAME = L"dlan_launcher";
    }

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DLANLOADER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DLANLOADER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
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

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DLANLOADER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DLANLOADER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

    ghMainWindow = CreateWindowW(szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        530,
        200,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!ghMainWindow)
    {
        return FALSE;
    }
    CenterWindow(ghMainWindow);

    // https://github.com/malortie/Tutorials/blob/master/tutorials/cpp/win32/controls/progressbar/ProgressBar.cpp
    // Create smooth progress bar.
    gHSmoothProgressCtrl = ::CreateWindowEx(
        0,
        PROGRESS_CLASS,
        TEXT(""),
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        30,
        40,
        390,
        30,
        ghMainWindow,
        (HMENU)ID_DEFAULTPROGRESSCTRL,
        hInstance,
        NULL);


    ::SendMessage(gHSmoothProgressCtrl, PBM_SETPOS, (WPARAM)(INT)0, 0);

    gHStatusLabel = ::CreateWindowEx(
        0,
        TEXT("EDIT"),
        TEXT("正在加载 ..."),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        30,
        90,
        390,
        20,
        ghMainWindow,
        (HMENU)ID_LABEL,
        hInstance,
        NULL);

    gHProgressLabel = ::CreateWindowEx(
        0,
        TEXT("EDIT"),
        TEXT("0 %"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        430,
        40,
        100,
        20,
        ghMainWindow,
        (HMENU)ID_LABEL,
        hInstance,
        NULL);

    SetCurDirToExeDir();

    HANDLE hThread = CreateThread(
        NULL,
        0,
        MigrateFiles,
        NULL,
        0,
        NULL);

    if (hThread == NULL) {
        SetWindowText(gHStatusLabel, _T("创建线程失败"));
    }


    ShowWindow(ghMainWindow, nCmdShow);
    UpdateWindow(ghMainWindow);

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

void UpdateProgress(double percent)
{
    TCHAR msg[1024] = { 0 };
    _stprintf(msg, _T("%.0lf %%"), percent);
    SetWindowText(gHProgressLabel, msg);

    ::SendMessage(gHSmoothProgressCtrl, PBM_SETPOS, (WPARAM)(INT)percent, 0);
}


void NotifyProgress(ULONGLONG fileBytes, LPCTSTR filePath, BOOL isOk)
{
    static double copiedBytes = 0;

    copiedBytes += fileBytes;
    double percent = copiedBytes / gClientTotalByts * 100;

    UpdateProgress(percent);
    if (isOk) {
        SetWindowText(gHStatusLabel, _T("正在加载 ..."));
    }
    else {
        SetWindowText(gHStatusLabel, _T("加载出错: "));
    }
}



void ShowClientLaunchProgress()
{
    SetWindowText(gHStatusLabel, _T("客户端启动成功后，该窗口自动关闭..."));
    int launchMS = 1000;
    int sleepMS = launchMS / 100;
    for (int i = 0; i <= 100; i++) {
        UpdateProgress(i);
        Sleep(sleepMS);
    }
}


DWORD WINAPI MigrateFiles(LPVOID lpParameter)
{
    TCHAR workDir[MAX_PATH] = { 0 };
    GetWorkDir(workDir);
    CString srcDir = workDir;
    srcDir += DLAN_FOLDER_NAME;
    ULONGLONG driveRestBytes = 0;
    CString dstDir = GetBiggestDriveFromHDD(&driveRestBytes);
    dstDir += DLAN_FOLDER_NAME;
    gClientTmpFile = dstDir + "\\record.txt";

    if (!IsDirExists(srcDir)) {
        CString msg = _T("无法启动，找不到文件夹: ");
        msg += DLAN_FOLDER_NAME;
        SetWindowText(gHStatusLabel, msg);
        return 1;
    }

    gClientTotalByts = GetDirectoryBytes(srcDir);

    if (driveRestBytes < gClientTotalByts) {
        SetWindowText(gHStatusLabel, _T("硬盘空间不足，直接从光盘运行"));
        gClientPath = srcDir + "\\dlan_launcher.exe";
    }
    else {
        gClientPath = dstDir + "\\dlan_launcher.exe";
    }

    BOOL hasSkippedCopy = FALSE;
    if (dstDir[0] == 'X') {
        gClientTmpFile = "X:\\record.txt";
        hasSkippedCopy = true;
        gClientPath = srcDir + "\\dlan_launcher.exe";
        SetWindowText(gHStatusLabel, _T("检测到在虚拟机运行，跳过加载 ..."));
        Sleep(1000);
    }
    else {
        //RemoveDirectoryRecursive(dstDir);
        CopyFolder(srcDir, dstDir, NotifyProgress);
    }
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(gClientPath) && GetLastError() == ERROR_FILE_NOT_FOUND) {
        SetWindowText(gHStatusLabel, _T("找不到客户端路径 ..."));
    }
    else {
        LaunchDlanClient(dstDir, hasSkippedCopy);
    }
    return 0;
}


void LaunchDlanClient(const CString& dstDir, bool hasSkippedCopy) {
    Sleep(500);
    SetWindowText(gHStatusLabel, _T("加载完成，正在启动客户端 ..."));
    UpdateProgress(100);

    CString cmd = gClientPath + " --record_file ";
    cmd += gClientTmpFile;
    PROCESS_INFORMATION pi = RunNewProcess(cmd);

    if (NULL == pi.hProcess) {
        SetWindowText(gHStatusLabel, _T("启动客户端失败 ..."));
    }
    else
    {
        ShowClientLaunchProgress();
        ShowWindow(ghMainWindow, SW_HIDE);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    if (!PathFileExists(gClientTmpFile)) {
        CString msg = _T("找不到文件 ") + gClientTmpFile;
        SetWindowText(gHStatusLabel, msg);
        return;
    }
    // 读取客户端临时产生数据的路径
    CString clientTmpDir;
    CStdioFile input(gClientTmpFile, CFile::modeRead);
    input.ReadString(clientTmpDir);
    input.Close();
    DeleteFile(gClientTmpFile);
    if (IsDirExists(clientTmpDir)) {
        RemoveDirectoryRecursive(clientTmpDir);
    }

    // 其退出时，清除客户端文件
    if (IsDirExists(dstDir) && !hasSkippedCopy) {
        SetWindowText(gHStatusLabel, _T("正在清空残存客户端文件"));
        RemoveDirectoryRecursive(dstDir);
    }

    exit(0);
}