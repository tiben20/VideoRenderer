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

#pragma once


#include <vector>
#include <string>
#include <rpc.h>
#include <ppl.h>
#include "ppltasks.h"
namespace Utility
{
    using namespace std;
    using namespace concurrency;

    typedef shared_ptr<vector<unsigned char> > ByteArray;
    extern ByteArray NullFile;

    // Reads the entire contents of a binary file.  If the file with the same name except with an additional
    // ".gz" suffix exists, it will be loaded and decompressed instead.
    // This operation blocks until the entire file is read.
    ByteArray ReadFileHelper(const wstring& fileName);

    // Same as previous except that it does not block but instead returns a task.
    task<ByteArray> ReadFileAsync(const wstring& fileName);

    bool ReadTextFile(const wchar_t* fileName, std::string& result);
    
    struct HandleCloser { void operator()(HANDLE h) noexcept { assert(h != INVALID_HANDLE_VALUE); if (h) CloseHandle(h); } };

    using ScopedHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HandleCloser>;

    static HANDLE SafeHandle(HANDLE h) noexcept { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

    bool ReadFile(const wchar_t* fileName, std::vector<BYTE>& result);
    bool WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize);

    bool DirExists(const wchar_t* fileName);
    bool FileExists(const wchar_t* fileName);

    std::vector<std::wstring> GetAllHlslInFolder(std::wstring folder);
    std::wstring GetDirAppData();
    std::wstring GetFilePathWrite(const wchar_t* fileName);
    std::wstring GetFilePathExists(const wchar_t* fileName);
} // namespace Utility
