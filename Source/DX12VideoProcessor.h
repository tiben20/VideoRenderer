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

#define TEST_SHADER 0

class CVideoRendererInputPin;

typedef struct {
	ID3D12Resource* resource = nullptr;
	ID3D12DescriptorHeap* heap = nullptr;
	UINT                  descriptorSize = 0;
} DescriptorAllocator;

class CDX12VideoProcessor
	: public CVideoProcessor
	, public CDX9Device
{
private:
	friend class CVideoRendererInputPin;
	//CComPtr
// Direct3D 12
	CComPtr<ID3D12Device1>        m_pDevice;
	DescriptorAllocator m_pSamplerPoint = {0};
	DescriptorAllocator m_pSamplerLinear = { 0 };
	DescriptorAllocator m_pSamplerDither = { 0 };
	CComPtr<ID3D12PipelineState>     m_pAlphaBlendState;
	CComPtr<ID3D12PipelineState>     m_pAlphaBlendStateInv;

	CComPtr<ID3D12Resource>          m_pFullFrameVertexBuffer = nullptr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
	CComPtr<ID3D12PipelineState> m_PSO;
	CComPtr<ID3D12RootSignature> m_pRootSignature;
	D3D12_SHADER_BYTECODE m_pVS_Simple = { 0 };
	D3D12_SHADER_BYTECODE m_pPS_Simple = { 0 };
	/*BLEND STATE*/
	D3D12_BLEND_DESC BlendNoColorWrite = { 0 };		// XXX
	D3D12_BLEND_DESC BlendDisable = { 0 };			// 1, 0
	D3D12_BLEND_DESC BlendPreMultiplied = { 0 };		// 1, 1-SrcA
	D3D12_BLEND_DESC BlendTraditional = { 0 };		// SrcA, 1-SrcA
	D3D12_BLEND_DESC BlendAdditive = { 0 };			// 1, 1
	D3D12_BLEND_DESC BlendTraditionalAdditive = { 0 };// SrcA, 1
	D3D12_BLEND_DESC BlendTraditionalInverse = { 0 };// SrcA, 1

	CComPtr<ID3D12Debug>       m_pD3DDebug;
	CComPtr<ID3D12Debug1> m_pD3DDebug1;
	
	
	DescriptorAllocator m_pPostScaleConstants;

//Dxgi device and swapchain
	CComPtr<IDXGIAdapter> m_pDXGIAdapter;
	CComPtr<IDXGIFactory2>   m_pDXGIFactory2;
	CComPtr<IDXGISwapChain1> m_pDXGISwapChain1;
	CComPtr<IDXGISwapChain4> m_pDXGISwapChain4;
	CComPtr<IDXGIOutput>    m_pDXGIOutput;
	CComPtr<IDXGIFactory1> m_pDXGIFactory1;
	/*d3d9 subpic*/
	CComPtr<IDirect3DSurface9>        m_pSurface9SubPic;
	CComPtr<ID3D12Resource>           m_pTextureSubPic;
	//CComPtr<ID3D11ShaderResourceView> m_pShaderResourceSubPic;
	bool                              m_bSubPicWasRendered = false;

	DXGI_SWAP_EFFECT              m_UsedSwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	// D3D12 Video Processor
	CD3D12VP m_D3D12VP;
public:
	CDX12VideoProcessor(CMpcVideoRenderer* pFilter, const Settings_t& config, HRESULT& hr);
	~CDX12VideoProcessor() override;
//CVideoProcessor
	int Type() override { return VP_DX12; }
	HRESULT Init(const HWND hwnd, bool* pChangeDevice = nullptr) override;
	BOOL VerifyMediaType(const CMediaType* pmt) override;
	BOOL InitMediaType(const CMediaType* pmt) override;
	void Configure(const Settings_t& config) override;

	void SetRotation(int value) override;

	void Flush() override;

	void ClearPreScaleShaders() override;
	void ClearPostScaleShaders() override;

	HRESULT AddPreScaleShader(const std::wstring& name, const std::string& srcCode) override;
	HRESULT AddPostScaleShader(const std::wstring& name, const std::string& srcCode) override;

	bool Initialized();
private:
	void ReleaseVP();
	void ReleaseDevice();
	void ReleaseSwapChain();



	bool HandleHDRToggle();

	bool Preferred10BitOutput() {
		return m_bitsPerChannelSupport >= 10 && (m_InternalTexFmt == DXGI_FORMAT_R10G10B10A2_UNORM || m_InternalTexFmt == DXGI_FORMAT_R16G16B16A16_FLOAT);
	}
	//Variables
	bool m_bDecoderDevice = false;
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

	// D3D11 VP texture format
	DXGI_FORMAT m_D3D11OutputFmt = DXGI_FORMAT_UNKNOWN;

	// intermediate texture format
	DXGI_FORMAT m_InternalTexFmt = DXGI_FORMAT_B8G8R8A8_UNORM;

	// swap chain format
	DXGI_FORMAT m_SwapChainFmt = DXGI_FORMAT_B8G8R8A8_UNORM;
	UINT32 m_bitsPerChannelSupport = 8;

	D3DCOLOR m_dwStatsTextColor = D3DCOLOR_XRGB(255, 255, 255);

	bool m_bCallbackDeviceIsSet = false;

	void SetCallbackDevice(const bool bChangeDevice = false);
//CVideoProcessor
	void SetGraphSize() override;
	BOOL GetAlignmentSize(const CMediaType& mt, SIZE& Size) override;

	HRESULT ProcessSample(IMediaSample* pSample) override;
	HRESULT Render(int field) override;
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
	
	HRESULT DrawStats(ID3D12Resource* pRenderTarget);

public:
	HRESULT SetDevice(ID3D12Device* pDevice, const bool bDecoderDevice);
	HRESULT InitSwapChain();

	// IMFVideoProcessor
	STDMETHODIMP SetProcAmpValues(DWORD dwFlags, DXVA2_ProcAmpValues* pValues) override;

	// IMFVideoMixerBitmap
	STDMETHODIMP SetAlphaBitmap(const MFVideoAlphaBitmap* pBmpParms) override;
	STDMETHODIMP UpdateAlphaBitmapParameters(const MFVideoAlphaBitmapParams* pBmpParms) override;
};
