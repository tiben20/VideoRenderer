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


} // namespace Utility
