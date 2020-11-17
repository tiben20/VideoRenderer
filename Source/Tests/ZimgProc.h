/*
* (C) 2020 see Authors.txt
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

#include "test.h"
#include "zimg/zimg.h"

class CZimgProc
{
private:
	HMODULE m_hZimgLib = nullptr;

	typedef unsigned(__stdcall* PFN_ZIMG_GET_API_VERSION)(
		unsigned* major,
		unsigned* minor);

	typedef void(__stdcall* PFN_ZIMG_IMAGE_FORMAT_DEFAULT)(
		zimg_image_format* ptr,
		unsigned version);

	typedef zimg_filter_graph*(__stdcall* PFN_ZIMG_FILTER_GRAPH_BUILD)(
		const zimg_image_format* src_format,
		const zimg_image_format* dst_format,
		const zimg_graph_builder_params* params);

	typedef zimg_error_code_e(__stdcall* PFN_ZIMG_FILTER_GRAPH_GET_TMP_SIZE)(
		const zimg_filter_graph* ptr,
		size_t* out);

	typedef zimg_error_code_e(__stdcall* PFN_ZIMG_FILTER_GRAPH_PROCESS)(
		const zimg_filter_graph* ptr, const zimg_image_buffer_const* src,
		const zimg_image_buffer* dst, void* tmp,
		zimg_filter_graph_callback unpack_cb, void* unpack_user,
		zimg_filter_graph_callback pack_cb, void* pack_user);

	typedef void(__stdcall* PFN_ZIMG_FILTER_GRAPH_FREE)(zimg_filter_graph* ptr);

	typedef zimg_error_code_e(__stdcall* PFN_ZIMG_GET_LAST_ERROR)(char* err_msg, size_t n);

	PFN_ZIMG_GET_API_VERSION           pfn_zimg_get_api_version           = nullptr;
	PFN_ZIMG_IMAGE_FORMAT_DEFAULT      pfn_zimg_image_format_default      = nullptr;
	PFN_ZIMG_FILTER_GRAPH_BUILD        pfn_zimg_filter_graph_build        = nullptr;
	PFN_ZIMG_FILTER_GRAPH_GET_TMP_SIZE pfn_zimg_filter_graph_get_tmp_size = nullptr;
	PFN_ZIMG_FILTER_GRAPH_PROCESS      pfn_zimg_filter_graph_process      = nullptr;
	PFN_ZIMG_FILTER_GRAPH_FREE         pfn_zimg_filter_graph_free         = nullptr;
	PFN_ZIMG_GET_LAST_ERROR            pfn_zimg_get_last_error            = nullptr;

	zimg_filter_graph* m_filter_graph = nullptr;
	void* m_tmp = nullptr;
	char m_err_msg[1024];

	ColorFormat_t m_src_cfmt = CF_NONE;
	zimg_image_format m_src_format = {};

	ColorFormat_t m_dst_cfmt = CF_NONE;
	zimg_image_format m_dst_format = {};

	void ZimgFree();
	void ZimgLogError();
	void ZimgSetImageFormat(const ImageArgs_t& args, zimg_image_format& format);

public:
	CZimgProc(HRESULT& hr);
	~CZimgProc();

	HRESULT Configure(const ImageArgs_t& src_args, const ImageArgs_t& dst_args);

	HRESULT Process(const void * const src_p[3], const unsigned src_stride[3], void* dst_p, const unsigned dst_stride);
};
