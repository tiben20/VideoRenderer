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

#include "stdafx.h"
#include "Utils\Util.h"
#include "Utils\StringUtil.h"
#include "ZimgProc.h"

// https://github.com/sekrit-twc/zimg/tree/master/doc/example

CZimgProc::CZimgProc(HRESULT& hr)
{
	std::wstring path(MAX_PATH, 0);
	DWORD bufsize = MAX_PATH;
	DWORD len = GetModuleFileNameW((HMODULE)&__ImageBase, &path[0], bufsize);
	if (len >= bufsize) {
		DLog(L"CZimgProc::CZimgProc() : failed to get module path");
		hr = E_FAIL;
		return;
	}
	path.resize(len);
	path.erase(path.find_last_of(L"\\") + 1);

	m_hZimgLib = LoadLibraryW((path + L"x64\\zimg.dll").c_str());
	if (!m_hZimgLib) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		DLog(L"CZimgProc::CZimgProc() : failed to load Zimg.dll with error {}", HR2Str(hr));
		return;
	}

	pfn_zimg_get_api_version = (PFN_ZIMG_GET_API_VERSION)GetProcAddress(m_hZimgLib, "zimg_get_api_version");
	if (!pfn_zimg_get_api_version) {
		DLog(L"CZimgProc::CZimgProc() : failed to get zimg_get_api_version()");
		hr = E_FAIL;
		return;
	}

	const unsigned zimg_api_ver = pfn_zimg_get_api_version(nullptr, nullptr);
	if (ZIMG_API_VERSION != zimg_api_ver) {
		DLog(L"CZimgProc::CZimgProc() : incorrect zimg api version");
		hr = E_FAIL;
		return;
	}

	pfn_zimg_image_format_default      = (PFN_ZIMG_IMAGE_FORMAT_DEFAULT)GetProcAddress(m_hZimgLib, "zimg_image_format_default");
	pfn_zimg_filter_graph_build        = (PFN_ZIMG_FILTER_GRAPH_BUILD)GetProcAddress(m_hZimgLib, "zimg_filter_graph_build");
	pfn_zimg_filter_graph_get_tmp_size = (PFN_ZIMG_FILTER_GRAPH_GET_TMP_SIZE)GetProcAddress(m_hZimgLib, "zimg_filter_graph_get_tmp_size");
	pfn_zimg_filter_graph_process      = (PFN_ZIMG_FILTER_GRAPH_PROCESS)GetProcAddress(m_hZimgLib, "zimg_filter_graph_process");
	pfn_zimg_filter_graph_free         = (PFN_ZIMG_FILTER_GRAPH_FREE)GetProcAddress(m_hZimgLib, "zimg_filter_graph_free");
	pfn_zimg_get_last_error            = (PFN_ZIMG_GET_LAST_ERROR)GetProcAddress(m_hZimgLib, "zimg_get_last_error");

	if (!pfn_zimg_image_format_default
			|| !pfn_zimg_filter_graph_build
			|| !pfn_zimg_filter_graph_get_tmp_size
			|| !pfn_zimg_filter_graph_process
			|| !pfn_zimg_filter_graph_free
			|| !pfn_zimg_get_last_error) {
		DLog(L"CZimgProc::CZimgProc() : failed to get fuctions");
		hr = E_FAIL;
		return;
	}

	hr = S_OK;
}

CZimgProc::~CZimgProc()
{
	ZimgFree();

	if (m_hZimgLib) {
		FreeLibrary(m_hZimgLib);
	}
}

void CZimgProc::ZimgFree()
{
	if (pfn_zimg_filter_graph_free) {
		pfn_zimg_filter_graph_free(m_filter_graph);
		m_filter_graph = nullptr;
	}
	_aligned_free(m_tmp);
	m_tmp = nullptr;
}

void CZimgProc::ZimgLogError()
{
	int err_code = pfn_zimg_get_last_error(m_err_msg, sizeof(m_err_msg));

	DLog(L"zimg error {} {}", err_code, A2WStr(m_err_msg));
}

struct ZimgFormatInfo_t {
	ColorFormat_t     cformat;
	zimg_pixel_type_e pixel_type;
	unsigned subsample_w;
	unsigned subsample_h;
	zimg_color_family_e color_family;
	unsigned depth;
};

void CZimgProc::ZimgSetImageFormat(const ImageArgs_t& args, zimg_image_format& format)
{
	pfn_zimg_image_format_default(&format, ZIMG_API_VERSION);

	format.width = args.width;
	format.height = args.height;

	const auto& FmtParams = GetFmtConvParams(args.cformat);

	if (FmtParams.CDepth == 8) {
		format.pixel_type = ZIMG_PIXEL_BYTE;
		format.depth = 8;
	}
	else if (FmtParams.CDepth == 10) {
		format.pixel_type = ZIMG_PIXEL_WORD;
		format.depth = 10;
	}
	else if (FmtParams.CDepth == 16) {
		format.pixel_type = ZIMG_PIXEL_WORD;
		format.depth = 16;
	}

	if (FmtParams.Subsampling == 420) {
		format.subsample_w = 1;
		format.subsample_h = 1;
	}
	else if (FmtParams.Subsampling == 422) {
		format.subsample_w = 1;
		format.subsample_h = 0;
	}

	if (FmtParams.CSType == CS_YUV) {
		format.color_family = ZIMG_COLOR_YUV;
	}
	else if (FmtParams.CSType == CS_RGB) {
		format.color_family = ZIMG_COLOR_RGB;
	}
	else if (FmtParams.CSType == CS_GRAY) {
		format.color_family = ZIMG_COLOR_GREY;
	}

	if (format.color_family == ZIMG_COLOR_RGB) {
		format.pixel_range = ZIMG_RANGE_FULL;
		format.matrix_coefficients = ZIMG_MATRIX_RGB;
		format.transfer_characteristics = ZIMG_TRANSFER_BT709;
		format.color_primaries = ZIMG_PRIMARIES_BT709;
	}
	else if (format.color_family == ZIMG_COLOR_YUV) {
		switch (args.exfmt.NominalRange) {
		case DXVA2_NominalRange_0_255:  format.pixel_range = ZIMG_RANGE_FULL;    break;
		case DXVA2_NominalRange_16_235: format.pixel_range = ZIMG_RANGE_LIMITED; break;
		}

		switch (args.exfmt.VideoTransferMatrix) {
		case DXVA2_VideoTransferMatrix_BT709:     format.matrix_coefficients = ZIMG_MATRIX_BT709;      break;
		case DXVA2_VideoTransferMatrix_BT601:     format.matrix_coefficients = ZIMG_MATRIX_BT470_BG;   break;
		case DXVA2_VideoTransferMatrix_SMPTE240M: format.matrix_coefficients = ZIMG_MATRIX_ST240_M;    break;
		case VIDEOTRANSFERMATRIX_BT2020_10:
		case VIDEOTRANSFERMATRIX_BT2020_12:       format.matrix_coefficients = ZIMG_MATRIX_BT2020_NCL; break;
		case VIDEOTRANSFERMATRIX_FCC:             format.matrix_coefficients = ZIMG_MATRIX_FCC;        break;
		case VIDEOTRANSFERMATRIX_YCgCo:           format.matrix_coefficients = ZIMG_MATRIX_YCGCO;      break;
		}

		switch (args.exfmt.VideoPrimaries) {
		case DXVA2_VideoPrimaries_BT709:         format.color_primaries = ZIMG_PRIMARIES_BT709;     break;
		case DXVA2_VideoPrimaries_BT470_2_SysM:  format.color_primaries = ZIMG_PRIMARIES_BT470_M;   break;
		case DXVA2_VideoPrimaries_BT470_2_SysBG: format.color_primaries = ZIMG_PRIMARIES_BT470_BG;  break;
		case DXVA2_VideoPrimaries_SMPTE170M:     format.color_primaries = ZIMG_PRIMARIES_ST170_M;   break;
		case DXVA2_VideoPrimaries_SMPTE240M:     format.color_primaries = ZIMG_PRIMARIES_ST240_M;   break;
		case DXVA2_VideoPrimaries_EBU3213:       format.color_primaries = ZIMG_PRIMARIES_EBU3213_E; break;
		case DXVA2_VideoPrimaries_SMPTE_C:       format.color_primaries = ZIMG_PRIMARIES_ST240_M;   break;
		case VIDEOPRIMARIES_BT2020:              format.color_primaries = ZIMG_PRIMARIES_BT2020;    break;
		case VIDEOPRIMARIES_DCI_P3:              format.color_primaries = ZIMG_PRIMARIES_ST431_2;   break;
		}

		switch (args.exfmt.VideoTransferFunction) {
		case DXVA2_VideoTransFunc_10:   format.transfer_characteristics = ZIMG_TRANSFER_LINEAR;    break;
		case DXVA2_VideoTransFunc_709:  format.transfer_characteristics = ZIMG_TRANSFER_BT709;     break;
		case DXVA2_VideoTransFunc_240M: format.transfer_characteristics = ZIMG_TRANSFER_BT601;     break;
		case VIDEOTRANSFUNC_Log_100:    format.transfer_characteristics = ZIMG_TRANSFER_LOG_100;   break;
		case VIDEOTRANSFUNC_Log_316:    format.transfer_characteristics = ZIMG_TRANSFER_LOG_316;   break;
		case VIDEOTRANSFUNC_2020_const: format.transfer_characteristics = ZIMG_TRANSFER_BT2020_10; break;
		case VIDEOTRANSFUNC_2020:       format.transfer_characteristics = ZIMG_TRANSFER_BT2020_10; break;
		case VIDEOTRANSFUNC_2084:       format.transfer_characteristics = ZIMG_TRANSFER_ST2084;    break;
		case VIDEOTRANSFUNC_HLG:        format.transfer_characteristics = ZIMG_TRANSFER_ARIB_B67;  break;
		}
	}
}

HRESULT CZimgProc::Configure(const ImageArgs_t& src_args, const ImageArgs_t& dst_args)
{
	ZimgFree();

	if ((src_args.cformat != CF_YV12 && src_args.cformat != CF_YV16 && src_args.cformat != CF_YV24) || dst_args.cformat != CF_XRGB32) {
		return E_ABORT;
	}

	ZimgSetImageFormat(src_args, m_src_format);
	ZimgSetImageFormat(dst_args, m_dst_format);

	m_filter_graph = pfn_zimg_filter_graph_build(&m_src_format, &m_dst_format, nullptr);
	if (!m_filter_graph) {
		ZimgLogError();
		ZimgFree();
		return E_FAIL;
	}

	size_t tmp_size = 0;
	zimg_error_code_e ret = pfn_zimg_filter_graph_get_tmp_size(m_filter_graph, &tmp_size);
	if (ret) {
		ZimgLogError();
		ZimgFree();
		return E_FAIL;
	}

	m_tmp = _aligned_malloc(tmp_size, 32);
	if (!m_tmp) {
		ZimgLogError();
		ZimgFree();
		return E_OUTOFMEMORY;
	}

	m_src_cfmt   = src_args.cformat;
	m_dst_cfmt   = dst_args.cformat;

	return S_OK;
}

HRESULT CZimgProc::Process(const void * const src_p[3], const unsigned src_stride[3], void* dst_p, const unsigned dst_stride)
{
	if (!m_filter_graph) {
		return E_ABORT;
	}

	zimg_image_buffer_const src_buf = { ZIMG_API_VERSION };
	for (unsigned i = 0; i < 3; ++i) {
		src_buf.plane[i].data = src_p[i];
		src_buf.plane[i].stride = src_stride[i];
		src_buf.plane[i].mask = ZIMG_BUFFER_MAX;
	}

	const unsigned temp_stride = ALIGN(m_dst_format.width, 32);
	const unsigned temp_size = temp_stride * m_dst_format.height * 3;

	void* const temp_p = _aligned_malloc(temp_size, 32);
	if (!temp_p) {
		_aligned_free(temp_p);
		return E_OUTOFMEMORY;
	}

	zimg_image_buffer temp_buf = { ZIMG_API_VERSION };
	temp_buf.plane[0].data = temp_p;
	temp_buf.plane[0].stride = temp_stride;
	temp_buf.plane[0].mask = ZIMG_BUFFER_MAX;
	temp_buf.plane[1].data = (uint8_t*)temp_buf.plane[0].data + temp_stride * m_dst_format.height;
	temp_buf.plane[1].stride = temp_stride;
	temp_buf.plane[1].mask = ZIMG_BUFFER_MAX;
	temp_buf.plane[2].data = (uint8_t*)temp_buf.plane[1].data + temp_stride * m_dst_format.height;
	temp_buf.plane[2].stride = temp_stride;
	temp_buf.plane[2].mask = ZIMG_BUFFER_MAX;

	zimg_error_code_e ret = pfn_zimg_filter_graph_process(m_filter_graph, &src_buf, &temp_buf, m_tmp, 0, 0, 0, 0);
	if (ret) {
		ZimgLogError();
		_aligned_free(temp_p);
		return E_FAIL;
	}

	const uint8_t* planar_r = static_cast<const uint8_t*>(temp_buf.plane[0].data);
	const uint8_t* planar_g = static_cast<const uint8_t*>(temp_buf.plane[1].data);
	const uint8_t* planar_b = static_cast<const uint8_t*>(temp_buf.plane[2].data);
	uint32_t* packed_xbgr = static_cast<uint32_t*>(dst_p);
	unsigned packed_pitch = dst_stride / 4;

	for (unsigned y = 0; y < m_dst_format.height; ++y) {
		for (unsigned x = 0; x < m_dst_format.width; ++x) {
			uint32_t pixel = planar_r[x] + (planar_g[x] << 8) + (planar_b[x] << 16) + 0xFF000000;
			packed_xbgr[x] = pixel;
		}
		planar_r += temp_stride;
		planar_g += temp_stride;
		planar_b += temp_stride;
		packed_xbgr += packed_pitch;
	}

	_aligned_free(temp_p);

	return S_OK;
}
