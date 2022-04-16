/*
 * the shader caching coming from https://github.com/Blinue/Magpie
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
#include "ShadersLoader.h"
#include "ShadersCache.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <mutex>
#include <filesystem>
#include <regex>
#include "Utility.h"
#include "FileUtility.h"
#include <bitset>
#include <unordered_set>
#include <charconv>
#include "ctype.h"
#include "helper.h"
#include "shaders.h"
#include "winternl.h"
#include <istream>
#include <yas/mem_streams.hpp>
#include <yas/binary_oarchive.hpp>
#include <yas/binary_iarchive.hpp>
#include <yas/types/std/pair.hpp>
#include <yas/types/std/string.hpp>
#include <yas/types/std/vector.hpp>
#include "d3dcompiler.h"
#pragma comment( lib, "bcrypt.lib" )
using namespace std::literals::string_literals;

template<typename Archive>
void serialize(Archive& ar, CComPtr<ID3DBlob>& o) {
	SIZE_T size = 0;
	ar& size;
	HRESULT hr = D3DCreateBlob(size, &o);
	if (FAILED(hr)) {
		
		throw new std::exception();
	}

	BYTE* buf = (BYTE*)o->GetBufferPointer();
	for (SIZE_T i = 0; i < size; ++i) {
		ar& (*buf++);
	}
}

template<typename Archive>
void serialize(Archive& ar, const CComPtr<ID3DBlob>& o) {
	SIZE_T size = o->GetBufferSize();
	ar& size;

	BYTE* buf = (BYTE*)o->GetBufferPointer();
	for (SIZE_T i = 0; i < size; ++i) {
		ar& (*buf++);
	}
}

template<typename Archive>
void serialize(Archive& ar, const ShaderConstantDesc& o) {
	size_t index = o.defaultValue.index();
	ar& index;

	if (index == 0) {
		ar& std::get<0>(o.defaultValue);
	}
	else {
		ar& std::get<1>(o.defaultValue);
	}

	ar& o.label;

	index = o.maxValue.index();
	ar& index;
	if (index == 1) {
		ar& std::get<1>(o.maxValue);
	}
	else if (index == 2) {
		ar& std::get<2>(o.maxValue);
	}

	index = o.minValue.index();
	ar& index;
	if (index == 1) {
		ar& std::get<1>(o.minValue);
	}
	else if (index == 2) {
		ar& std::get<2>(o.minValue);
	}

	index = o.currentValue.index();
	ar& index;
	if (index == 0) {
		ar& std::get<0>(o.currentValue);
	}
	else {
		ar& std::get<1>(o.currentValue);
	}

	

	ar& o.name& o.type;
}

template<typename Archive>
void serialize(Archive& ar, ShaderConstantDesc& o) {
	size_t index = 0;
	ar& index;

	if (index == 0) {
		o.defaultValue.emplace<0>();
		ar& std::get<0>(o.defaultValue);
	}
	else {
		o.defaultValue.emplace<1>();
		ar& std::get<1>(o.defaultValue);
	}

	ar& o.label;

	ar& index;
	if (index == 0) {
		o.maxValue.emplace<0>();
	}
	else if (index == 1) {
		o.maxValue.emplace<1>();
		ar& std::get<1>(o.maxValue);
	}
	else {
		o.maxValue.emplace<2>();
		ar& std::get<2>(o.maxValue);
	}

	ar& index;
	if (index == 0) {
		o.minValue.emplace<0>();
	}
	else if (index == 1) {
		o.minValue.emplace<1>();
		ar& std::get<1>(o.minValue);
	}
	else {
		o.minValue.emplace<2>();
		ar& std::get<2>(o.minValue);
	}


	ar& index;
	if (index == 0) {
		o.currentValue.emplace<0>();
		ar& std::get<0>(o.currentValue);
	}
	else {
		o.currentValue.emplace<1>();
		ar& std::get<1>(o.currentValue);
	}
	
	ar& o.name& o.type;
}

template<typename Archive>
void serialize(Archive& ar, ShaderValueConstantDesc& o) {
	ar& o.name& o.type& o.valueExpr;
}

template<typename Archive>
void serialize(Archive& ar, ShaderIntermediateTextureDesc& o) {
	ar& o.format& o.name& o.source& o.sizeExpr;
}

template<typename Archive>
void serialize(Archive& ar, ShaderSamplerDesc& o) {
	ar& o.filterType& o.addressType& o.name;
}

template<typename Archive>
void serialize(Archive& ar, ShaderPassDesc& o) {
	ar& o.inputs& o.outputs& o.cso;
}

template<typename Archive>
void serialize(Archive& ar, ShaderDesc& o) {
	ar& o.outSizeExpr& o.shaderDescription& o.constants& o.valueConstants& o.dynamicValueConstants& o.textures& o.samplers& o.passes;
}


std::wstring ConvertFileName(const wchar_t* fileName)
{
	std::wstring file(fileName);

	// remove the path from the filename
	size_t pos = file.find_last_of('\\');
	if (pos != std::wstring::npos) {
		file.erase(0, pos + 1);
	}

	std::replace(file.begin(), file.end(), '.', '_');
	return file;
}


Hasher::~Hasher()
{
	if (_hAlg) {
		BCryptCloseAlgorithmProvider(_hAlg, 0);
	}
	if (_hashObj) {
		HeapFree(GetProcessHeap(), 0, _hashObj);
	}
	if (_hHash) {
		BCryptDestroyHash(_hHash);
	}
}

bool Hasher::Initialize()
{
	NTSTATUS status = BCryptOpenAlgorithmProvider(&_hAlg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
	if (!NT_SUCCESS(status)) {
		
		return false;
	}

	ULONG result;

	status = BCryptGetProperty(_hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&_hashObjLen, sizeof(_hashObjLen), &result, 0);
	if (!NT_SUCCESS(status)) {
		
		return false;
	}

	_hashObj = HeapAlloc(GetProcessHeap(), 0, _hashObjLen);
	if (!_hashObj) {
		
		return false;
	}

	status = BCryptGetProperty(_hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&_hashLen, sizeof(_hashLen), &result, 0);
	if (!NT_SUCCESS(status))
	{
		
		return false;
	}

	status = BCryptCreateHash(_hAlg, &_hHash, (PUCHAR)_hashObj, _hashObjLen, NULL, 0, BCRYPT_HASH_REUSABLE_FLAG);
	if (!NT_SUCCESS(status))
	{
		
		return false;
	}

	
	return true;
}

bool Hasher::Hash(void* data, size_t len, std::vector<BYTE>& result) {
	result.resize(_hashLen);

	NTSTATUS status = BCryptHashData(_hHash, (PUCHAR)data, (ULONG)len, 0);
	if (!NT_SUCCESS(status)) {
		
		return false;
	}

	status = BCryptFinishHash(_hHash, result.data(), (ULONG)result.size(), 0);
	if (!NT_SUCCESS(status)) {
		
		return false;
	}

	return true;
}

std::wstring ShaderCache::_GetCacheFileName(const wchar_t* fileName, std::string_view hash)
{
	std::wstring fname = ConvertFileName(fileName);
	std::wstring fhash = Utility::UTF8ToUTF16(hash);
	std::wstring fsuffix = _SUFFIX;

	return fmt::format(L".\\cache\\{}_{}.{}", fname, fhash, fsuffix);
}

void ShaderCache::_AddToMemCache(const std::wstring& cacheFileName, const ShaderDesc& desc) {
	_memCache[cacheFileName] = desc;

	if (_memCache.size() > _MAX_CACHE_COUNT) {
		// clear half of the memory cache
		auto it = _memCache.begin();
		std::advance(it, _memCache.size() / 2);
		_memCache.erase(_memCache.begin(), it);

		
	}
}


bool ShaderCache::Load(const wchar_t* fileName, std::string_view hash, ShaderDesc& desc)
{
	std::wstring cacheFileName = _GetCacheFileName(fileName, hash);

	auto it = _memCache.find(cacheFileName);
	if (it != _memCache.end()) {
		desc = it->second;
		return true;
	}

	if (!Utility::FileExists(cacheFileName.c_str()))
	{
		std::wstring appdata = _wgetenv(L"APPDATA");

		appdata.append(L"\\MPCVideoRenderer\\Shaders\\cache\\");
		cacheFileName.replace(0, 8, appdata);
		if (!Utility::FileExists(cacheFileName.c_str()))
			return false;
	}

	std::vector<BYTE> buf;
	if (!Utility::ReadFile(cacheFileName.c_str(), buf) || buf.empty()) {
		return false;
	}

	if (buf.size() < 100) {
		return false;
	}

	// Format: HASH-VERSION-FL-{BODY}

	// check hash
	std::vector<BYTE> bufHash;
	
	if (!Hasher::GetInstance().Hash(
		buf.data() + Hasher::GetInstance().GetHashLength(),
		buf.size() - Hasher::GetInstance().GetHashLength(),
		bufHash
	))
	{
		return false;
	}

	if (std::memcmp(buf.data(), bufHash.data(), bufHash.size()) != 0)
	{
		
		return false;
	}

	try
	{
		yas::mem_istream mi(buf.data() + bufHash.size(), buf.size() - bufHash.size());
		yas::binary_iarchive<yas::mem_istream, yas::binary> ia(mi);

		// vérifier la version
		UINT version;
		ia& version;
		if (version != _VERSION) {
			
			return false;
		}
		ia& desc;
	}
	catch (...) {
		
		desc = {};
		return false;
	}

	//_AddToMemCache(cacheFileName, desc);

	
	return true;
}

void ShaderCache::Save(const wchar_t* fileName, std::string_view hash, const ShaderDesc& desc)
{


	// Format: HASH-VERSION-FL-{BODY}
	std::vector<BYTE> buf;
	buf.reserve(4096);
	buf.resize(Hasher::GetInstance().GetHashLength());
	std::stringstream s;
	try
	{
	  yas::vector_ostream os(buf);
		yas::binary_oarchive<yas::vector_ostream<BYTE>, yas::binary> oa(os);

		oa& _VERSION;
		oa& desc;
	}
	catch (...) {
		
		return;
	}
	
	
	// fill HASH
	std::vector<BYTE> bufHash;
	if (!Hasher::GetInstance().Hash(buf.data() + Hasher::GetInstance().GetHashLength(),
		buf.size() - Hasher::GetInstance().GetHashLength(),
		bufHash
	))
	{
		
		return;
	}
	std::memcpy(buf.data(), bufHash.data(), bufHash.size());


	if (!Utility::DirExists(L".\\cache")) {
		if (!CreateDirectory(L".\\cache", nullptr)) {
			return;
		}
	}
	else {
		// supprimer tous les caches de ce fichier
		std::wregex regex(fmt::format(L"^{}_[0-9,a-f]{{{}}}.{}$", ConvertFileName(fileName),
			Hasher::GetInstance().GetHashLength() * 2, _SUFFIX), std::wregex::optimize | std::wregex::nosubs);

		WIN32_FIND_DATA findData;
		HANDLE hFind = Utility::SafeHandle(FindFirstFile(L".\\cache\\*", &findData));
		if (hFind) {
			while (FindNextFile(hFind, &findData)) {
				if (lstrlenW(findData.cFileName) < 8) {
					continue;
				}

				// Regex matching filename
				if (!std::regex_match(findData.cFileName, regex)) {
					continue;
				}
				
				if (!DeleteFile((L".\\cache\\"s + findData.cFileName).c_str())) {
				}
			}
			FindClose(hFind);
		}
		else {
			
		}
	}

	std::wstring cacheFileName = _GetCacheFileName(fileName, hash);
	if (!Utility::WriteFile(cacheFileName.c_str(), buf.data(), buf.size()))
	{
		std::wstring appdata = _wgetenv(L"APPDATA");

		appdata.append(L"\\MPCVideoRenderer\\Shaders\\cache\\");
		cacheFileName.replace(0, 8, appdata);
		if (!Utility::WriteFile(cacheFileName.c_str(), buf.data(), buf.size()))
			DLog(L"asti");
		else
			DLog(L"asti");
		//not in admin mode just save it it in appdata
	}

	_AddToMemCache(cacheFileName, desc);

	
}
