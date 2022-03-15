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
//#include <zlib.h> // From NuGet package 

using namespace std;
using namespace Utility;

namespace Utility
{
    ByteArray NullFile = make_shared<vector<BYTE> > (vector<BYTE>() );


ByteArray ReadFileHelper(const wstring& fileName)
{
    struct _stat64 fileStat;
    int fileExists = _wstat64(fileName.c_str(), &fileStat);
    if (fileExists == -1)
        return NullFile;

    ifstream file( fileName, ios::in | ios::binary );
    if (!file)
        return NullFile;

    Utility::ByteArray byteArray = make_shared<vector<BYTE> >( fileStat.st_size );
    file.read( (char*)byteArray->data(), byteArray->size() );
    file.close();

    return byteArray;
}

}