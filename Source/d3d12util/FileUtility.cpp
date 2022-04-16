//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "stdafx.h"
#include "FileUtility.h"
#include <fstream>
#include <mutex>
#include "corecrt_io.h"
//#include <zlib.h> // From NuGet package 

using namespace std;
using namespace Utility;

namespace Utility
{
  ByteArray NullFile = make_shared<vector<BYTE> >(vector<BYTE>());


  bool replaceslash(std::wstring& str, const std::wstring& from, const std::wstring& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::wstring::npos)
      return false;
    str.replace(start_pos, from.length(), to);
    return true;
  }
  std::wstring thisDllDirPath()
  {
    std::wstring thisPath = L"";
    WCHAR path[MAX_PATH];
    HMODULE hm;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      (LPWSTR)&thisDllDirPath, &hm))
    {
      GetModuleFileNameW(hm, path, MAX_PATH);
      PathRemoveFileSpecW(path);
      thisPath = std::wstring(path);
      if (!thisPath.empty() && thisPath.at(thisPath.size() - 1) != '\\')
        thisPath += L"\\";
    }
    return thisPath;
  }
  ByteArray ReadFileHelper(const wstring& fileName)
  {
    struct _stat64 fileStat;
    std::wstring thefile;
    thefile = fileName;
    int fileExists = _wstat64(thefile.c_str(), &fileStat);

    if (fileExists == -1)
    {
      thefile = thisDllDirPath();

      thefile.append(fileName);
      replaceslash(thefile, L"/", L"\\");
      fileExists = _wstat64(thefile.c_str(), &fileStat);


    }

    if (fileExists == -1)
      return NullFile;

    ifstream file(thefile, ios::in | ios::binary);
    if (!file)
      return NullFile;

    Utility::ByteArray byteArray = make_shared<vector<BYTE> >(fileStat.st_size);
    file.read((char*)byteArray->data(), byteArray->size());
    file.close();

    return byteArray;
  }

  bool WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize)
  {
    FILE* hFile;
    if (_wfopen_s(&hFile, fileName, L"wb") || !hFile) {
      
      return false;
    }

    size_t writed = fwrite(buffer, 1, bufferSize, hFile);
    assert(writed == bufferSize);

    fclose(hFile);
    return true;
  }

  bool ReadTextFile(const wchar_t* fileName, std::string& result) {
    FILE* hFile;
    if (_wfopen_s(&hFile, fileName, L"rt") || !hFile) {
      return false;
    }
    int fd = _fileno(hFile);
    long size = _filelength(fd);

    result.clear();
    result.resize(static_cast<size_t>(size) + 1, 0);

    size_t readed = fread(result.data(), 1, size, hFile);
    result.resize(readed);

    fclose(hFile);
    return true;
  }

  bool ReadFile(const wchar_t* fileName, std::vector<BYTE>& result)
  {
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    ScopedHandle hFile(SafeHandle(CreateFile2(fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr)));
#else
    ScopedHandle hFile(SafeHandle(CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr)));
#endif

    if (!hFile) {
      
      return false;
    }

    DWORD size = GetFileSize(hFile.get(), nullptr);
    result.resize(size);

    DWORD readed;
    if (!::ReadFile(hFile.get(), result.data(), size, &readed, nullptr)) {
      
      return false;
    }

    return true;
  }

  bool DirExists(const wchar_t* fileName) {
    DWORD attrs = GetFileAttributesW(fileName);
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
  }

  bool FileExists(const wchar_t* fileName)
  {
    DWORD attrs = GetFileAttributesW(fileName);
    // exclude folders
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
  }
}