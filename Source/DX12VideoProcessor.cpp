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

#include "stdafx.h"
#include <uuids.h>
#include <mfapi.h> // for MR_BUFFER_SERVICE
#include <Mferror.h>
#include <Mfidl.h>
#include <dwmapi.h>
#include <xutility>
#include "Helper.h"
#include "Times.h"
#include "resource.h"
#include "VideoRenderer.h"
#include "../Include/Version.h"
#include "d3d10.h"
#include "../Include/ID3DVideoMemoryConfiguration.h"
#include "Shaders.h"
#include "Utils/CPUInfo.h"

#include "../external/minhook/include/MinHook.h"

#include "DX12VideoProcessor.h"
#pragma comment( lib, "d3d12.lib" )
#pragma comment(lib, "dxguid.lib")
bool g_bPresent12 = false;
bool bCreateSwapChain12 = false;

typedef BOOL(WINAPI* pSetWindowPos)(
	_In_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_ int X,
	_In_ int Y,
	_In_ int cx,
	_In_ int cy,
	_In_ UINT uFlags);

pSetWindowPos pOrigSetWindowPosDX12 = nullptr;
static BOOL WINAPI pNewSetWindowPosDX12(
	_In_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_ int X,
	_In_ int Y,
	_In_ int cx,
	_In_ int cy,
	_In_ UINT uFlags)
{
	if (g_bPresent12) {
		DLog(L"call SetWindowPos() function during Present()");
		uFlags |= SWP_ASYNCWINDOWPOS;
	}
	return pOrigSetWindowPosDX12(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

typedef LONG(WINAPI* pSetWindowLongA)(
	_In_ HWND hWnd,
	_In_ int nIndex,
	_In_ LONG dwNewLong);

pSetWindowLongA pOrigSetWindowLongADX12 = nullptr;
static LONG WINAPI pNewSetWindowLongADX12(
	_In_ HWND hWnd,
	_In_ int nIndex,
	_In_ LONG dwNewLong)
{
	if (bCreateSwapChain12) {
		DLog(L"Blocking call SetWindowLongA() function during create fullscreen swap chain");
		return 0L;
	}
	return pOrigSetWindowLongADX12(hWnd, nIndex, dwNewLong);
}

template <typename T>
inline bool HookFunc(T** ppSystemFunction, PVOID pHookFunction)
{
	return MH_CreateHook(*ppSystemFunction, pHookFunction, reinterpret_cast<LPVOID*>(ppSystemFunction)) == MH_OK;
}

struct VERTEX12 {
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 TexCoord;
};

struct PS_COLOR_TRANSFORM12 {
	DirectX::XMFLOAT4 cm_r;
	DirectX::XMFLOAT4 cm_g;
	DirectX::XMFLOAT4 cm_b;
	DirectX::XMFLOAT4 cm_c;
};

struct PS_EXTSHADER_CONSTANTS12 {
	DirectX::XMFLOAT2 pxy; // pixel size in normalized coordinates
	DirectX::XMFLOAT2 wh;  // width and height of texture
	uint32_t counter;      // rendered frame counter
	float clock;           // some time in seconds
	float reserved1;
	float reserved2;
};

static_assert(sizeof(PS_EXTSHADER_CONSTANTS12) % 16 == 0);

HRESULT CreateVertexBuffer12(ID3D12Device* pDevice, ID3D12Resource** ppVertexBuffer,
	const UINT srcW, const UINT srcH, const RECT& srcRect,
	const int iRotation, const bool bFlip)
{
	ASSERT(ppVertexBuffer);
	ASSERT(*ppVertexBuffer == nullptr);

	const float src_dx = 1.0f / srcW;
	const float src_dy = 1.0f / srcH;
	float src_l = src_dx * srcRect.left;
	float src_r = src_dx * srcRect.right;
	const float src_t = src_dy * srcRect.top;
	const float src_b = src_dy * srcRect.bottom;

	POINT points[4];
	switch (iRotation) {
	case 90:
		points[0] = { -1, +1 };
		points[1] = { +1, +1 };
		points[2] = { -1, -1 };
		points[3] = { +1, -1 };
		break;
	case 180:
		points[0] = { +1, +1 };
		points[1] = { +1, -1 };
		points[2] = { -1, +1 };
		points[3] = { -1, -1 };
		break;
	case 270:
		points[0] = { +1, -1 };
		points[1] = { -1, -1 };
		points[2] = { +1, +1 };
		points[3] = { -1, +1 };
		break;
	default:
		points[0] = { -1, -1 };
		points[1] = { -1, +1 };
		points[2] = { +1, -1 };
		points[3] = { +1, +1 };
		break;
	}

	if (bFlip) {
		std::swap(src_l, src_r);
	}

	VERTEX12 Vertices[4] = {
		// Vertices for drawing whole texture
		// 2 ___4
		//  |\ |
		// 1|_\|3
		{ {(float)points[0].x, (float)points[0].y, 0}, {src_l, src_b} },
		{ {(float)points[1].x, (float)points[1].y, 0}, {src_l, src_t} },
		{ {(float)points[2].x, (float)points[2].y, 0}, {src_r, src_b} },
		{ {(float)points[3].x, (float)points[3].y, 0}, {src_r, src_t} },
	};
	D3D12_HEAP_PROPERTIES constProp;
	ZeroMemory(&constProp, sizeof(D3D12_HEAP_PROPERTIES));
	constProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	constProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	constProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	constProp.CreationNodeMask = 1;
	constProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC constantDesc;
	ZeroMemory(&constantDesc, sizeof(D3D12_RESOURCE_DESC));
	constantDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	//quad->vertexStride = sizeof(d3d_vertex_t);
	constantDesc.Width = 256;// sizeof(Vertices); dont work need to be multiple of 256
	constantDesc.Height = 1;
	constantDesc.DepthOrArraySize = 1;
	constantDesc.MipLevels = 1;
	constantDesc.Format = DXGI_FORMAT_UNKNOWN;
	constantDesc.SampleDesc.Count = 1;
	constantDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	constantDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = pDevice->CreateCommittedResource(&constProp, D3D12_HEAP_FLAG_NONE,
		&constantDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
		IID_ID3D12Resource, (void**)ppVertexBuffer);
	DLogIf(FAILED(hr), L"CreateVertexBuffer() : CreateBuffer() failed with error {}", HR2Str(hr));

	return hr;
}

CDX12VideoProcessor::CDX12VideoProcessor(CMpcVideoRenderer* pFilter, const Settings_t& config, HRESULT& hr)
  : CVideoProcessor(pFilter)
{
	m_bShowStats = config.bShowStats;
	m_iResizeStats = config.iResizeStats;
	m_iTexFormat = config.iTexFormat;
	m_VPFormats = config.VPFmts;
	m_bDeintDouble = config.bDeintDouble;
	m_bVPScaling = config.bVPScaling;
	m_iChromaScaling = config.iChromaScaling;
	m_iUpscaling = config.iUpscaling;
	m_iDownscaling = config.iDownscaling;
	m_bInterpolateAt50pct = config.bInterpolateAt50pct;
	m_bUseDither = config.bUseDither;
	m_iSwapEffect = config.iSwapEffect;
	m_bVBlankBeforePresent = config.bVBlankBeforePresent;
	m_bHdrPassthrough = config.bHdrPassthrough;
	m_iHdrToggleDisplay = config.iHdrToggleDisplay;
	m_bConvertToSdr = config.bConvertToSdr;

	m_nCurrentAdapter = -1;
	m_pDisplayMode = &m_DisplayMode;

	hr = CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&m_pDXGIFactory1);
	if (FAILED(hr)) {
		DLog(L"CDX12VideoProcessor::CDX12VideoProcessor() : CreateDXGIFactory1() failed with error {}", HR2Str(hr));
		return;
	}

	// set default ProcAmp ranges and values
	SetDefaultDXVA2ProcAmpRanges(m_DXVA2ProcAmpRanges);
	SetDefaultDXVA2ProcAmpValues(m_DXVA2ProcAmpValues);

	pOrigSetWindowPosDX12 = SetWindowPos;
	auto ret = HookFunc(&pOrigSetWindowPosDX12, pNewSetWindowPosDX12);
	DLogIf(!ret, L"CDX12VideoProcessor::CDX12VideoProcessor() : hook for SetWindowPos() fail");

	pOrigSetWindowLongADX12 = SetWindowLongA;
	ret = HookFunc(&pOrigSetWindowLongADX12, pNewSetWindowLongADX12);
	DLogIf(!ret, L"CDX12VideoProcessor::CDX12VideoProcessor() : hook for SetWindowLongA() fail");

	MH_EnableHook(MH_ALL_HOOKS);

	CComPtr<IDXGIAdapter> pDXGIAdapter;
	for (UINT adapter = 0; m_pDXGIFactory1->EnumAdapters(adapter, &pDXGIAdapter) != DXGI_ERROR_NOT_FOUND; ++adapter) {
		CComPtr<IDXGIOutput> pDXGIOutput;
		for (UINT output = 0; pDXGIAdapter->EnumOutputs(output, &pDXGIOutput) != DXGI_ERROR_NOT_FOUND; ++output) {
			DXGI_OUTPUT_DESC desc{};
			if (SUCCEEDED(pDXGIOutput->GetDesc(&desc))) {
				DisplayConfig_t displayConfig = {};
				if (GetDisplayConfig(desc.DeviceName, displayConfig)) {
					m_hdrModeStartState[desc.DeviceName] = displayConfig.advancedColor.advancedColorEnabled;
				}
			}

			pDXGIOutput.Release();
		}

		pDXGIAdapter.Release();
	}
}

CDX12VideoProcessor::~CDX12VideoProcessor()
{
}

HRESULT CDX12VideoProcessor::Init(const HWND hwnd, bool* pChangeDevice)
{
	DLog(L"CDX12VideoProcessor::Init()");

#if _DEBUG
	//need to do this before any init
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_pD3DDebug))))
	{
		m_pD3DDebug->EnableDebugLayer();
		m_pD3DDebug->QueryInterface(IID_PPV_ARGS(&m_pD3DDebug1));
		//m_pD3DDebug1->SetEnableGPUBasedValidation(true);
		m_pD3DDebug1->SetEnableSynchronizedCommandQueueValidation(1);
	}
#endif
	m_hWnd = hwnd;
	m_bHdrPassthroughSupport = false;
	m_bHdrDisplayModeEnabled = false;
	m_bitsPerChannelSupport = 8;

	MONITORINFOEXW mi = { sizeof(mi) };
	GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY), (MONITORINFO*)&mi);
	DisplayConfig_t displayConfig = {};

	if (GetDisplayConfig(mi.szDevice, displayConfig)) {
		const auto& ac = displayConfig.advancedColor;
		m_bHdrPassthroughSupport = ac.advancedColorSupported && ac.advancedColorEnabled;
		m_bHdrDisplayModeEnabled = ac.advancedColorEnabled;
		m_bitsPerChannelSupport = displayConfig.bitsPerChannel;
	}

	if (m_bIsFullscreen != m_pFilter->m_bIsFullscreen) {
		m_srcVideoTransferFunction = 0;
	}

	//IDXGIAdapter* pDXGIAdapter = nullptr;
	const UINT currentAdapter = GetAdapter(hwnd, m_pDXGIFactory1, &m_pDXGIAdapter);
	CheckPointer(m_pDXGIAdapter, E_FAIL);
	if (m_nCurrentAdapter == currentAdapter) {
		//SAFE_RELEASE(pDXGIAdapter);
		if (hwnd) {
			HRESULT hr = InitDX9Device(hwnd, pChangeDevice);
			ASSERT(S_OK == hr);
			if (m_pD3DDevEx) {
				// set a special blend mode for alpha channels for ISubRenderCallback rendering
				// this is necessary for the second alpha blending
				m_pD3DDevEx->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
				m_pD3DDevEx->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
				m_pD3DDevEx->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
				m_pD3DDevEx->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);

				SetCallbackDevice(pChangeDevice ? *pChangeDevice : false);
			}
		}

		if (!m_pDXGISwapChain1 || m_bIsFullscreen != m_pFilter->m_bIsFullscreen) {
			InitSwapChain();
			UpdateStatsStatic();
			m_pFilter->OnDisplayModeChange();
		}

		return S_OK;
	}
	m_nCurrentAdapter = currentAdapter;

	if (m_bDecoderDevice && m_pDXGISwapChain1) {
		return S_OK;
	}

	ReleaseSwapChain();
	m_pDXGIFactory2.Release();
	ReleaseDevice();
	//can we go lower?
	D3D_FEATURE_LEVEL max_level = D3D_FEATURE_LEVEL_12_1;

	ID3D12Device* pDevice = nullptr;
	
	HRESULT hr = D3D12CreateDevice(m_pDXGIAdapter, max_level, IID_PPV_ARGS(&pDevice));
	

	//TODO the d11 version deallocate the dxgiadapter and request the one from the filter i assume
	//SAFE_RELEASE(m_pDXGIAdapter);
	if (FAILED(hr)) {
		DLog(L"CDX12VideoProcessor::Init() : D3D11CreateDevice() failed with error {}", HR2Str(hr));
		return hr;
	}

	DLog(L"CDX12VideoProcessor::Init() : D3D11CreateDevice() successfully with feature level 12_1");

	hr = SetDevice(pDevice, false);
	pDevice->Release();

	if (S_OK == hr) {
		if (pChangeDevice) {
			*pChangeDevice = true;
		}
	}

	if (m_VendorId == PCIV_INTEL && CPUInfo::HaveSSE41()) {
		m_pCopyGpuFn = CopyGpuFrame_SSE41;
	}
	else {
		m_pCopyGpuFn = CopyFrameAsIs;
	}

	return hr;
}

HRESULT CDX12VideoProcessor::SetDevice(ID3D12Device* pDevice, const bool bDecoderDevice)
{
#if 1
	

	DLog(L"CDX12VideoProcessor::SetDevice()");

	ReleaseSwapChain();
	m_pDXGIFactory2.Release();
	ReleaseDevice();

	CheckPointer(pDevice, E_POINTER);
	
	
	HRESULT hr = pDevice->QueryInterface(IID_PPV_ARGS(&m_pDevice));
	if (FAILED(hr)) {
		return hr;
	}
	//todo
	hr = m_D3D12VP.InitVideoDevice(m_pDevice);
	DLogIf(FAILED(hr), L"CDX12VideoProcessor::SetDevice() : InitVideoDevice failed with error {}", HR2Str(hr));
	ZeroMemory(&m_pPostScaleConstants, sizeof(m_pPostScaleConstants));
	ZeroMemory(&m_pSamplerPoint, sizeof(m_pSamplerPoint));
	ZeroMemory(&m_pSamplerLinear, sizeof(m_pSamplerLinear));
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.NumDescriptors = 2 + 4 * 64; 
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 1;
	EXECUTE_ASSERT(S_OK == pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pPostScaleConstants.heap)));
	D3D12_SAMPLER_DESC SampDesc = {};
	SampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	SampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SampDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SampDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	SampDesc.MinLOD = 0;
	SampDesc.MaxLOD = D3D12_FLOAT32_MAX;
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	D3D12_DESCRIPTOR_HEAP_DESC sampleDesc;
	sampleDesc.NumDescriptors = 256;
	sampleDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	sampleDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	sampleDesc.NodeMask = 1;
	m_pDevice->CreateDescriptorHeap(&sampleDesc, IID_PPV_ARGS(&m_pSamplerPoint.heap));
	
	m_pDevice->CreateSampler(&SampDesc, m_pSamplerPoint.heap->GetCPUDescriptorHandleForHeapStart());

	//EXECUTE_ASSERT(S_OK == m_pDevice->CreateSampler(&SampDesc, m_pSamplerPoint));

	SampDesc.Filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT; // linear interpolation for magnification
	//EXECUTE_ASSERT(S_OK == m_pDevice->CreateSamplerState(&SampDesc, &m_pSamplerLinear));
	m_pDevice->CreateDescriptorHeap(&sampleDesc, IID_PPV_ARGS(&m_pSamplerLinear.heap));
	m_pDevice->CreateSampler(&SampDesc, m_pSamplerLinear.heap->GetCPUDescriptorHandleForHeapStart());

	SampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	SampDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SampDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//EXECUTE_ASSERT(S_OK == m_pDevice->CreateSamplerState(&SampDesc, &m_pSamplerDither));
	m_pDevice->CreateDescriptorHeap(&sampleDesc, IID_PPV_ARGS(&m_pSamplerDither.heap));
	m_pDevice->CreateSampler(&SampDesc, m_pSamplerDither.heap->GetCPUDescriptorHandleForHeapStart());
	ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
	m_PSODesc.NodeMask = 1;
	m_PSODesc.SampleMask = 0xFFFFFFFFu;
	m_PSODesc.SampleDesc.Count = 1;
	

	D3D12_BLEND_DESC bdesc = {};
	bdesc.RenderTarget[0].BlendEnable = FALSE;
	bdesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	bdesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_ALPHA;
	bdesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	bdesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	bdesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	bdesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	bdesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	BlendDisable = bdesc;
	bdesc.RenderTarget[0].BlendEnable = TRUE;
	BlendTraditional = bdesc;

	bdesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	BlendPreMultiplied = bdesc;

	bdesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	BlendAdditive = bdesc;

	bdesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	BlendTraditionalAdditive = bdesc;
	//m_PSODesc.BlendState = bdesc;
	//EXECUTE_ASSERT(S_OK == m_pDevice->CreateBlendState(&bdesc, &m_pAlphaBlendState));
	bdesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	BlendTraditionalInverse = bdesc;
	//EXECUTE_ASSERT(S_OK == m_pDevice->CreateBlendState(&bdesc, &m_pAlphaBlendStateInv));

	EXECUTE_ASSERT(S_OK == CreateVertexBuffer12(m_pDevice, &m_pFullFrameVertexBuffer, 1, 1, CRect(0, 0, 1, 1), 0, false));

	LPVOID data;
	DWORD size;
	EXECUTE_ASSERT(S_OK == GetDataFromResource(data, size, IDF_VSH11_SIMPLE));
	LPVOID dataVS;
	DWORD sizeVS;
	EXECUTE_ASSERT(S_OK == GetDataFromResource(dataVS, sizeVS, IDF_PSH11_SIMPLE));
	//TODO SHADER CREATER
	//EXECUTE_ASSERT(S_OK == m_pDevice->CreateVertexShader(data, size, nullptr, &m_pVS_Simple));
	static D3D12_INPUT_ELEMENT_DESC Layout[] =
	{
			{ "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,     0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	bool sharp = true;//todo
	D3D12_STATIC_SAMPLER_DESC samplers[2];
	ZeroMemory(samplers, sizeof(samplers));
	samplers[0].Filter = sharp ? D3D12_FILTER_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS; // NEVER
	samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplers[0].MinLOD = 0.0f;
	samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	samplers[0].ShaderRegister = 0;
	samplers[1] = samplers[0];
	samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplers[1].ShaderRegister = 1;

	D3D12_ROOT_SIGNATURE_DESC1 rootDesc;
	std::vector<D3D12_ROOT_PARAMETER1> rootTables;
	rootDesc.NumParameters = rootTables.size();
	rootDesc.pParameters = rootTables.data();
	rootDesc.NumStaticSamplers = (sizeof(samplers) / sizeof((samplers)[0]));
	rootDesc.pStaticSamplers = samplers;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignDesc;
	rootSignDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSignDesc.Desc_1_1 = rootDesc;
	ID3DBlob* signatureblob, * pErrBlob;
	hr = D3D12SerializeVersionedRootSignature(&rootSignDesc, &signatureblob, &pErrBlob);
	hr = m_pDevice->CreateRootSignature(0, signatureblob->GetBufferPointer(),
		signatureblob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
	signatureblob->Release();

	m_pPS_Simple.BytecodeLength = size;
	m_pPS_Simple.pShaderBytecode = data;
	m_pVS_Simple.BytecodeLength = sizeVS;
	m_pVS_Simple.pShaderBytecode = dataVS;
	m_PSODesc.InputLayout.NumElements = std::size(Layout);
	m_PSODesc.InputLayout.pInputElementDescs = Layout;
	m_PSODesc.pRootSignature = m_pRootSignature,
	m_PSODesc.VS = m_pVS_Simple;
	m_PSODesc.PS = m_pPS_Simple;
	m_PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	m_PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	m_PSODesc.RasterizerState.FrontCounterClockwise = FALSE;
	m_PSODesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	m_PSODesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	m_PSODesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	m_PSODesc.RasterizerState.DepthClipEnable = TRUE;
	m_PSODesc.RasterizerState.MultisampleEnable = FALSE;
	m_PSODesc.RasterizerState.AntialiasedLineEnable = FALSE;
	m_PSODesc.RasterizerState.ForcedSampleCount = 0;
	m_PSODesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	m_PSODesc.DepthStencilState.DepthEnable = FALSE;
	m_PSODesc.DepthStencilState.StencilEnable = FALSE;
	m_PSODesc.SampleMask = UINT_MAX;
	m_PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_PSODesc.SampleDesc.Count = 1;
	//m_PSODesc.
	D3D12_CONSTANT_BUFFER_VIEW_DESC BufferDesc = { 0 };
	BufferDesc.SizeInBytes = 256;// sizeof(PS_EXTSHADER_CONSTANTS12); need to be multiple of 256
	BufferDesc.BufferLocation = m_pFullFrameVertexBuffer->GetGPUVirtualAddress();
	

	m_pDevice->CreateConstantBufferView(&BufferDesc, m_pPostScaleConstants.heap->GetCPUDescriptorHandleForHeapStart());

	//just to remember its the start
	m_pPostScaleConstants.descriptorSize += 0;// its the start of the handle
	hr = m_pDevice->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO));
	if (FAILED(hr))
	{
		//the pipelinestate need to be done at the end
		assert(0);
	}
	//
		//D3D11_BUFFER_DESC BufferDesc = {};
		//BufferDesc.Usage = D3D11_USAGE_DEFAULT;
		//BufferDesc.ByteWidth = sizeof(PS_EXTSHADER_CONSTANTS12);
		//BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		//BufferDesc.CPUAccessFlags = 0;
		//EXECUTE_ASSERT(S_OK == m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pPostScaleConstants));
	//todo
	//EXECUTE_ASSERT(S_OK == m_pDevice->CreateInputLayout(Layout, std::size(Layout), data, size, &m_pVSimpleInputLayout));

	//EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPS_Simple, IDF_PSH11_SIMPLE));

	CComPtr<IDXGIDevice> pDXGIDevice;
	hr = m_pDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));
	//you cant query dxgidevice in d3d12

	CComPtr<IDXGIFactory1> pDXGIFactory1;
	hr = m_pDXGIAdapter->GetParent(IID_PPV_ARGS(&pDXGIFactory1));
	if (FAILED(hr)) {
		DLog(L"CDX12VideoProcessor::SetDevice() : GetParent(IDXGIFactory1) failed with error {}", HR2Str(hr));
		ReleaseDevice();
		return hr;
	}

	hr = pDXGIFactory1->QueryInterface(IID_PPV_ARGS(&m_pDXGIFactory2));
	if (FAILED(hr)) {
		DLog(L"CDX12VideoProcessor::SetDevice() : QueryInterface(IDXGIFactory2) failed with error {}", HR2Str(hr));
		ReleaseDevice();
		return hr;
	}

	if (m_pFilter->m_inputMT.IsValid()) {
		if (!InitMediaType(&m_pFilter->m_inputMT)) {
			ReleaseDevice();
			return E_FAIL;
		}
	}

	bool changeDevice = false;
	hr = InitDX9Device(m_hWnd, &changeDevice);
	ASSERT(S_OK == hr);
	if (m_pD3DDevEx) {
		// set a special blend mode for alpha channels for ISubRenderCallback rendering
		// this is necessary for the second alpha blending
		m_pD3DDevEx->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
		m_pD3DDevEx->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
		m_pD3DDevEx->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
		m_pD3DDevEx->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);

		SetCallbackDevice(changeDevice);
	}

	if (m_hWnd) {
		hr = InitSwapChain();
		if (FAILED(hr)) {
			ReleaseDevice();
			return hr;
		}
	}

	DXGI_ADAPTER_DESC dxgiAdapterDesc = {};
	hr = m_pDXGIAdapter->GetDesc(&dxgiAdapterDesc);
	if (SUCCEEDED(hr)) {
		m_VendorId = dxgiAdapterDesc.VendorId;
		m_strAdapterDescription = fmt::format(L"{} ({:04X}:{:04X})", dxgiAdapterDesc.Description, dxgiAdapterDesc.VendorId, dxgiAdapterDesc.DeviceId);
		DLog(L"Graphics DXGI adapter: {}", m_strAdapterDescription);
	}
#else
	HRESULT hr2 = m_Font3D.InitDeviceObjects(m_pDevice, m_pDeviceContext);
	DLogIf(FAILED(hr2), L"m_Font3D.InitDeviceObjects() failed with error {}", HR2Str(hr2));
	if (SUCCEEDED(hr2)) {
		hr2 = m_StatsBackground.InitDeviceObjects(m_pDevice, m_pDeviceContext);
		hr2 = m_Rect3D.InitDeviceObjects(m_pDevice, m_pDeviceContext);
		hr2 = m_Underlay.InitDeviceObjects(m_pDevice, m_pDeviceContext);
		hr2 = m_Lines.InitDeviceObjects(m_pDevice, m_pDeviceContext);
		hr2 = m_SyncLine.InitDeviceObjects(m_pDevice, m_pDeviceContext);
		DLogIf(FAILED(hr2), L"Geometric primitives InitDeviceObjects() failed with error {}", HR2Str(hr2));
	}
	ASSERT(S_OK == hr2);

	HRESULT hr3 = m_TexDither.Create(m_pDevice, DXGI_FORMAT_R16G16B16A16_FLOAT, dither_size, dither_size, Tex2D_DynamicShaderWrite);
	if (S_OK == hr3) {
		hr3 = GetDataFromResource(data, size, IDF_DITHER_32X32_FLOAT16);
		if (S_OK == hr3) {
			D3D12_MAPPED_SUBRESOURCE mappedResource;
			hr3 = m_pDeviceContext->Map(m_TexDither.pTexture, 0, D3D12_MAP_WRITE_DISCARD, 0, &mappedResource);
			if (S_OK == hr3) {
				uint16_t* src = (uint16_t*)data;
				BYTE* dst = (BYTE*)mappedResource.pData;
				for (UINT y = 0; y < dither_size; y++) {
					uint16_t* pUInt16 = reinterpret_cast<uint16_t*>(dst);
					for (UINT x = 0; x < dither_size; x++) {
						*pUInt16++ = src[x];
						*pUInt16++ = src[x];
						*pUInt16++ = src[x];
						*pUInt16++ = src[x];
					}
					src += dither_size;
					dst += mappedResource.RowPitch;
				}
				m_pDeviceContext->Unmap(m_TexDither.pTexture, 0);
			}
		}
		if (FAILED(hr3)) {
			m_TexDither.Release();
		}
	}

	m_bDecoderDevice = bDecoderDevice;

	m_pFilter->OnDisplayModeChange();
	UpdateStatsStatic();
	SetGraphSize();


	m_pDevice->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_pAlphaBlendState));
	return hr;
#endif
	
}

HRESULT CDX12VideoProcessor::InitSwapChain()
{
	return S_OK;
#if 0
	DLog(L"CDX12VideoProcessor::InitSwapChain() - {}", m_pFilter->m_bIsFullscreen ? L"fullscreen" : L"window");
	CheckPointer(m_pDXGIFactory2, E_FAIL);

	ReleaseSwapChain();

	auto bFullscreenChange = m_bIsFullscreen != m_pFilter->m_bIsFullscreen;
	m_bIsFullscreen = m_pFilter->m_bIsFullscreen;

	if (bFullscreenChange) {
		HandleHDRToggle();
	}

	const auto bHdrOutput = m_bHdrPassthroughSupport && m_bHdrPassthrough && SourceIsHDR();
	const auto b10BitOutput = bHdrOutput || Preferred10BitOutput();
	m_SwapChainFmt = b10BitOutput ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;
	m_dwStatsTextColor = (m_bHdrDisplayModeEnabled && bHdrOutput) ? D3DCOLOR_XRGB(170, 170, 170) : D3DCOLOR_XRGB(255, 255, 255);

	HRESULT hr = S_OK;
	DXGI_SWAP_CHAIN_DESC1 desc1 = {};

	if (m_bIsFullscreen) {
		MONITORINFOEXW mi = { sizeof(mi) };
		GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
		const CRect rc(mi.rcMonitor);

		desc1.Width = rc.Width();
		desc1.Height = rc.Height();
		desc1.Format = m_SwapChainFmt;
		desc1.SampleDesc.Count = 1;
		desc1.SampleDesc.Quality = 0;
		desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		if ((m_iSwapEffect == SWAPEFFECT_Flip && IsWindows8OrGreater()) || bHdrOutput) {
			desc1.BufferCount = bHdrOutput ? 6 : 2;
			desc1.Scaling = DXGI_SCALING_NONE;
			desc1.SwapEffect = IsWindows10OrGreater() ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		}
		else { // default SWAPEFFECT_Discard
			desc1.BufferCount = 1;
			desc1.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		}
		desc1.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.RefreshRate.Numerator = 0;
		fullscreenDesc.RefreshRate.Denominator = 1;
		fullscreenDesc.Windowed = FALSE;

		bCreateSwapChain12 = true;
		hr = m_pDXGIFactory2->CreateSwapChainForHwnd(m_pDevice, m_hWnd, &desc1, &fullscreenDesc, nullptr, &m_pDXGISwapChain1);
		bCreateSwapChain12 = false;
		DLogIf(FAILED(hr), L"CDX12VideoProcessor::InitSwapChain() : CreateSwapChainForHwnd(fullscreen) failed with error {}", HR2Str(hr));
	}
	else {
		desc1.Width = std::max(8, m_windowRect.Width());
		desc1.Height = std::max(8, m_windowRect.Height());
		desc1.Format = m_SwapChainFmt;
		desc1.SampleDesc.Count = 1;
		desc1.SampleDesc.Quality = 0;
		desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		if ((m_iSwapEffect == SWAPEFFECT_Flip && IsWindows8OrGreater()) || bHdrOutput) {
			desc1.BufferCount = bHdrOutput ? 6 : 2;
			desc1.Scaling = DXGI_SCALING_NONE;
			desc1.SwapEffect = IsWindows10OrGreater() ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		}
		else { // default SWAPEFFECT_Discard
			desc1.BufferCount = 1;
			desc1.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		}
		desc1.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		DLogIf(m_windowRect.Width() < 8 || m_windowRect.Height() < 8,
			L"CDX12VideoProcessor::InitSwapChain() : Invalid window size {}x{}, use {}x{}",
			m_windowRect.Width(), m_windowRect.Height(), desc1.Width, desc1.Height);

		hr = m_pDXGIFactory2->CreateSwapChainForHwnd(m_pDevice, m_hWnd, &desc1, nullptr, nullptr, &m_pDXGISwapChain1);
		DLogIf(FAILED(hr), L"CDX12VideoProcessor::InitSwapChain() : CreateSwapChainForHwnd() failed with error {}", HR2Str(hr));
	}

	if (m_pDXGISwapChain1) {
		m_UsedSwapEffect = desc1.SwapEffect;

		HRESULT hr2 = m_pDXGISwapChain1->GetContainingOutput(&m_pDXGIOutput);

		m_currentSwapChainColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		if (bHdrOutput) {
			hr2 = m_pDXGISwapChain1->QueryInterface(IID_PPV_ARGS(&m_pDXGISwapChain4));
		}

		m_pShaderResourceSubPic.Release();
		m_pTextureSubPic.Release();

		m_pSurface9SubPic.Release();

		if (m_pD3DDevEx) {
			HANDLE sharedHandle = nullptr;
			hr2 = m_pD3DDevEx->CreateRenderTarget(
				m_d3dpp.BackBufferWidth,
				m_d3dpp.BackBufferHeight,
				D3DFMT_A8R8G8B8,
				D3DMULTISAMPLE_NONE,
				0,
				FALSE,
				&m_pSurface9SubPic,
				&sharedHandle);
			DLogIf(FAILED(hr2), L"CDX12VideoProcessor::InitSwapChain() : CreateRenderTarget(Direct3D9) failed with error {}", HR2Str(hr2));

			if (m_pSurface9SubPic) {
				hr2 = m_pDevice->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&m_pTextureSubPic));
				DLogIf(FAILED(hr2), L"CDX12VideoProcessor::InitSwapChain() : OpenSharedResource() failed with error {}", HR2Str(hr2));
			}

			if (m_pTextureSubPic) {
				D3D11_TEXTURE2D_DESC texdesc = {};
				m_pTextureSubPic->GetDesc(&texdesc);
				if (texdesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
					D3D11_SHADER_RESOURCE_VIEW_DESC shaderDesc;
					shaderDesc.Format = texdesc.Format;
					shaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					shaderDesc.Texture2D.MostDetailedMip = 0; // = Texture2D desc.MipLevels - 1
					shaderDesc.Texture2D.MipLevels = 1;       // = Texture2D desc.MipLevels

					hr2 = m_pDevice->CreateShaderResourceView(m_pTextureSubPic, &shaderDesc, &m_pShaderResourceSubPic);
					DLogIf(FAILED(hr2), L"CDX12VideoProcessor::InitSwapChain() : CreateShaderResourceView() failed with error {}", HR2Str(hr2));
				}

				if (m_pShaderResourceSubPic) {
					hr2 = m_pD3DDevEx->ColorFill(m_pSurface9SubPic, nullptr, D3DCOLOR_ARGB(255, 0, 0, 0));
					hr2 = m_pD3DDevEx->SetRenderTarget(0, m_pSurface9SubPic);
					DLogIf(FAILED(hr2), L"CDX12VideoProcessor::InitSwapChain() : SetRenderTarget(Direct3D9) failed with error {}", HR2Str(hr2));
				}
			}
		}
	}

	return hr;
#endif
}


BOOL CDX12VideoProcessor::VerifyMediaType(const CMediaType* pmt)
{
	return 0;
}

BOOL CDX12VideoProcessor::InitMediaType(const CMediaType* pmt)
{
	return 0;
}

void CDX12VideoProcessor::Configure(const Settings_t& config)
{
}

void CDX12VideoProcessor::SetRotation(int value)
{
}

void CDX12VideoProcessor::Flush()
{
}

void CDX12VideoProcessor::ClearPreScaleShaders()
{
}

void CDX12VideoProcessor::ClearPostScaleShaders()
{
}

HRESULT CDX12VideoProcessor::AddPreScaleShader(const std::wstring& name, const std::string& srcCode)
{
	return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::AddPostScaleShader(const std::wstring& name, const std::string& srcCode)
{
	return E_NOTIMPL;
}

bool CDX12VideoProcessor::Initialized()
{
  return false;
}

void CDX12VideoProcessor::ReleaseVP()
{
}

void CDX12VideoProcessor::ReleaseDevice()
{
	DLog(L"CDX11VideoProcessor::ReleaseDevice()");
}

void CDX12VideoProcessor::ReleaseSwapChain()
{
}

bool CDX12VideoProcessor::HandleHDRToggle()
{
	return false;
}

void CDX12VideoProcessor::SetGraphSize()
{
}

BOOL CDX12VideoProcessor::GetAlignmentSize(const CMediaType& mt, SIZE& Size)
{
    return 0;
}

HRESULT CDX12VideoProcessor::ProcessSample(IMediaSample* pSample)
{
    return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::Render(int field)
{
    return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::FillBlack()
{
    return E_NOTIMPL;
}

void CDX12VideoProcessor::SetVideoRect(const CRect& videoRect)
{
}

HRESULT CDX12VideoProcessor::SetWindowRect(const CRect& windowRect)
{
    return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::Reset()
{
    return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::GetCurentImage(long* pDIBImage)
{
    return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::GetDisplayedImage(BYTE** ppDib, unsigned* pSize)
{
  return E_NOTIMPL;
}

HRESULT CDX12VideoProcessor::GetVPInfo(std::wstring& str)
{
  return E_NOTIMPL;
}

void CDX12VideoProcessor::UpdateStatsPresent()
{
}

void CDX12VideoProcessor::UpdateStatsStatic()
{
}

HRESULT CDX12VideoProcessor::DrawStats(ID3D12Resource* pRenderTarget)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT __stdcall) CDX12VideoProcessor::SetProcAmpValues(DWORD dwFlags, DXVA2_ProcAmpValues* pValues)
{
  return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT __stdcall) CDX12VideoProcessor::SetAlphaBitmap(const MFVideoAlphaBitmap* pBmpParms)
{
  return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT __stdcall) CDX12VideoProcessor::UpdateAlphaBitmapParameters(const MFVideoAlphaBitmapParams* pBmpParms)
{
  return E_NOTIMPL;
}


void CDX12VideoProcessor::SetCallbackDevice(const bool bChangeDevice/* = false*/)
{
	if ((!m_bCallbackDeviceIsSet || bChangeDevice) && m_pD3DDevEx && m_pFilter->m_pSubCallBack) {
		m_bCallbackDeviceIsSet = SUCCEEDED(m_pFilter->m_pSubCallBack->SetDevice(m_pD3DDevEx));
	}
}