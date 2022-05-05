/*
* (C) 2019-2021 see Authors.txt
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
#include <D3Dcompiler.h>
#include "Helper.h"
#include "resource.h"
#include "IVideoRenderer.h"
#include "Shaders.h"

#define CLAMP_IN_RECT 0

HRESULT CompileShader(const std::string& srcCode, const char* entryPoint, LPCSTR target, ID3DBlob** ppShaderBlob, const char* sourceName, ID3DInclude* d3dinc)
{
	//ASSERT(*ppShaderBlob == nullptr);

	static HMODULE s_hD3dcompilerDll = LoadLibraryW(L"d3dcompiler_47.dll");
	static pD3DCompile s_fnD3DCompile = nullptr;

	if (s_hD3dcompilerDll && !s_fnD3DCompile) {
		s_fnD3DCompile = (pD3DCompile)GetProcAddress(s_hD3dcompilerDll, "D3DCompile");
	}

	if (!s_fnD3DCompile) {
		return E_FAIL;
	}

	ID3DBlob* pErrorBlob = nullptr;
	HRESULT hr = s_fnD3DCompile(srcCode.c_str(), srcCode.size(), sourceName, nullptr,d3dinc, entryPoint,target, 0, 0, ppShaderBlob, &pErrorBlob);

	if (FAILED(hr)) {
		SAFE_RELEASE(*ppShaderBlob);

		if (pErrorBlob) {
			std::string strErrorMsgs((char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
			DLog(A2WStr(strErrorMsgs));
		}
		else {
			DLog(L"Unexpected compiler error");
		}
	}

	SAFE_RELEASE(pErrorBlob);

	return hr;
}

HRESULT CompileShader(const std::string& srcCode, const D3D_SHADER_MACRO* pDefines, LPCSTR pTarget, ID3DBlob** ppShaderBlob)
{
	//ASSERT(*ppShaderBlob == nullptr);

	static HMODULE s_hD3dcompilerDll = LoadLibraryW(L"d3dcompiler_47.dll");
	static pD3DCompile s_fnD3DCompile = nullptr;

	if (s_hD3dcompilerDll && !s_fnD3DCompile) {
		s_fnD3DCompile = (pD3DCompile)GetProcAddress(s_hD3dcompilerDll, "D3DCompile");
	}

	if (!s_fnD3DCompile) {
		return E_FAIL;
	}

	ID3DBlob* pErrorBlob = nullptr;
	HRESULT hr = s_fnD3DCompile(srcCode.c_str(), srcCode.size(), nullptr, pDefines, nullptr, "main", pTarget, 0, 0, ppShaderBlob, &pErrorBlob);

	if (FAILED(hr)) {
		SAFE_RELEASE(*ppShaderBlob);

		if (pErrorBlob) {
			std::string strErrorMsgs((char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
			DLog(A2WStr(strErrorMsgs));
		} else {
			DLog(L"Unexpected compiler error");
		}
	}

	SAFE_RELEASE(pErrorBlob);

	return hr;
}

const char code_CatmullRom_weights[] =
	"float2 t2 = t * t;\n"
	"float2 t3 = t * t2;\n"
	"float2 w0 = t2 - (t3 + t) / 2;\n"
	"float2 w1 = t3 * 1.5 + 1 - t2 * 2.5;\n"
	"float2 w2 = t2 * 2 + t / 2 - t3 * 1.5;\n"
	"float2 w3 = (t3 - t2) / 2;\n";

const char code_Bicubic_UV[] =
	"float2 Q0 = c00 * w0.x + c10 * w1.x + c20 * w2.x + c30 * w3.x;\n"
	"float2 Q1 = c01 * w0.x + c11 * w1.x + c21 * w2.x + c31 * w3.x;\n"
	"float2 Q2 = c02 * w0.x + c12 * w1.x + c22 * w2.x + c32 * w3.x;\n"
	"float2 Q3 = c03 * w0.x + c13 * w1.x + c23 * w2.x + c33 * w3.x;\n"
	"colorUV = Q0 * w0.y + Q1 * w1.y + Q2 * w2.y + Q3 * w3.y;\n";
