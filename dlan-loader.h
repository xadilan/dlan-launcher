#pragma once

#include "resource.h"
void NotifyProgress(ULONGLONG fileBytes, LPCTSTR filePath, BOOL isOk);
void UpdateProgress(double percent);
