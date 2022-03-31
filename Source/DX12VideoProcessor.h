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
#include "dx12helper.h"
#include "D3D12VP.h"
#include "DX9Device.h"
#include "VideoProcessor.h"
#include "d3d12util/PipelineState.h"
#include "d3d12util/ColorBuffer.h"
#include "d3d12util/gpubuffer.h"
#include "d3d12util/CommandContext.h"
#include "d3d12util/uploadbuffer.h"
#include "d3d12util/ImageScaling.h"
#include "d3d12util/GeometryRenderer.h"
#define TEST_SHADER 0



class CVideoRendererInputPin;



class CDX12VideoProcessor
	: public CVideoProcessor
	, public CDX9Device
{
private:
	friend class CVideoRendererInputPin;
	


	CComPtr<ID3D12Debug>    m_pD3DDebug;
	CComPtr<ID3D12Debug1>   m_pD3DDebug1;

	void GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

//Dxgi device and swapchain
	CComPtr<IDXGIAdapter> m_pDXGIAdapter;
	CComPtr<IDXGIFactory2>   m_pDXGIFactory2;
	CComPtr<IDXGISwapChain1> m_pDXGISwapChain1;
	CComPtr<IDXGISwapChain4> m_pDXGISwapChain4;
	CComPtr<IDXGIOutput>    m_pDXGIOutput;
	CComPtr<IDXGIFactory1> m_pDXGIFactory1;
	
	//video prop
	D3D12_VIDEO_FIELD_TYPE m_SampleFormat;

	/*d3d9 subpic*/
	CComPtr<IDirect3DSurface9>        m_pSurface9SubPic;
	CComPtr<ID3D12Resource>           m_pTextureSubPic;
	//CComPtr<ID3D11ShaderResourceView> m_pShaderResourceSubPic;
	bool                              m_bSubPicWasRendered = false;

	DXGI_SWAP_EFFECT              m_UsedSwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	// D3D12 Video Processor
	CD3D12VP m_D3D12VP;
	const wchar_t* m_strCorrection = nullptr;
	//Instead of dynamycally recreating the shaders we use a constant buffer
	//TODO dont update it when it didn't change
	typedef struct {
		FLOAT Colorspace[4 * 3];
		FLOAT Opacity;
		FLOAT LuminanceScale;
		FLOAT BoundaryX;
		FLOAT BoundaryY;
		FLOAT padding[48]; // 256 bytes alignment
	} PS_CONSTANT_BUFFER;
	PS_CONSTANT_BUFFER* m_sShaderConstants;

	//ByteAddressBuffer m_pVideoQuadVertex;
	//ByteAddressBuffer m_pVideoIndexBuffer;
	UploadBuffer m_pVertexBuffer;
	UploadBuffer m_pIndexBuffer;
	UploadBuffer m_pViewpointShaderConstant;
	UploadBuffer m_pPixelShaderConstants;
	D3D12_VERTEX_BUFFER_VIEW m_pVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_pIndexBufferView;
	D3D12_DESCRIPTOR_HEAP_DESC m_pVertexHeapDesc;//m_cbvSrvHeap
	CComPtr<ID3D12DescriptorHeap> m_pVertexHeap;//m_cbvSrvHeap

	bool resetquad = false;
	ID3D12Resource* SwapChainBuffer[3];
	ColorBuffer SwapChainBufferColor[3];
	ColorBuffer m_pResizeResource;// same format as back buffer,will have the plane rendered onto
	ColorBuffer m_pVideoOutputResource;// same format as back buffer,will have the plane rendered onto
	ColorBuffer m_pPlaneResource[2];//Those surface are for copy texture from nv12 to rgb
	D3DCOLOR m_dwStatsTextColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	DXGI_COLOR_SPACE_TYPE m_currentSwapChainColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;


	

	GraphRectangle m_StatsBackground;
	GraphRectangle m_Rect3D;
	GraphRectangle m_Underlay;
	std::vector<GraphLine> m_Lines;
	int p_CurrentBuffer = 0;

	typedef struct  {
		struct {
			FLOAT x;
			FLOAT y;
			FLOAT z;
		} position;
		struct {
			FLOAT u;
			FLOAT v;
		} texture;
	} VideoQuadVertex;



public:
	CDX12VideoProcessor(CMpcVideoRenderer* pFilter, const Settings_t& config, HRESULT& hr);
	~CDX12VideoProcessor() override;

//CVideoProcessor
	int Type() override { return VP_DX12; }
	HRESULT Init(const HWND hwnd, bool* pChangeDevice = nullptr) override;
	BOOL VerifyMediaType(const CMediaType* pmt) override;
	BOOL InitMediaType(const CMediaType* pmt) override;
	void Configure(const Settings_t& config) override;
	HRESULT Render(int field) override;
	void SetRotation(int value) override;

	void Flush() override;

	void ClearPreScaleShaders() override;
	void ClearPostScaleShaders() override;

	HRESULT AddPreScaleShader(const std::wstring& name, const std::string& srcCode) override;
	HRESULT AddPostScaleShader(const std::wstring& name, const std::string& srcCode) override;

	HRESULT InitializeTexVP(const FmtConvParams_t& params, const UINT width, const UINT height);
	void UpdateFrameProperties() {
		m_srcPitch = m_srcWidth * m_srcParams.Packsize;
		m_srcLines = m_srcHeight * m_srcParams.PitchCoeff / 2;
	}
	bool Initialized();
private:
	void ReleaseDevice();
	void ReleaseSwapChain();
	void SetupQuad();
	void UpdateQuad();

	bool HandleHDRToggle();

	bool Preferred10BitOutput() {
		return m_bitsPerChannelSupport >= 10 && (m_InternalTexFmt == DXGI_FORMAT_R10G10B10A2_UNORM || m_InternalTexFmt == DXGI_FORMAT_R16G16B16A16_FLOAT);
	}
	//Variables
	bool m_bIsFullscreen = false;

	bool m_bHdrPassthroughSupport = false;
	bool m_bHdrDisplaySwitching = false; // switching HDR display in progress
	bool m_bHdrDisplayModeEnabled = false;
	bool m_bHdrAllowSwitchDisplay = true;
	UINT m_srcVideoTransferFunction = 0; // need a description or rename

	std::map<std::wstring, BOOL> m_hdrModeSavedState;
	std::map<std::wstring, BOOL> m_hdrModeStartState;

	struct HDRMetadata {
		DXGI_HDR_METADATA_HDR10 hdr10 = {};
		bool bValid = false;
	};
	HDRMetadata m_hdr10 = {};
	HDRMetadata m_lastHdr10 = {};

	// Input parameters
	DXGI_FORMAT m_srcDXGIFormat = DXGI_FORMAT_UNKNOWN;

	// intermediate texture format
	DXGI_FORMAT m_InternalTexFmt = DXGI_FORMAT_B8G8R8A8_UNORM;

	// swap chain format
	DXGI_FORMAT m_SwapChainFmt = DXGI_FORMAT_R10G10B10A2_UNORM;
	UINT32 m_bitsPerChannelSupport = 8;

	bool m_bCallbackDeviceIsSet = false;

	void SetCallbackDevice(const bool bChangeDevice = false);
//CVideoProcessor
	void SetGraphSize() override;
	BOOL GetAlignmentSize(const CMediaType& mt, SIZE& Size) override;

	void SetShaderConvertColorParams();
	CONSTANT_BUFFER_VAR m_pBufferVar;

	bool m_PSConvColorData = false;

	HRESULT Process(const CRect& srcRect, const CRect& dstRect, const bool second);

	HRESULT ProcessSample(IMediaSample* pSample) override;
	HRESULT CopySample(IMediaSample* pSample);

	/*Drawing graphs*/
	void DrawStats(GraphicsContext& Context, float x, float y, float w, float h);

	HRESULT FillBlack() override;

	void SetVideoRect(const CRect& videoRect)      override;
	HRESULT SetWindowRect(const CRect& windowRect) override;
	HRESULT Reset() override;
	bool IsInit() const override { return m_bHdrDisplaySwitching; }

	IDirect3DDeviceManager9* GetDeviceManager9() override { return GetDevMan9(); }
	HRESULT GetCurentImage(long* pDIBImage) override;
	HRESULT GetDisplayedImage(BYTE** ppDib, unsigned* pSize) override;
	HRESULT GetVPInfo(std::wstring& str) override;

	void UpdateStatsPresent();
	void UpdateStatsStatic();
	
public:
	HRESULT SetDevice(ID3D12Device* pDevice, const bool bDecoderDevice);
	

	// IMFVideoProcessor
	STDMETHODIMP SetProcAmpValues(DWORD dwFlags, DXVA2_ProcAmpValues* pValues) override;

	// IMFVideoMixerBitmap
	STDMETHODIMP SetAlphaBitmap(const MFVideoAlphaBitmap* pBmpParms) override;
	STDMETHODIMP UpdateAlphaBitmapParameters(const MFVideoAlphaBitmapParams* pBmpParms) override;
};
