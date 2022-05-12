/*
* (C) 2019-2022 see Authors.txt
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

#include <d3dcommon.h>

enum :int {
	SHADER_CONVERT_NONE = 0,
	SHADER_CONVERT_TO_SDR,
	SHADER_CONVERT_TO_PQ,
};

HRESULT CompileShader(const std::string& srcCode, const char* entryPoint, LPCSTR target, ID3DBlob** ppShaderBlob, const char* sourceName, ID3DInclude* d3dinc);

HRESULT CompileShader(const std::string& srcCode, const D3D_SHADER_MACRO* pDefines, LPCSTR pTarget, ID3DBlob** ppCode);
