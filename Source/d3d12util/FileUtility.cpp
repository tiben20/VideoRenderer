/*
 *
 * (C) 2022 Ti-BEN
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "FileUtility.h"
#include <fstream>
#include <mutex>
#include "corecrt_io.h"
#include "Utils/Util.h"


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

  bool ReadTextFile(const char* fileName, std::string& result) {
    FILE* hFile;
    if (fopen_s(&hFile, fileName, "rt") || !hFile) {
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

  std::wstring GetFilePathWrite(const wchar_t* fileName)
  {
    FILE* hFile;
    std::wstring file = fileName;
    if (_wfopen_s(&hFile, fileName, L"wb") || !hFile)
    {
      wchar_t* pValue;
      size_t len;
      errno_t err = _wdupenv_s(&pValue, &len, L"APPDATA");
      if (err != 0)
      {
        DLog(L"Couldnt get appdata directory");
        return L"";
      }
      std::wstring filestripped = file.substr(2);
      file = pValue;
      free(pValue);
      file.append(L"\\MPCVideoRenderer\\Shaders\\");

      file.append(filestripped);
      if (_wfopen_s(&hFile, file.c_str(), L"wb") || !hFile)
      {
        DLog(L"This file cant be written {}", file);
        return L"";
      }
      fclose(hFile);
    }
    return file;
  }

  std::wstring GetDirAppData()
  {
    std::wstring returnValue = L"";
    wchar_t* pValue;
    size_t len;
    errno_t err = _wdupenv_s(&pValue, &len, L"APPDATA");
    if (err != 0)
    {
      DLog(L"Couldnt get appdata directory");
      return L"";
    }

    returnValue = pValue;
    free(pValue);
    return returnValue;
  }

  std::vector<std::wstring> GetAllHlslInFolder(std::wstring folder)
  {
    std::vector<std::wstring> names;
    std::wstring search_path = folder + L"/*.hlsl";
    WIN32_FIND_DATA fd;
    HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        // read all (real) files in current folder
        // , delete '!' read other 2 default folder . and ..
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
          names.push_back(fd.cFileName);
        }
      } while (::FindNextFile(hFind, &fd));
      ::FindClose(hFind);
    }
    return names;
  }

  std::wstring GetFilePathExists(const wchar_t* fileName)
  {
    std::wstring file = fileName;
    if (!FileExists(file.c_str()))
    {
      wchar_t* pValue;
      size_t len;
      errno_t err = _wdupenv_s(&pValue, &len, L"APPDATA");
      if (err != 0)
      {
        DLog(L"Couldnt get appdata directory");
        return L"";
      }
      std::wstring filestripped;
      if (file.substr(2, 2) == L".\\")
        filestripped = file.substr(2);
      else
        filestripped = file;
      file = pValue;
      free(pValue);
      file.append(L"\\MPCVideoRenderer\\Shaders\\");
      
      file.append(filestripped);
      if (!FileExists(file.c_str()))
      {
        DLog(L"This file dosent exist {}", file);
        return L"";
      }
    }
    
    return file;
  }
}