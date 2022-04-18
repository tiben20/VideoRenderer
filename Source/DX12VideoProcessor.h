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

#include <DXGI1_2.h>
#include <dxva2api.h>
#include <dxgi1_5.h>
#include <strmif.h>
#include <map>
#include "IVideoRenderer.h"
#include "DX12Engine.h"
#include "D3D12VP.h"
#include "DX9Device.h"
#include "VideoProcessor.h"
#include "D3D12Thread.h"

#include "GeometryRenderer.h"

#define TEST_SHADER 0



class CVideoRendererInputPin;
class CD3D12InputQueue;


class CDX12VideoProcessor
	: public CVideoProcessor
	, public CDX9Device
{
private:
	friend class CVideoRendererInputPin;

	/*Threading*/
	void GetNextRessource(ColorBuffer** resource) {};

	/*end threading*/
	void GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

	//video prop
	D3D12_VIDEO_FIELD_TYPE m_SampleFormat;

	/*d3d9 subpic*/
	CComPtr<IDirect3DSurface9>        m_pSurface9SubPic;
	ColorBuffer                       m_pTextureSubPic;

	bool                              m_bSubPicWasRendered = false;

	DXGI_SWAP_EFFECT              m_UsedSwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	// D3D12 Video Processor
	CD3D12VP m_D3D12VP;
	const wchar_t* m_strCorrection = nullptr;
	uint64_t m_iStartTick;

	//ByteAddressBuffer m_pVideoQuadVertex;
	//ByteAddressBuffer m_pVideoIndexBuffer;
	UploadBuffer m_pVertexBuffer;
	UploadBuffer m_pIndexBuffer;
	UploadBuffer m_pViewpointShaderConstant;
	UploadBuffer m_pPixelShaderConstants;
	Texture      m_pAlphaBitmapTexture;
	Texture m_pTexturePlane1;
	Texture m_pTexturePlane2;
	CD3D12InputQueue* m_pInputQueue;

	CONSTANT_BUFFER_VAR m_pBufferVar;
	bool m_PSConvColorData = false;
	D3D12_VERTEX_BUFFER_VIEW m_pVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_pIndexBufferView;
	bool m_bSWRendering = false;
	std::wstring m_pCurrentRenderTiming;
	int m_pStatsDelay = 0;
	std::wstring m_sScalerX = L"";
	std::wstring m_sScalerY = L"";
	
	bool resetquad = false;

	
	D3DCOLOR m_dwStatsTextColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	

	//Graph rendering
	GraphRectangle m_StatsBackground;
	GraphRectangle m_Rect3D;
	GraphRectangle m_Underlay;
	std::vector<GraphLine> m_Lines;

public:
	CDX12VideoProcessor(CMpcVideoRenderer* pFilter, const Settings_t& config, HRESULT& hr);
	~CDX12VideoProcessor() override;

//CVideoProcessor
	int Type() override { return VP_DX12; }
	HRESULT Init(const HWND hwnd, bool* pChangeDevice = nullptr) override;
	HRESULT Render(int field) override;
	HRESULT AddPreScaleShader(const std::wstring& name, const std::string& srcCode) override;
	HRESULT AddPostScaleShader(const std::wstring& name, const std::string& srcCode) override;

	BOOL VerifyMediaType(const CMediaType* pmt) override;
	BOOL InitMediaType(const CMediaType* pmt) override;

	void Configure(const Settings_t& config) override;
	void SetRotation(int value) override;
	void Flush() override;
	void ClearPreScaleShaders() override;
	void ClearPostScaleShaders() override;

	HRESULT RenderSubpics(ColorBuffer input, GraphicsContext& Context);
	
	bool Initialized();

	HRESULT InitializeTexVP(const FmtConvParams_t& params, const UINT width, const UINT height);
	void SetShaderConvertColorParams(DXVA2_ExtendedFormat srcExFmt, FmtConvParams_t m_srcParams, DXVA2_ProcAmpValues m_DXVA2ProcAmpValues);
	
	void UpdateFrameProperties() {
		m_srcPitch = m_srcWidth * m_srcParams.Packsize;
		m_srcLines = m_srcHeight * m_srcParams.PitchCoeff / 2;
	}
	
private:
	void ReleaseDevice();
	void ReleaseVP();

	bool HandleHDRToggle();
	bool m_bForceD3D12 = false;
	struct HDRMetadata {
		DXGI_HDR_METADATA_HDR10 hdr10 = {};
		bool bValid = false;
	};
	HDRMetadata m_hdr10 = {};
	HDRMetadata m_lastHdr10 = {};
	bool m_bCallbackDeviceIsSet = false;

	void SetCallbackDevice(const bool bChangeDevice = false);
//CVideoProcessor
	void SetGraphSize() override;
	BOOL GetAlignmentSize(const CMediaType& mt, SIZE& Size) override;
	HRESULT Process(GraphicsContext& Context, const CRect& srcRect, const CRect& dstRect, const bool second);
	HRESULT ProcessSample(IMediaSample* pSample) override;
	HRESULT CopySample(IMediaSample* pSample);

	/*Drawing graphs*/
	void DrawStats(GraphicsContext& Context, float x, float y, float w, float h);

	HRESULT FillBlack() override;
	void UpdateRenderRect();

	void SetVideoRect(const CRect& videoRect)      override;
	HRESULT SetWindowRect(const CRect& windowRect) override;
	HRESULT Reset() override;
	bool IsInit() const override {
		//assert(0);
		return true;
		//return m_bHdrDisplaySwitching; 
	}

	IDirect3DDeviceManager9* GetDeviceManager9() override { return GetDevMan9(); }
	HRESULT GetCurentImage(long* pDIBImage) override;
	HRESULT GetDisplayedImage(BYTE** ppDib, unsigned* pSize) override;
	HRESULT GetVPInfo(std::wstring& str) override;

	void UpdateStatsPresent();
	void UpdateStatsStatic();
	
	HRESULT MemCopyToTexSrcVideo(const BYTE* srcData, const int srcPitch, size_t bufferlength);

	HRESULT CreateDevice();
	void CreateSubPicSurface();
public:
	
	HRESULT SetDevice(ID3D12Device* pDevice, const bool bDecoderDevice);
	void UpdateDisplayInfo(const DisplayConfig_t& dc);

	// IMFVideoProcessor
	STDMETHODIMP SetProcAmpValues(DWORD dwFlags, DXVA2_ProcAmpValues* pValues) override;

	// IMFVideoMixerBitmap
	STDMETHODIMP SetAlphaBitmap(const MFVideoAlphaBitmap* pBmpParms) override;
	STDMETHODIMP UpdateAlphaBitmapParameters(const MFVideoAlphaBitmapParams* pBmpParms) override;
};
