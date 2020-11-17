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

#include "Helper.h"
#include "libswscale/swscale.h"

struct ImageArgs2_t {
	ColorFormat_t cformat;
	unsigned width;
	unsigned height;
	DXVA2_ExtendedFormat exfmt;
};

class CSwscaleProc
{
private:
	HMODULE m_hAvutilLib = nullptr;
	HMODULE m_hSwscaleLib = nullptr;

	typedef unsigned(__stdcall* PFN_SWSCALE_VERSION)(void);

	typedef struct SwsContext *(__stdcall* PFN_SWS_GETCACHEDCONTEXT)(
		struct SwsContext *context,
		int srcW, int srcH, enum AVPixelFormat srcFormat,
		int dstW, int dstH, enum AVPixelFormat dstFormat,
		int flags, SwsFilter *srcFilter,
		SwsFilter *dstFilter, const double *param);

	typedef void (__stdcall* PFN_SWS_FREECONTEXT)(struct SwsContext *swsContext);

	typedef int (__stdcall* PFN_SWS_SETCOLORSPACEDETAILS)(
		struct SwsContext *c, const int inv_table[4],
		int srcRange, const int table[4], int dstRange,
		int brightness, int contrast, int saturation);

	typedef int (__stdcall* PFN_SWS_GETCOLORSPACEDETAILS)(
		struct SwsContext *c, int **inv_table,
		int *srcRange, int **table, int *dstRange,
		int *brightness, int *contrast, int *saturation);

	typedef const int* (__stdcall* PFN_SWS_GETCOEFFICIENTS)(int colorspace);

	typedef int (__stdcall* PFN_SWS_SCALE)(
		struct SwsContext *c, const uint8_t *const srcSlice[],
		const int srcStride[], int srcSliceY, int srcSliceH,
		uint8_t *const dst[], const int dstStride[]);

	PFN_SWSCALE_VERSION          pfn_swscale_version          = nullptr;
	PFN_SWS_GETCACHEDCONTEXT     pfn_sws_getCachedContext     = nullptr;
	PFN_SWS_FREECONTEXT          pfn_sws_freeContext          = nullptr;
	PFN_SWS_SETCOLORSPACEDETAILS pfn_sws_setColorspaceDetails = nullptr;
	PFN_SWS_GETCOLORSPACEDETAILS pfn_sws_getColorspaceDetails = nullptr;
	PFN_SWS_GETCOEFFICIENTS      pfn_sws_getCoefficients      = nullptr;
	PFN_SWS_SCALE                pfn_sws_scale                = nullptr;

	SwsContext* m_pSwsContext = nullptr;
	ColorFormat_t m_src_cfmt = CF_NONE;
	int m_src_height = 0;

public:
	CSwscaleProc(HRESULT& hr);
	~CSwscaleProc();

private:
	void SwscaleFree();

public:
	HRESULT Configure(const ImageArgs2_t& src_args, const ImageArgs2_t& dst_args);

	HRESULT Process(const void * const src_p[3], const unsigned src_stride[3], void* dst_p, const unsigned dst_stride);
};
