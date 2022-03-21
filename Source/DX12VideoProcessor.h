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
#include "d3d12util/display.h"
#include "d3d12util/ColorBuffer.h"
#include "d3d12util/gpubuffer.h"
#include "d3d12util/CommandContext.h"
#include "d3d12util/uploadbuffer.h"
#define TEST_SHADER 0

class CVideoRendererInputPin;

class CDX12VideoProcessor
	: public CVideoProcessor
	, public CDX9Device
{
private:
	friend class CVideoRendererInputPin;
	//CComPtr
// Direct3D 12
	CopyFrameDataFn m_pConvertFn = nullptr;
	CopyFrameDataFn m_pCopyGpuFn = CopyFrameAsIs;
	CComPtr<ID3D12Device1>        m_pDevice;
	//DescriptorAllocator m_pSamplerPoint;
	//DescriptorAllocator m_pSamplerLinear;
	//DescriptorAllocator m_pSamplerDither;
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

	/*from d3d12 hello*/
	static const UINT FrameCount = 2;
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	void GetHardwareAdapter(
		IDXGIFactory1* pFactory,
		IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

	float m_aspectRatio;

	// Adapter info.
	bool m_useWarpDevice;

	UINT m_rtvDescriptorSize;

	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	CComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;
	/*d3d hello*/
	//DescriptorAllocator m_pPostScaleConstants;

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
	const wchar_t* m_strCorrection = nullptr;

	/*d3d12engine*/
	std::vector<GraphicsPSO> sm_PSOs;
	RootSignature m_RootSig;

	GraphicsPSO m_VideoPSO; //video pso
	GraphicsPSO BilinearUpsamplePS2;


	typedef struct {
		FLOAT Colorspace[4 * 3];
		FLOAT Opacity;
		FLOAT LuminanceScale;
		FLOAT BoundaryX;
		FLOAT BoundaryY;
		FLOAT padding[48]; // 256 bytes alignment
	} PS_CONSTANT_BUFFER;
	PS_CONSTANT_BUFFER* m_sShaderConstants;

	typedef struct {
		FLOAT View[4 * 4];
		FLOAT Zoom[4 * 4];
		FLOAT Projection[4 * 4];
		FLOAT padding[16]; // 256 bytes alignment
	} VS_PROJECTION_CONST;
	VS_PROJECTION_CONST* m_sVertexConstants;

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
	


	DescriptorHeap m_pQuadHeap;
	DescriptorHandle m_pQuadHeapHandle;
	bool resetquad = false;
	ID3D12Resource* SwapChainBuffer[3];
	ColorBuffer SwapChainBufferColor[3];
	ColorBuffer m_pScalingResource[2];
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

	struct VideoVertexBuffer {
		BYTE* data;
		uint8_t size;
		uint8_t allocated_size;
		uint8_t depthsize;
		uint8_t stride;
	};
	//VideoVertexBuffer m_pVideoBuffer;
	//GraphicsContext m_context;
	//ColorBuffer m_pVideoBuffer;
	//GpuBuffer m_pGpuBuffer;
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

	bool Initialized();
private:
	void ReleaseVP();
	void ReleaseDevice();
	void ReleaseSwapChain();
	void SetupQuad();
	void UpdateQuad();

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
	void SetShaderConvertColorParams();
	__declspec(align(16)) struct CONSTANT_BUFFER_VAR {
		DirectX::XMFLOAT4 cm_r;
		DirectX::XMFLOAT4 cm_g;
		DirectX::XMFLOAT4 cm_b;
		DirectX::XMFLOAT4 cm_c;
	};
	CONSTANT_BUFFER_VAR m_pBufferVar;

		bool m_PSConvColorData = false;
	HRESULT ProcessSample(IMediaSample* pSample) override;
	
	void Display(GraphicsContext& Context, float x, float y, float w, float h);

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
	HRESULT InitSwapChain();

	// IMFVideoProcessor
	STDMETHODIMP SetProcAmpValues(DWORD dwFlags, DXVA2_ProcAmpValues* pValues) override;

	// IMFVideoMixerBitmap
	STDMETHODIMP SetAlphaBitmap(const MFVideoAlphaBitmap* pBmpParms) override;
	STDMETHODIMP UpdateAlphaBitmapParameters(const MFVideoAlphaBitmapParams* pBmpParms) override;
};
