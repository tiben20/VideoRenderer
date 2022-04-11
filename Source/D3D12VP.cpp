/*
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

#include "stdafx.h"
#include <cmath>
#include "Helper.h"
#include "DX12Engine.h"
#include "D3D12VP.h"
#include "DX12Capabilities.h"
#define ENABLE_FUTUREFRAMES 0
#include "texture.h"


// CD3D12VP

HRESULT CD3D12VP::InitVideoDevice(ID3D12Device *pDevice)
{

	HRESULT hr = pDevice->QueryInterface(IID_PPV_ARGS(&m_pVideoDevice));
	if (FAILED(hr)) {
		DLog(L"CD3D11VP::InitVideoDevice() : QueryInterface(ID3D11VideoDevice) failed with error {}", HR2Str(hr));
		ReleaseVideoDevice();
		return hr;
	}
	
}

void CD3D12VP::ReleaseVideoDevice()
{
	ReleaseVideoProcessor();

	m_pVideoDevice.Release();
}

HRESULT CD3D12VP::InitVideoProcessor(const DXGI_FORMAT inputFmt, const UINT width, const UINT height, const bool interlaced, DXGI_FORMAT& outputFmt)
{
	ReleaseVideoProcessor();
	HRESULT hr = S_OK;
	m_pInputFormat = inputFmt;
	m_pOutputFormat = outputFmt;
	bool inputfmtok = false;
	bool outputfmtok = false;
	m_srcRect.left = 0; m_srcRect.top = 0;
	m_srcRect.right = width;
	m_srcRect.bottom = height;
	//D3D12Engine::g_D3D12Capabilities
	std::map<DXGI_FORMAT, VideoCapabilities> caps = D3D12Engine::GetCapabilities();
	for (std::map<DXGI_FORMAT, VideoCapabilities>::iterator it = caps.begin(); it != caps.end(); it++)
	{
		if (it->first == inputFmt)
		{
			VideoCapabilities caps = it->second;
			if (caps.Input == true)
				inputfmtok = true;
		}
		if (it->first == outputFmt)
		{
			VideoCapabilities caps = it->second;
			if (caps.Output == true)
				outputfmtok = true;
		}
	}
	if (!inputfmtok || !outputfmtok)
	{
		DLog(L"CD3D11VP::InitVideoDevice() : QueryInterface(ID3D11VideoDevice) failed with error {}", HR2Str(hr));
		ReleaseVideoDevice();
		return E_FAIL;
	}
	D3D12_VIDEO_PROCESS_OUTPUT_STREAM_DESC pOutputStreamDesc;
	D3D12_VIDEO_PROCESS_INPUT_STREAM_DESC pInputStreamDescs;
	pInputStreamDescs = {};
	pInputStreamDescs.Format = inputFmt;
	pInputStreamDescs.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
	pInputStreamDescs.SourceAspectRatio = DXGI_RATIONAL({ width, height });
	pInputStreamDescs.DestinationAspectRatio = DXGI_RATIONAL({ width, height });
	pInputStreamDescs.FrameRate = DXGI_RATIONAL({ 24, 1 });
	pInputStreamDescs.SourceSizeRange = D3D12_VIDEO_SIZE_RANGE({ width,height,(width/2),(height/2)});
	pInputStreamDescs.DestinationSizeRange = D3D12_VIDEO_SIZE_RANGE({ width,height,(width / 2),(height / 2) });
	pInputStreamDescs.EnableOrientation = false;
	pInputStreamDescs.FilterFlags = D3D12_VIDEO_PROCESS_FILTER_FLAG_NONE;
	pInputStreamDescs.StereoFormat = D3D12_VIDEO_FRAME_STEREO_FORMAT_NONE;
	pInputStreamDescs.FieldType = D3D12_VIDEO_FIELD_TYPE_NONE;
	pInputStreamDescs.DeinterlaceMode = D3D12_VIDEO_PROCESS_DEINTERLACE_FLAG_NONE;
	pInputStreamDescs.EnableAlphaBlending = 0;
	pInputStreamDescs.LumaKey = D3D12_VIDEO_PROCESS_LUMA_KEY({ 0,0,0 });
	pInputStreamDescs.NumPastFrames = 0;
	pInputStreamDescs.NumFutureFrames = 0;
	pInputStreamDescs.EnableAutoProcessing = 0;
	pOutputStreamDesc.Format = outputFmt;
	pOutputStreamDesc.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
	pOutputStreamDesc.AlphaFillMode = D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE_OPAQUE;
	pOutputStreamDesc.AlphaFillModeSourceStreamIndex = 0;
	pOutputStreamDesc.BackgroundColor[0] = 1.0f;
	pOutputStreamDesc.BackgroundColor[1] = 1.0f;
	pOutputStreamDesc.BackgroundColor[2] = 1.0f;
	pOutputStreamDesc.BackgroundColor[3] = 1.0f;
	pOutputStreamDesc.FrameRate = DXGI_RATIONAL({ 24, 1 });
	pOutputStreamDesc.EnableStereo = 0;
	hr = m_pVideoDevice->CreateVideoProcessor(0, &pOutputStreamDesc, 1, &pInputStreamDescs, IID_PPV_ARGS(&m_pVideoProcessor));
	D3D12_FEATURE_DATA_VIDEO_PROCESS_REFERENCE_INFO formatInfo = {};
	hr = m_pVideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_PROCESS_REFERENCE_INFO, &formatInfo, sizeof(D3D12_FEATURE_DATA_VIDEO_PROCESS_REFERENCE_INFO));

	//HRESULT hr = pDevice->QueryInterface(IID_PPV_ARGS(&m_pVideoDevice));
	if (FAILED(hr)) {
		DLog(L"CD3D12VP::InitVideoDevice() : QueryInterface(ID3D12VideoDevice) failed with error {}", HR2Str(hr));
		ReleaseVideoDevice();
		return hr;
	}

	return hr;

}

void CD3D12VP::ReleaseVideoProcessor()
{
	//clearing video textures
	m_pVideoProcessor.Release();
}

HRESULT CD3D12VP::InitInputTextures()
{
	
	HRESULT hr = E_FAIL;
	m_pInputTextures.CreateVideoTexture(m_srcRect.Width(), m_srcRect.Height(), m_pInputFormat);
	m_pOutputTextures.CreateVideoTexture(m_srcRect.Width(), m_srcRect.Height(), m_pInputFormat);
	return hr;
}

HRESULT CD3D12VP::Process(ID3D12Resource* target, ID3D12Resource* sub)
{
	VideoProcessorContext& pVideoContext = VideoProcessorContext::Begin(L"Create texture transit");
	
	
	//D3D12_VIDEO_PROCESS_INPUT_STREAM instream = {};
	//D3D12_VIDEO_PROCESS_OUTPUT_STREAM outstream = {};
	D3D12_VIDEO_PROCESS_INPUT_STREAM_ARGUMENTS inargs = {};
	D3D12_VIDEO_PROCESS_OUTPUT_STREAM_ARGUMENTS outargs = {};
	inargs.AlphaBlending.Enable = false;
	inargs.Transform.SourceRectangle = m_srcRect;
	inargs.Transform.DestinationRectangle = m_srcRect;
	inargs.RateInfo.InputFrameOrField = 0;
	inargs.RateInfo.OutputIndex = 0;
	inargs.Transform.Orientation = D3D12_VIDEO_PROCESS_ORIENTATION_DEFAULT;
	inargs.FilterLevels[0] = D3D12_VIDEO_PROCESS_FILTER_FLAG_NONE;
	inargs.Flags = D3D12_VIDEO_PROCESS_INPUT_STREAM_FLAG_NONE;
	inargs.InputStream->pTexture2D = m_pInputTextures.GetResource();
	outargs.OutputStream->pTexture2D = m_pOutputTextures.GetResource();
	//outargs.OutputStream[0].pTexture2D = test.GetResource();
	//instream.pTexture2D = target;
	D3D12_VIDEO_PROCESS_REFERENCE_SET ref = {};
	//instream.ReferenceSet = ref;
	//outstream.
	pVideoContext.ProcessFrames(m_pVideoProcessor, &outargs, 1, &inargs);
	pVideoContext.FinishVideo();
	return E_NOTIMPL;
}
