#include "utils.h"
#include <tchar.h>


BOOL IsDirectory(LPCTSTR pstrPath)
{
    if (NULL == pstrPath)
    {
        return FALSE;
    }

    /*ȥ��·��ĩβ�ķ�б��*/
    CString strPath = pstrPath;
    if (strPath.Right(1) == _T('\\'))
    {
        strPath.Delete(strPath.GetLength() - 1);
    }

    CFileFind finder;
    BOOL bRet = finder.FindFile(strPath);
    if (!bRet)
    {
        return FALSE;
    }

    /*�жϸ�·���Ƿ���Ŀ¼*/
    finder.FindNextFile();
    bRet = finder.IsDirectory();
    finder.Close();
    return bRet;
}



BOOL CopyFolder(LPCTSTR pstrSrcFolder, LPCTSTR pstrDstFolder, SlotCallback SlotNotifyCallback)
{
    if (NULL == pstrSrcFolder || NULL == pstrSrcFolder)
    {
        return FALSE;
    }

    /*���ԴĿ¼�Ƿ��ǺϷ�Ŀ¼*/
    if (!IsDirectory(pstrSrcFolder))
    {
        return FALSE;
    }

    /*���ļ�����ת��ΪCString���ͣ���ȷ��Ŀ¼��·��ĩβΪ��б��*/
    CString strSrcFolder(pstrSrcFolder);
    if (strSrcFolder.Right(1) != _T('\\'))
    {
        strSrcFolder += _T("\\");
    }
    CString strDstFolder(pstrDstFolder);
    if (strDstFolder.Right(1) != _T("\\"))
    {
        strDstFolder += _T("\\");
    }

    /*�����Ŀ¼�����ƣ�ֱ�ӷ��ظ��Ƴɹ�*/
    if (0 == _tcscmp(strSrcFolder, strDstFolder))
    {
        return TRUE;
    }

    /*���Ŀ��Ŀ¼������,�򴴽�Ŀ¼*/
    if (!IsDirectory(strDstFolder))
    {
        if (!CreateDirectory(strDstFolder, NULL))
        {
            /*����Ŀ��Ŀ¼ʧ��*/
            return FALSE;
        }
    }

    /*����ԴĿ¼�в����ļ���ͨ���*/
    CString strWildcard(strSrcFolder);
    strWildcard += _T("*.*");

    /*���ļ����ң��鿴ԴĿ¼���Ƿ����ƥ����ļ�*/
    /*����FindFile�󣬱������FindNextFile���ܻ�ò����ļ�����Ϣ*/
    CFileFind finder;
    BOOL bWorking = finder.FindFile(strWildcard);

    while (bWorking)
    {
        /*������һ���ļ�*/
        bWorking = finder.FindNextFile();

        /*������ǰĿ¼��.������һ��Ŀ¼��..��*/
        if (finder.IsDots())
        {
            continue;
        }

        /*�õ���ǰҪ���Ƶ�Դ�ļ���·��*/
        CString strSrcFile = finder.GetFilePath();

        /*�õ�Ҫ���Ƶ�Ŀ���ļ���·��*/
        CString strDstFile(strDstFolder);
        strDstFile += finder.GetFileName();

        /*�жϵ�ǰ�ļ��Ƿ���Ŀ¼,*/
        /*�����Ŀ¼���ݹ���ø���Ŀ¼,*/
        /*����ֱ�Ӹ����ļ�*/
        if (finder.IsDirectory())
        {
            if (!CopyFolder(strSrcFile, strDstFile, SlotNotifyCallback))
            {
                finder.Close();
                return FALSE;
            }
        }
        else
        {
#ifdef _DEBUG
            // ���Ի�����Ӳ�̿����ļ�̫�죬������һ˲������ˣ���˼����ӳ��Թ���Ч
            Sleep(1);
#endif
            if (!CopyFile(strSrcFile, strDstFile, FALSE))
            {
                finder.Close();
                return FALSE;
            }
            SlotNotifyCallback(finder.GetLength(), finder.GetFileName());
        }

    } /*while (bWorking)*/

    /*�ر��ļ�����*/
    finder.Close();

    return TRUE;
}


// http://cplusrun.com/WinAPI/Set_Current_Working_Directory_to_EXE_Path
void SetCurDirToExeDir() {
    WCHAR ModuleFileName[MAX_PATH] = { 0 };
    WCHAR Cwd[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, ModuleFileName, MAX_PATH);
    wcsncpy(Cwd, ModuleFileName, wcslen(ModuleFileName) - wcslen(wcsrchr(ModuleFileName, L'\\')) + 1);
    SetCurrentDirectory(Cwd);
}


void GetWorkDir(LPTSTR szDir)
{
    ASSERT(szDir != NULL);
    WCHAR ModuleFileName[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, ModuleFileName, MAX_PATH);
    wcsncpy(szDir, ModuleFileName, wcslen(ModuleFileName) - wcslen(wcsrchr(ModuleFileName, L'\\')) + 1);
}


ULONGLONG GetClientTotalBytesFromTextFile(LPCSTR configFilePath)
{
    FILE* fp = fopen(configFilePath, "r");
    if (NULL != fp) {
        ULONGLONG totalBytes = 0;
        fscanf(fp, "%llu", &totalBytes);
        fclose(fp);
        return totalBytes;
    }
    return 0;
}

void RunNewProcess(LPCTSTR lpCommandLine, LPCTSTR lpWorkDir) {
    LPTSTR cmd = _tcsdup(lpCommandLine);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL,
        cmd,
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_PROCESS_GROUP,
        NULL,
        lpWorkDir,
        &si,
        &pi))
    {
        _tprintf(_T("CreateProcess failed %s (%d).\n"), lpCommandLine, GetLastError());
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    free(cmd);
}


CString GetBiggestDriveFromHDD(ULONGLONG* restBytes)
{
    DWORD cchBuffer;
    WCHAR* driveStrings;
    UINT driveType;
    ULARGE_INTEGER freeSpace;

    // Find out how big a buffer we need
    cchBuffer = GetLogicalDriveStrings(0, NULL);

    driveStrings = (WCHAR*)malloc((cchBuffer + 1) * sizeof(TCHAR));
    if (driveStrings == NULL)
    {
        return "";
    }
    WCHAR* buf = driveStrings;
    // Fetch all drive strings    
    GetLogicalDriveStrings(cchBuffer, driveStrings);


    ULONGLONG biggestSize = 0;
    // Loop until we find the final '\0'
    // driveStrings is a double null terminated list of null terminated strings)
    CString maxDrive;
    while (*driveStrings)
    {
        // Dump drive information
        driveType = GetDriveType(driveStrings);
        if (driveType == DRIVE_FIXED) {
            GetDiskFreeSpaceEx(driveStrings, &freeSpace, NULL, NULL);
            if (freeSpace.QuadPart > biggestSize) {
                biggestSize = freeSpace.QuadPart;
                maxDrive = driveStrings;
                if (restBytes != NULL) {
                    *restBytes = biggestSize;
                }
            }
        }

        // Move to next drive string
        // +1 is to move past the null at the end of the string.
        driveStrings += lstrlen(driveStrings) + 1;
    }

    free(buf);
    return maxDrive;
}



ULONGLONG GetDirectoryBytes(CString path)
{
    WIN32_FIND_DATA data;
    ULONGLONG totalBytes = 0;
    CString fname = path + "\\*.*";
    HANDLE h = FindFirstFile(fname, &data);
    if (h != INVALID_HANDLE_VALUE)
    {
        do {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                // make sure we skip "." and "..".  Have to use strcmp here because
                // some file names can start with a dot, so just testing for the 
                // first dot is not suffient.
                if (_tcscmp(data.cFileName, L".") != 0 && _tcscmp(data.cFileName, L"..") != 0)
                {
                    // We found a sub-directory, so get the files in it too
                    fname = path + "\\" + data.cFileName;
                    // recurrsion here!
                    totalBytes += GetDirectoryBytes(fname);
                }

            }
            else
            {
                LARGE_INTEGER sz;
                // All we want here is the file size.  Since file sizes can be larger
                // than 2 gig, the size is reported as two DWORD objects.  Below we
                // combine them to make one 64-bit integer.
                sz.LowPart = data.nFileSizeLow;
                sz.HighPart = data.nFileSizeHigh;
                totalBytes += sz.QuadPart;

            }
        } while (FindNextFile(h, &data) != 0);
        FindClose(h);

    }
    return totalBytes;
}


bool IsDirExists(const CString& dirName_in)
{
    DWORD ftyp = GetFileAttributes(dirName_in);
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}


void test() {
    SetCurDirToExeDir();
    ULONGLONG t = GetClientTotalBytesFromTextFile("size.bin");
    TCHAR workDir[256] = { 0 };
    GetWorkDir(workDir);
    Sleep(1);
}