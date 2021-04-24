#pragma once
#include <afx.h>

typedef void (*SlotCallback)(ULONGLONG, LPCTSTR);
BOOL CopyFolder(LPCTSTR pstrSrcFolder, LPCTSTR pstrDstFolder, SlotCallback callback);

ULONGLONG GetClientTotalBytesFromTextFile(LPCSTR configFilePath);
VOID SetCurDirToExeDir();

void RunNewProcess(LPCTSTR lpCommandLine, LPCTSTR lpWorkDir=NULL);

CString GetBiggestDriveFromHDD(ULONGLONG* restBytes);

void GetWorkDir(LPTSTR szDir);

ULONGLONG GetDirectoryBytes(CString path);
bool IsDirExists(const CString& );

void test();