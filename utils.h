#pragma once
// https://blog.csdn.net/jtujtujtu/article/details/8903798
#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS
#include <afx.h>
#include <shlwapi.h>

typedef void (*SlotCallback)(ULONGLONG, LPCTSTR, BOOL);
BOOL CopyFolder(LPCTSTR pstrSrcFolder, LPCTSTR pstrDstFolder, SlotCallback callback);

ULONGLONG GetClientTotalBytesFromTextFile(LPCSTR configFilePath);
VOID SetCurDirToExeDir();

PROCESS_INFORMATION RunNewProcess(LPCTSTR lpCommandLine, LPCTSTR lpWorkDir=NULL);

CString GetBiggestDriveFromHDD(ULONGLONG* restBytes);

void GetWorkDir(LPTSTR szDir);

ULONGLONG GetDirectoryBytes(CString path);
bool IsDirExists(const CString& );
BOOL RemoveDirectoryRecursive(LPCTSTR dirName);
BOOL IsProcessRunning(DWORD pid, int timeoutMs);
void KillExistsDlanProcessInBackground();

void test();