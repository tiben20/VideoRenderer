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
#include "swscaleProc.h"

CSwscaleProc::CSwscaleProc(HRESULT& hr)
{
	std::wstring path(MAX_PATH, 0);
	DWORD bufsize = MAX_PATH;
	DWORD len = GetModuleFileNameW((HMODULE)&__ImageBase, &path[0], bufsize);
	if (len >= bufsize) {
		DLog(L"CSwscaleProc::CSwscaleProc() : failed to get module path");
		hr = E_FAIL;
		return;
	}
	path.resize(len);
	path.erase(path.find_last_of(L"\\") + 1);

	m_hAvutilLib = LoadLibraryW((path + L"x64\\avutil-56.dll").c_str());
	if (!m_hAvutilLib) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		DLog(L"CSwscaleProc::CSwscaleProc() : failed to load avutil-56.dll with error {}", HR2Str(hr));
		return;
	}

	m_hSwscaleLib = LoadLibraryW((path + L"x64\\swscale-5.dll").c_str());
	if (!m_hSwscaleLib) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		DLog(L"CSwscaleProc::CSwscaleProc() : failed to load swscale-5.dll with error {}", HR2Str(hr));
		return;
	}

	pfn_swscale_version = (PFN_SWSCALE_VERSION)GetProcAddress(m_hSwscaleLib, "swscale_version");
	if (!pfn_swscale_version) {
		DLog(L"CSwscaleProc::CSwscaleProc() : failed to get swscale_version()");
		hr = E_FAIL;
		return;
	}

	const unsigned swscale_ver = pfn_swscale_version();
	if (swscale_ver != LIBSWSCALE_VERSION_INT) {
		DLog(L"CSwscaleProc::CSwscaleProc() : incorrect swscale version");
		hr = E_FAIL;
		return;
	}

	pfn_sws_getCachedContext     = (PFN_SWS_GETCACHEDCONTEXT)GetProcAddress(m_hSwscaleLib, "sws_getCachedContext");
	pfn_sws_freeContext          = (PFN_SWS_FREECONTEXT)GetProcAddress(m_hSwscaleLib, "sws_freeContext");
	pfn_sws_setColorspaceDetails = (PFN_SWS_SETCOLORSPACEDETAILS)GetProcAddress(m_hSwscaleLib, "sws_setColorspaceDetails");
	pfn_sws_getColorspaceDetails = (PFN_SWS_GETCOLORSPACEDETAILS)GetProcAddress(m_hSwscaleLib, "sws_getColorspaceDetails");
	pfn_sws_getCoefficients      = (PFN_SWS_GETCOEFFICIENTS)GetProcAddress(m_hSwscaleLib, "sws_getCoefficients");
	pfn_sws_scale                = (PFN_SWS_SCALE)GetProcAddress(m_hSwscaleLib, "sws_scale");

	if (!pfn_sws_getCachedContext
			|| !pfn_sws_freeContext
			|| !pfn_sws_setColorspaceDetails
			|| !pfn_sws_getColorspaceDetails
			|| !pfn_sws_getCoefficients
			|| !pfn_sws_scale) {
		DLog(L"CSwscaleProc::CSwscaleProc() : failed to get fuctions");
		hr = E_FAIL;
		return;
	}

	hr = S_OK;
}

CSwscaleProc::~CSwscaleProc()
{
	SwscaleFree();

	if (m_hSwscaleLib) {
		FreeLibrary(m_hSwscaleLib);
	}

	if (m_hAvutilLib) {
		FreeLibrary(m_hAvutilLib);
	}
}

void CSwscaleProc::SwscaleFree()
{
	if (m_pSwsContext) {
		pfn_sws_freeContext(m_pSwsContext);
	}
}

AVPixelFormat ColorFormatToAvPixFormat(ColorFormat_t cformat)
{
	AVPixelFormat pixFmt;

	switch (cformat) {
	default:
	case CF_NONE:   pixFmt = AV_PIX_FMT_NONE;     break;
	case CF_YV12:   pixFmt = AV_PIX_FMT_YUV420P;  break;
	case CF_NV12:   pixFmt = AV_PIX_FMT_NV12;     break;
	case CF_P010:   pixFmt = AV_PIX_FMT_P010LE;   break;
	case CF_P016:   pixFmt = AV_PIX_FMT_P016LE;   break;
	case CF_YUY2:   pixFmt = AV_PIX_FMT_YVYU422;  break;
	case CF_YV16:   pixFmt = AV_PIX_FMT_YUV422P;  break;
	case CF_YV24:   pixFmt = AV_PIX_FMT_YUV444P;  break;
	case CF_RGB24:  pixFmt = AV_PIX_FMT_BGR24;    break;
	case CF_XRGB32: pixFmt = AV_PIX_FMT_BGR0;     break;
	case CF_ARGB32: pixFmt = AV_PIX_FMT_BGRA;     break;
	case CF_RGB48:  pixFmt = AV_PIX_FMT_BGR48LE;  break;
	case CF_B48R:   pixFmt = AV_PIX_FMT_RGB48BE;  break;
	case CF_ARGB64: pixFmt = AV_PIX_FMT_BGRA64LE; break;
	case CF_B64A:   pixFmt = AV_PIX_FMT_RGBA64BE; break;
	case CF_Y8:     pixFmt = AV_PIX_FMT_GRAY8;    break;
	case CF_Y800:   pixFmt = AV_PIX_FMT_GRAY8;    break;
	case CF_Y116:   pixFmt = AV_PIX_FMT_GRAY16;   break;
	}

	return pixFmt;
}

HRESULT CSwscaleProc::Configure(const ImageArgs_t& src_args, const ImageArgs_t& dst_args)
{
	SwscaleFree();

	AVPixelFormat src_pixfmt = ColorFormatToAvPixFormat(src_args.cformat);
	AVPixelFormat dst_pixfmt = ColorFormatToAvPixFormat(dst_args.cformat);

	m_pSwsContext = pfn_sws_getCachedContext(
		nullptr,
		src_args.width,
		src_args.height,
		src_pixfmt,
		dst_args.width,
		dst_args.height,
		dst_pixfmt,
		SWS_BICUBLIN | SWS_ACCURATE_RND | SWS_FULL_CHR_H_INP | SWS_PRINT_INFO,
		nullptr,
		nullptr,
		nullptr);

	if (!m_pSwsContext) {
		DLog(L"FCSwscaleProc::Configure() - sws_getCachedContext() failed");
		return E_FAIL;
	}

	int *inv_tbl = nullptr, *tbl = nullptr;
	int srcRange, dstRange, brightness, contrast, saturation;

	int ret = pfn_sws_getColorspaceDetails(m_pSwsContext, &inv_tbl, &srcRange, &tbl, &dstRange, &brightness, &contrast, &saturation);
	if (ret >= 0) {
		const auto& src_FmtParams = GetFmtConvParams(src_args.cformat);
		const auto& dst_FmtParams = GetFmtConvParams(dst_args.cformat);

		if (src_FmtParams.CSType == CS_RGB || src_args.exfmt.NominalRange == DXVA2_NominalRange_0_255) {
			srcRange = 1;
		} else {
			srcRange = 0;
		}

		if (dst_FmtParams.CSType == CS_RGB || dst_args.exfmt.NominalRange == DXVA2_NominalRange_0_255) {
			dstRange = 1;
		} else {
			dstRange = 0;
		}

		if (src_FmtParams.CSType == CS_YUV && dst_FmtParams.CSType == CS_RGB) {
			AVColorSpace colorspace = {};

			switch (src_args.exfmt.VideoTransferMatrix) {
			case DXVA2_VideoTransferMatrix_BT709:     colorspace = AVCOL_SPC_BT709;      break;
			case DXVA2_VideoTransferMatrix_BT601:     colorspace = AVCOL_SPC_BT470BG;    break;
			case DXVA2_VideoTransferMatrix_SMPTE240M: colorspace = AVCOL_SPC_SMPTE240M;  break;
			case VIDEOTRANSFERMATRIX_BT2020_10:
			case VIDEOTRANSFERMATRIX_BT2020_12:       colorspace = AVCOL_SPC_BT2020_NCL; break;
			case VIDEOTRANSFERMATRIX_FCC:             colorspace = AVCOL_SPC_FCC;        break;
			case VIDEOTRANSFERMATRIX_YCgCo:           colorspace = AVCOL_SPC_YCGCO;      break;
			}

			inv_tbl = (int*)pfn_sws_getCoefficients(colorspace);
		}

		ret = pfn_sws_setColorspaceDetails(m_pSwsContext, inv_tbl, srcRange, tbl, dstRange, brightness, contrast, saturation);
	}

	m_src_cfmt   = src_args.cformat;
	m_src_height = src_args.height;

	return (ret >= 0) ? S_OK : E_FAIL;
}

HRESULT CSwscaleProc::Process(const void * const src_p[3], const unsigned src_stride[3], void* dst_p, const unsigned dst_stride)
{
	if (!m_pSwsContext) {
		return E_ABORT;
	}

	const uint8_t* srcSlice[4] = { (const uint8_t*)src_p[0], (const uint8_t*)src_p[1], (const uint8_t*)src_p[2] };

	if (m_src_cfmt == CF_YV12 || m_src_cfmt == CF_YV16 || m_src_cfmt == CF_YV24) {
		std::swap(srcSlice[1], srcSlice[2]);
	}

	uint8_t *const dst[4] = { (uint8_t*)dst_p };

	int srcStride[4] = { src_stride[0], src_stride[1], src_stride[2]};
	int dstStride[4] = { dst_stride };

	int ret = pfn_sws_scale(m_pSwsContext, srcSlice, srcStride, 0, m_src_height, dst, dstStride);

	return (ret >= 0) ? S_OK : E_FAIL;
}
