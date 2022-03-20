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
#include "d3dcompiler.h"
#include <iostream>
#include <fstream>
#include "dxgi1_6.h"



#include "d3d12util/BufferManager.h"
#include "d3d12util/CompiledShaders/ScreenQuadCommonVS.h"
#include "d3d12util/CompiledShaders/LanczosVerticalPS.h"
#include "d3d12util/CompiledShaders/LanczosHorizontalPS.h"
#include "d3d12util/CompiledShaders/BufferCopyPS.h"
#include "d3d12util/CompiledShaders/VideoQuadPresentVS.h"
#include "d3d12util/CompiledShaders/ScreenQuadPresentVS.h"
#include "d3d12util/CompiledShaders/PresentSDRPS.h"
#include "d3d12util/CompiledShaders/PresentHDRPS.h"
#include "d3d12util/CompiledShaders/CompositeSDRPS.h"
#include "d3d12util/CompiledShaders/ScaleAndCompositeSDRPS.h"
#include "d3d12util/CompiledShaders/CompositeHDRPS.h"
#include "d3d12util/CompiledShaders/BlendUIHDRPS.h"
#include "d3d12util/CompiledShaders/ScaleAndCompositeHDRPS.h"
#include "d3d12util/CompiledShaders/MagnifyPixelsPS.h"
#include "d3d12util/CompiledShaders/GenerateMipsLinearCS.h"
#include "d3d12util/CompiledShaders/GenerateMipsLinearOddCS.h"
#include "d3d12util/CompiledShaders/GenerateMipsLinearOddXCS.h"
#include "d3d12util/CompiledShaders/GenerateMipsLinearOddYCS.h"
#include "d3d12util/CompiledShaders/GenerateMipsGammaCS.h"
#include "d3d12util/CompiledShaders/GenerateMipsGammaOddCS.h"
#include "d3d12util/CompiledShaders/GenerateMipsGammaOddXCS.h"
#include "d3d12util/CompiledShaders/GenerateMipsGammaOddYCS.h"
#include "d3d12util/CompiledShaders/BilinearUpsamplePS.h"
#include "d3d12util/CompiledShaders/ColorConvertVLCNV12PS.h"
#include "d3d12util/TextRenderer.h"
#include "d3d12util/ImageScaling.h"
#include "d3d12util/EsramAllocator.h"
#include "d3d12util/math/Common.h"
#define USE_PIX
#include "pix3.h"


#include "../external/minhook/include/MinHook.h"

#include "DX12VideoProcessor.h"
#pragma comment( lib, "d3d12.lib" )
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
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


CDX12VideoProcessor::CDX12VideoProcessor(CMpcVideoRenderer* pFilter, const Settings_t& config, HRESULT& hr)
  : CVideoProcessor(pFilter)
{

	HMODULE mod = PIXLoadLatestWinPixGpuCapturerLibrary();
	PIXBeginCapture(0, nullptr);
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
	for (int xx = 0; xx < 3; xx++)
	{
		SwapChainBuffer[xx] = nullptr;

	}
}

CDX12VideoProcessor::~CDX12VideoProcessor()
{
	D3D12Public::g_CommandManager.IdleGPU();
	CommandContext::DestroyAllContexts();
	D3D12Public::g_CommandManager.Shutdown();
	
	PSO::DestroyAll();
	RootSignature::DestroyAll();
	DescriptorAllocator::DestroyAll();

	D3D12Public::DestroyCommonState();
	D3D12Public::DestroyRenderingBuffers();
	TextRenderer::Shutdown();
	
	
}



void CDX12VideoProcessor::GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr;

	CComPtr<IDXGIAdapter1> adapter;

	CComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				m_nCurrentAdapter = adapterIndex;
				break;
			}
		}
	}

	if (adapter == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = adapter.Detach();
}

HRESULT CDX12VideoProcessor::Init(const HWND hwnd, bool* pChangeDevice)
{

	if (!hwnd)
		return S_OK;

	if (m_hWnd != hwnd)
		m_hWnd = hwnd;
	D3D12Public::g_hWnd = hwnd;

	if (!D3D12Public::g_Device)
		return S_OK;
	
	if (D3D12Public::g_CommandManager.GetGraphicsQueue().IsReady())
		return S_OK;
	D3D12Public::g_CommandManager.Create(D3D12Public::g_Device);
	D3D12Public::InitializeCommonState();
	D3D12Public::InitializeRenderingBuffers(1280,528);
	TextRenderer::Initialize();
	Display::Initialize();

	m_VideoPSO = GraphicsPSO(L"Renderer: Default PSO"); // Not finalized.  Used as a template.
	BilinearUpsamplePS2 = GraphicsPSO(L"Image Scaling: Bilinear Upsample PSO");
																											//m_pScalingResource.Create(L"Scaling Resource", m_srcRect.Width(), m_srcRect.Height(), 1, DXGI_FORMAT_R10G10B10A2_UNORM);
	//ImageScaling::Initialize(m_pScalingResource.GetFormat());
	SamplerDesc VideoSamplerDesc[3];
	VideoSamplerDesc[0] = {};
	VideoSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	VideoSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	VideoSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	VideoSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	VideoSamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS; // NEVER
	VideoSamplerDesc[0].SetBorderColor({ 255.0f,255.0f ,1.0f ,1.0f });
	VideoSamplerDesc[0].MinLOD = 0;
	VideoSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	VideoSamplerDesc[1] = VideoSamplerDesc[0];
	VideoSamplerDesc[1].Filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
	VideoSamplerDesc[2] = VideoSamplerDesc[0];
	VideoSamplerDesc[2].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	VideoSamplerDesc[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	VideoSamplerDesc[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//add lienear and dither only have point right now

	//D3D12_DESCRIPTOR_RANGE_TYPE_SRV-> shader-resource views
	//D3D12_DESCRIPTOR_RANGE_TYPE_UAV is Specifies a range of unordered-access views (UAVs).
	//D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER is speciefies range of samplers
	//D3D12_DESCRIPTOR_RANGE_TYPE_CBV   Specifies a range of constant-buffer views (CBVs).

		//m_RootSig.Reset(4, 3);
		//m_RootSig.InitStaticSampler(0, VideoSamplerDesc[0]);
		//m_RootSig.InitStaticSampler(1, VideoSamplerDesc[1]);
		//m_RootSig.InitStaticSampler(2, VideoSamplerDesc[2]);


		m_RootSig.Reset(4, 2);
#if 1
		//m_RootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		m_RootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		m_RootSig[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
		m_RootSig[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
		m_RootSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
		//this one is ok its the texture for the shader resource
		
		

		
#else
		m_RootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		m_RootSig[1].InitAsConstants(0, 32, D3D12_SHADER_VISIBILITY_ALL);
		m_RootSig[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
		m_RootSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
#endif
		m_RootSig.InitStaticSampler(0, VideoSamplerDesc[0]);
		m_RootSig.InitStaticSampler(1, VideoSamplerDesc[1]);
		m_RootSig.Finalize(L"Present", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		static D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,     0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,     0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
				
		};

		m_VideoPSO.SetRootSignature(m_RootSig);
		m_VideoPSO.SetRasterizerState(D3D12Public::RasterizerTwoSided);
		m_VideoPSO.SetBlendState(D3D12Public::BlendDisable);
		m_VideoPSO.SetDepthStencilState(D3D12Public::DepthStateDisabled);
		m_VideoPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		m_VideoPSO.SetSampleMask(UINT_MAX);
		m_VideoPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_VideoPSO.SetVertexShader(g_pVideoQuadPresentVS, sizeof(g_pVideoQuadPresentVS));
		m_VideoPSO.SetPixelShader(g_pColorConvertVLCNV12PS, sizeof(g_pColorConvertVLCNV12PS));
		m_VideoPSO.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);
		
		m_VideoPSO.Finalize();
#if 0
		BilinearUpsamplePS2.SetRootSignature(m_RootSig);
		BilinearUpsamplePS2.SetRasterizerState(D3D12Public::RasterizerTwoSided);
		BilinearUpsamplePS2.SetBlendState(BlendDisable);
		BilinearUpsamplePS2.SetDepthStencilState(D3D12Public::DepthStateDisabled);
		BilinearUpsamplePS2.SetSampleMask(0xFFFFFFFF);
		BilinearUpsamplePS2.SetInputLayout(0, nullptr);
		BilinearUpsamplePS2.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		BilinearUpsamplePS2.SetVertexShader(g_pScreenQuadPresentVS, sizeof(g_pScreenQuadPresentVS));
		BilinearUpsamplePS2.SetPixelShader(g_pBilinearUpsamplePS, sizeof(g_pBilinearUpsamplePS));
		BilinearUpsamplePS2.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);
		BilinearUpsamplePS2.Finalize();
#endif
		//m_RootSig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
		//m_RootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		//m_RootSig.Finalize(L"RootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);




		//sizeof(bdesc.RenderTarget)
		//
		/*m_VideoPSO.SetRootSignature(m_RootSig);
		m_VideoPSO.SetRasterizerState(D3D12Public::RasterizerDefault);
		m_VideoPSO.SetBlendState(D3D12Public::BlendPreMultiplied);
		m_VideoPSO.SetDepthStencilState(D3D12Public::DepthStateDisabled);
		m_VideoPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		m_VideoPSO.SetSampleMask(UINT_MAX);
		m_VideoPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_VideoPSO.SetVertexShader(g_pVideoQuadPresentVS, sizeof(g_pVideoQuadPresentVS));
		m_VideoPSO.SetPixelShader(g_pBufferCopyPS, sizeof(g_pBufferCopyPS));*/


		/*DXGI_FORMAT SwapChainFormats[2] = {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM};
		m_VideoPSO.SetRenderTargetFormats(2, SwapChainFormats, DXGI_FORMAT_UNKNOWN);
		m_VideoPSO.Finalize();*/



		for (int i = 0; i < 64; i++)
		{
			m_pTemporaryTexture.push_back(TemporaryTexture());
		}


		//SetupQuad();

		//m_pViewpointShaderConstant;
		//m_pPixelShaderConstants;
		SetupQuadVertices();
		SetShaderConvertColorParams();

		return S_OK;
	
}

void CDX12VideoProcessor::UpdateQuad()
{
	
	//result = D3D_SetupQuadData(o, &quad->generic, output, pVertexDataBegin, triangleVertices, orientation);
//vertex buffer data pVertexDataBegin dst_data pData
//index buffer data triangleVertices
/*create index buffer and quad vertex buffer*/
	
	VideoQuadVertex vertex_data[4];
	WORD triangleVertices[6];

	D3D12_RANGE readRange = { 0 };
	HRESULT hr = m_pVertexBuffer->Map(0, &readRange, (void**)&vertex_data);
	if (FAILED(hr))
		assert(0);
	hr = m_pIndexBuffer->Map(0, &readRange, (void**)&triangleVertices);
	if (FAILED(hr))
		assert(0);

	//vertex_data = (VideoQuadVertex*)malloc(sizeof(VideoQuadVertex[4]));
	//triangleVertices = (WORD*)malloc(sizeof(triangleVertices[6]));
	unsigned int src_width = m_srcRect.Width();
	unsigned int src_height = m_srcRect.Height();
	float MidX, MidY;

	float top, bottom, left, right;
	/* find the middle of the visible part of the texture, it will be a 0,0
	 * the rest of the visible area must correspond to -1,1 */

	 /* left/top aligned */
	MidY = (m_windowRect.top + m_windowRect.bottom) / 2.f;
	MidX = (m_windowRect.left + m_windowRect.right) / 2.f;
	top = MidY / (MidY - m_windowRect.top);
	bottom = -(src_height - MidY) / (m_windowRect.bottom - MidY);
	left = -MidX / (MidX - m_windowRect.left);
	right = (src_width - MidX) / (m_windowRect.right - MidX);



	const float vertices_coords[4][2] = {
			{ left,  bottom },
			{ right, bottom },
			{ right, top    },
			{ left,  top    },
	};
	int vertex_order[4];
	vertex_order[0] = 0;
	vertex_order[1] = 1;
	vertex_order[2] = 2;
	vertex_order[3] = 3;


	for (int i = 0; i < 4; ++i) {
		vertex_data[i].position.x = vertices_coords[vertex_order[i]][0];
		vertex_data[i].position.y = vertices_coords[vertex_order[i]][1];
	}

	// bottom left
	vertex_data[0].position.z = 0.0f;
	vertex_data[0].texture.u = 0.0f;
	vertex_data[0].texture.v = 1.0f;

	// bottom right
	vertex_data[1].position.z = 0.0f;
	vertex_data[1].texture.u = 1.0f;
	vertex_data[1].texture.v = 1.0f;

	// top right
	vertex_data[2].position.z = 0.0f;
	vertex_data[2].texture.u = 1.0f;
	vertex_data[2].texture.v = 0.0f;

	// top left
	vertex_data[3].position.z = 0.0f;
	vertex_data[3].texture.u = 0.0f;
	vertex_data[3].texture.v = 0.0f;

	/* Make sure surfaces are facing the right way */

	triangleVertices[0] = (WORD)3;
	triangleVertices[1] = (WORD)1;
	triangleVertices[2] = (WORD)0;

	triangleVertices[3] = (WORD)2;
	triangleVertices[4] = (WORD)1;
	triangleVertices[5] = (WORD)3;
	m_pVertexBuffer->Unmap(0, NULL);
	m_pIndexBuffer->Unmap(0, NULL);
	resetquad = false;
	//buffer need to be 16byte aligned which is too small for this vertice
	///void* stackMem = _malloca((sizeof(WORD) + 1) * 16);
	//WORD* stackptr = Math::AlignUp((WORD*)stackMem, 16);
	//stackMem = (void*)triangleVertices;

	//_freea(stackMem);

	
}
void CDX12VideoProcessor::SetupQuadVertices()
{

	
	//VideoQuadVertex = stride
	//vertexcount = 4 on rectangular
	
	m_pVertexBuffer.Create(L"QuadVertex", sizeof(VideoQuadVertex) * 4);//in the good state too start
	m_pVertexBufferView.BufferLocation = m_pVertexBuffer.GetGpuVirtualAddress();
	m_pVertexBufferView.SizeInBytes = m_pVertexBuffer.GetBufferSize();
	m_pVertexBufferView.StrideInBytes = sizeof(VideoQuadVertex);
	//indexcount = 6

	m_pIndexBuffer.Create(L"IndexBuffer", sizeof(WORD)*6);
	m_pIndexBufferView.BufferLocation = m_pIndexBuffer.GetGpuVirtualAddress();
	m_pIndexBufferView.SizeInBytes = m_pIndexBuffer.GetBufferSize();
	m_pIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	
	
	
	




	
	UpdateQuad();
	resetquad = false;
}
static void MultMat43(FLOAT dst[4 * 3], const FLOAT left[4 * 3], const FLOAT right[4 * 3])
{
	// Cache the invariants in registers
	FLOAT x = left[0 * 4 + 0];
	FLOAT y = left[0 * 4 + 1];
	FLOAT z = left[0 * 4 + 2];
	FLOAT w = left[0 * 4 + 3];
	// Perform the operation on the first row
	dst[0 * 4 + 0] = (right[0 * 4 + 0] * x) + (right[1 * 4 + 0] * y) + (right[2 * 4 + 0] * z) + (right[3 * 4 + 0] * w);
	dst[0 * 4 + 1] = (right[0 * 4 + 1] * x) + (right[1 * 4 + 1] * y) + (right[2 * 4 + 1] * z) + (right[3 * 4 + 1] * w);
	dst[0 * 4 + 2] = (right[0 * 4 + 2] * x) + (right[1 * 4 + 2] * y) + (right[2 * 4 + 2] * z) + (right[3 * 4 + 2] * w);
	dst[0 * 4 + 3] = (right[0 * 4 + 3] * x) + (right[1 * 4 + 3] * y) + (right[2 * 4 + 3] * z) + (right[3 * 4 + 3] * w);
	// Repeat for all the other rows
	x = left[1 * 4 + 0];
	y = left[1 * 4 + 1];
	z = left[1 * 4 + 2];
	w = left[1 * 4 + 3];
	dst[1 * 4 + 0] = (right[0 * 4 + 0] * x) + (right[1 * 4 + 0] * y) + (right[2 * 4 + 0] * z) + (right[3 * 4 + 0] * w);
	dst[1 * 4 + 1] = (right[0 * 4 + 1] * x) + (right[1 * 4 + 1] * y) + (right[2 * 4 + 1] * z) + (right[3 * 4 + 1] * w);
	dst[1 * 4 + 2] = (right[0 * 4 + 2] * x) + (right[1 * 4 + 2] * y) + (right[2 * 4 + 2] * z) + (right[3 * 4 + 2] * w);
	dst[1 * 4 + 3] = (right[0 * 4 + 3] * x) + (right[1 * 4 + 3] * y) + (right[2 * 4 + 3] * z) + (right[3 * 4 + 3] * w);
	x = left[2 * 4 + 0];
	y = left[2 * 4 + 1];
	z = left[2 * 4 + 2];
	w = left[2 * 4 + 3];
	dst[2 * 4 + 0] = (right[0 * 4 + 0] * x) + (right[1 * 4 + 0] * y) + (right[2 * 4 + 0] * z) + (right[3 * 4 + 0] * w);
	dst[2 * 4 + 1] = (right[0 * 4 + 1] * x) + (right[1 * 4 + 1] * y) + (right[2 * 4 + 1] * z) + (right[3 * 4 + 1] * w);
	dst[2 * 4 + 2] = (right[0 * 4 + 2] * x) + (right[1 * 4 + 2] * y) + (right[2 * 4 + 2] * z) + (right[3 * 4 + 2] * w);
	dst[2 * 4 + 3] = (right[0 * 4 + 3] * x) + (right[1 * 4 + 3] * y) + (right[2 * 4 + 3] * z) + (right[3 * 4 + 3] * w);
}

static const FLOAT COLORSPACE_BT709_YUV_TO_FULL_RGBA[4 * 3] = {
				1.164383561643836f,                 0.f,  1.792741071428571f, 0.f,
				1.164383561643836f, -0.213248614273730f, -0.532909328559444f, 0.f,
				1.164383561643836f,  2.112401785714286f,                 0.f, 0.f,
};

static const FLOAT IDENTITY_4X3[4 * 3] = {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
};

void CDX12VideoProcessor::SetupConstantBuffers(D3D12_CPU_DESCRIPTOR_HANDLE bufferviewheaphandle)
{
	if(m_pPixelShaderConstants.GetGpuVirtualAddress() == D3D12_GPU_VIRTUAL_ADDRESS_NULL)
		m_pPixelShaderConstants.Create(L"Viewpoint Shader Constant", sizeof(*m_sShaderConstants));
	D3D12_RANGE readRange = { 0 }; // no reading

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = m_pPixelShaderConstants.GetGpuVirtualAddress();
	cbvDesc.SizeInBytes = m_pPixelShaderConstants.GetBufferSize();

	D3D12Public::g_Device->CreateConstantBufferView(&cbvDesc, bufferviewheaphandle);

	//its ok to leave it on map
	m_pPixelShaderConstants->Map(0, &readRange, (void**)&m_sShaderConstants);
	m_sShaderConstants->BoundaryX = 1.0;
	m_sShaderConstants->BoundaryY = 1.0;
	m_sShaderConstants->Opacity = 1.0;
	m_sShaderConstants->LuminanceScale = 1.0;
	FLOAT WhitePoint[4 * 3];
	memcpy(WhitePoint, IDENTITY_4X3, sizeof(WhitePoint));
	WhitePoint[0 * 4 + 3] = -0.f;
	WhitePoint[1 * 4 + 3] = -0.f;
	WhitePoint[2 * 4 + 3] = -0.f;

	MultMat43(m_sShaderConstants->Colorspace, COLORSPACE_BT709_YUV_TO_FULL_RGBA, WhitePoint);
}

HRESULT CDX12VideoProcessor::SetDevice(ID3D12Device* pDevice, const bool bDecoderDevice)
{
	D3D12Public::g_Device = pDevice;
	Init(m_hWnd, false);
	return S_OK;
}

BOOL CDX12VideoProcessor::VerifyMediaType(const CMediaType* pmt)
{
	const auto& FmtParams = GetFmtConvParams(pmt);
	if (FmtParams.VP11Format == DXGI_FORMAT_UNKNOWN && FmtParams.DX11Format == DXGI_FORMAT_UNKNOWN) {
		return FALSE;
	}

	const BITMAPINFOHEADER* pBIH = GetBIHfromVIHs(pmt);
	if (!pBIH) {
		return FALSE;
	}

	if (pBIH->biWidth <= 0 || !pBIH->biHeight || (!pBIH->biSizeImage && pBIH->biCompression != BI_RGB)) {
		return FALSE;
	}

	if (FmtParams.Subsampling == 420 && ((pBIH->biWidth & 1) || (pBIH->biHeight & 1))) {
		return FALSE;
	}
	if (FmtParams.Subsampling == 422 && (pBIH->biWidth & 1)) {
		return FALSE;
	}

	return TRUE;
}

BOOL CDX12VideoProcessor::InitMediaType(const CMediaType* pmt)
{
	DLog(L"CDX12VideoProcessor::InitMediaType()");

	if (!VerifyMediaType(pmt)) {
		return FALSE;
	}

	ReleaseVP();

	auto FmtParams = GetFmtConvParams(pmt);
	m_srcParams = FmtParams;
	bool disableD3D11VP = false;
	switch (FmtParams.cformat) {
	case CF_NV12: disableD3D11VP = !m_VPFormats.bNV12; break;
	case CF_P010:
	case CF_P016: disableD3D11VP = !m_VPFormats.bP01x;  break;
	case CF_YUY2: disableD3D11VP = !m_VPFormats.bYUY2;  break;
	default:      disableD3D11VP = !m_VPFormats.bOther; break;
	}
	if (disableD3D11VP) {
		FmtParams.VP11Format = DXGI_FORMAT_UNKNOWN;
	}

	const BITMAPINFOHEADER* pBIH = nullptr;
	m_decExFmt.value = 0;

	if (pmt->formattype == FORMAT_VideoInfo2) {
		const VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pmt->pbFormat;
		pBIH = &vih2->bmiHeader;
		m_srcRect = vih2->rcSource;
		m_srcAspectRatioX = vih2->dwPictAspectRatioX;
		m_srcAspectRatioY = vih2->dwPictAspectRatioY;
		if (FmtParams.CSType == CS_YUV && (vih2->dwControlFlags & (AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT))) {
			m_decExFmt.value = vih2->dwControlFlags;
			m_decExFmt.SampleFormat = AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT; // ignore other flags
		}
		m_bInterlaced = (vih2->dwInterlaceFlags & AMINTERLACE_IsInterlaced);
		m_rtAvgTimePerFrame = vih2->AvgTimePerFrame;
	}
	else if (pmt->formattype == FORMAT_VideoInfo) {
		const VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
		pBIH = &vih->bmiHeader;
		m_srcRect = vih->rcSource;
		m_srcAspectRatioX = 0;
		m_srcAspectRatioY = 0;
		m_bInterlaced = 0;
		m_rtAvgTimePerFrame = vih->AvgTimePerFrame;
	}
	else {
		return FALSE;
	}

	m_pFilter->m_FrameStats.SetStartFrameDuration(m_rtAvgTimePerFrame);

	UINT biWidth = pBIH->biWidth;
	UINT biHeight = labs(pBIH->biHeight);
	UINT biSizeImage = pBIH->biSizeImage;
	if (pBIH->biSizeImage == 0 && pBIH->biCompression == BI_RGB) { // biSizeImage may be zero for BI_RGB bitmaps
		biSizeImage = biWidth * biHeight * pBIH->biBitCount / 8;
	}

	m_srcLines = biHeight * FmtParams.PitchCoeff / 2;
	m_srcPitch = biWidth * FmtParams.Packsize;
	switch (FmtParams.cformat) {
	case CF_Y8:
	case CF_NV12:
	case CF_RGB24:
	case CF_BGR48:
		m_srcPitch = ALIGN(m_srcPitch, 4);
		break;
	}
	if (pBIH->biCompression == BI_RGB && pBIH->biHeight > 0) {
		m_srcPitch = -m_srcPitch;
	}

	UINT origW = biWidth;
	UINT origH = biHeight;
	if (pmt->FormatLength() == 112 + sizeof(VR_Extradata)) {
		const VR_Extradata* vrextra = reinterpret_cast<VR_Extradata*>(pmt->pbFormat + 112);
		if (vrextra->QueryWidth == pBIH->biWidth && vrextra->QueryHeight == pBIH->biHeight && vrextra->Compression == pBIH->biCompression) {
			origW = vrextra->FrameWidth;
			origH = abs(vrextra->FrameHeight);
		}
	}

	if (m_srcRect.IsRectNull()) {
		m_srcRect.SetRect(0, 0, origW, origH);
	}
	m_srcRectWidth = m_srcRect.Width();
	m_srcRectHeight = m_srcRect.Height();

	m_srcExFmt = SpecifyExtendedFormat(m_decExFmt, FmtParams, m_srcRectWidth, m_srcRectHeight);

	const auto frm_gcd = std::gcd(m_srcRectWidth, m_srcRectHeight);
	const auto srcFrameARX = m_srcRectWidth / frm_gcd;
	const auto srcFrameARY = m_srcRectHeight / frm_gcd;

	if (!m_srcAspectRatioX || !m_srcAspectRatioY) {
		m_srcAspectRatioX = srcFrameARX;
		m_srcAspectRatioY = srcFrameARY;
		m_srcAnamorphic = false;
	}
	else {
		const auto ar_gcd = std::gcd(m_srcAspectRatioX, m_srcAspectRatioY);
		m_srcAspectRatioX /= ar_gcd;
		m_srcAspectRatioY /= ar_gcd;
		m_srcAnamorphic = (srcFrameARX != m_srcAspectRatioX || srcFrameARY != m_srcAspectRatioY);
	}
#if 0 
	UpdateUpscalingShaders();
	UpdateDownscalingShaders();

	m_pPSCorrection.Release();
	m_pPSConvertColor.Release();
	m_PSConvColorData.bEnable = false;

	UpdateTexParams(FmtParams.CDepth);
#endif
	if (m_bHdrAllowSwitchDisplay && m_srcVideoTransferFunction != m_srcExFmt.VideoTransferFunction) {
		auto ret = HandleHDRToggle();
		if (!ret && (m_bHdrPassthrough && m_bHdrPassthroughSupport && SourceIsHDR() && !m_pDXGISwapChain4)) {
			ret = true;
		}
		if (ret) {
			ReleaseSwapChain();
			Init(m_hWnd);
		}
	}

	if (Preferred10BitOutput() && m_SwapChainFmt == DXGI_FORMAT_B8G8R8A8_UNORM) {
		ReleaseSwapChain();
		Init(m_hWnd);
	}


	m_srcVideoTransferFunction = m_srcExFmt.VideoTransferFunction;
	
#ifdef TODO
	// D3D11 Video Processor
	if (FmtParams.VP11Format != DXGI_FORMAT_UNKNOWN && !(m_VendorId == PCIV_NVIDIA && FmtParams.CSType == CS_RGB)) {
		// D3D11 VP does not work correctly if RGB32 with odd frame width (source or target) on Nvidia adapters

		if (S_OK == InitializeD3D11VP(FmtParams, origW, origH)) {
			bool bTransFunc22 = m_srcExFmt.VideoTransferFunction == DXVA2_VideoTransFunc_22
				|| m_srcExFmt.VideoTransferFunction == DXVA2_VideoTransFunc_709
				|| m_srcExFmt.VideoTransferFunction == DXVA2_VideoTransFunc_240M;

			if (m_srcExFmt.VideoTransferFunction == VIDEOTRANSFUNC_2084 && !(m_bHdrPassthroughSupport && m_bHdrPassthrough) && m_bConvertToSdr) {
				EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_CONVERT_PQ_TO_SDR));
				m_strCorrection = L"PQ to SDR";
			}
			else if (m_srcExFmt.VideoTransferFunction == VIDEOTRANSFUNC_HLG) {
				if (m_bHdrPassthroughSupport && m_bHdrPassthrough) {
					EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_CONVERT_HLG_TO_PQ));
					m_strCorrection = L"HLG to PQ";
				}
				else if (m_bConvertToSdr) {
					EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_CONVERT_HLG_TO_SDR));
					m_strCorrection = L"HLG to SDR";
				}
				else if (m_srcExFmt.VideoPrimaries == VIDEOPRIMARIES_BT2020) {
					// HLG compatible with SDR
					EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_FIX_BT2020));
					m_strCorrection = L"Fix BT.2020";
				}
			}
			else if (m_srcExFmt.VideoTransferMatrix == VIDEOTRANSFERMATRIX_YCgCo) {
				EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_FIX_YCGCO));
				m_strCorrection = L"Fix YCoCg";
			}
			else if (bTransFunc22 && m_srcExFmt.VideoPrimaries == VIDEOPRIMARIES_BT2020) {
				EXECUTE_ASSERT(S_OK == CreatePShaderFromResource(&m_pPSCorrection, IDF_PSH11_FIX_BT2020));
				m_strCorrection = L"Fix BT.2020";
			}

			DLogIf(m_pPSCorrection, L"CDX11VideoProcessor::InitMediaType() m_pPSCorrection created");

			m_pFilter->m_inputMT = *pmt;
			UpdateTexures();
			UpdatePostScaleTexures();
			UpdateStatsStatic();
			SetCallbackDevice();

			return TRUE;
		}

		ReleaseVP();
	}

	// Tex Video Processor
	if (FmtParams.DX11Format != DXGI_FORMAT_UNKNOWN && S_OK == InitializeTexVP(FmtParams, origW, origH)) {
		m_pFilter->m_inputMT = *pmt;
		SetShaderConvertColorParams();
		UpdateTexures();
		UpdatePostScaleTexures();
		UpdateStatsStatic();
		SetCallbackDevice();

		return TRUE;
	}

	return FALSE;
#endif
	return TRUE;
}

void CDX12VideoProcessor::Configure(const Settings_t& config)
{
	bool reloadPipeline = false;
	bool changeWindow = false;
	bool changeDevice = false;
	bool changeVP = false;
	bool changeHDR = false;
	bool changeTextures = false;
	bool changeConvertShader = false;
	bool changeUpscalingShader = false;
	bool changeDowndcalingShader = false;
	bool changeNumTextures = false;
	bool changeResizeStats = false;

	// settings that do not require preparation
	m_bShowStats = config.bShowStats;
	m_bDeintDouble = config.bDeintDouble;
	m_bInterpolateAt50pct = config.bInterpolateAt50pct;
	m_bVBlankBeforePresent = config.bVBlankBeforePresent;

	// checking what needs to be changed

	if (config.iResizeStats != m_iResizeStats) {
		m_iResizeStats = config.iResizeStats;
		changeResizeStats = true;
	}

	if (config.iTexFormat != m_iTexFormat) {
		m_iTexFormat = config.iTexFormat;
		changeTextures = true;
	}

	if (m_srcParams.cformat == CF_NV12) {
		changeVP = config.VPFmts.bNV12 != m_VPFormats.bNV12;
	}
	else if (m_srcParams.cformat == CF_P010 || m_srcParams.cformat == CF_P016) {
		changeVP = config.VPFmts.bP01x != m_VPFormats.bP01x;
	}
	else if (m_srcParams.cformat == CF_YUY2) {
		changeVP = config.VPFmts.bYUY2 != m_VPFormats.bYUY2;
	}
	else {
		changeVP = config.VPFmts.bOther != m_VPFormats.bOther;
	}
	m_VPFormats = config.VPFmts;

	if (config.bVPScaling != m_bVPScaling) {
		m_bVPScaling = config.bVPScaling;
		changeTextures = true;
		changeVP = true; // temporary solution
	}
#ifdef TODO
	if (config.iChromaScaling != m_iChromaScaling) {
		m_iChromaScaling = config.iChromaScaling;
		changeConvertShader = m_PSConvColorData.bEnable && (m_srcParams.Subsampling == 420 || m_srcParams.Subsampling == 422);
	}
#endif
	if (config.iUpscaling != m_iUpscaling) {
		m_iUpscaling = config.iUpscaling;
		changeUpscalingShader = true;
	}
	if (config.iDownscaling != m_iDownscaling) {
		m_iDownscaling = config.iDownscaling;
		changeDowndcalingShader = true;
	}

	if (config.bUseDither != m_bUseDither) {
		m_bUseDither = config.bUseDither;
		changeNumTextures = m_InternalTexFmt != DXGI_FORMAT_B8G8R8A8_UNORM;
	}

	if (config.iSwapEffect != m_iSwapEffect) {
		m_iSwapEffect = config.iSwapEffect;
		changeWindow = !m_pFilter->m_bIsFullscreen;
	}

	if (config.bHdrPassthrough != m_bHdrPassthrough) {
		m_bHdrPassthrough = config.bHdrPassthrough;
		changeHDR = true;
	}

	if (config.iHdrToggleDisplay != m_iHdrToggleDisplay) {
		if (config.iHdrToggleDisplay == HDRTD_Off || m_iHdrToggleDisplay == HDRTD_Off) {
			changeHDR = true;
		}
		m_iHdrToggleDisplay = config.iHdrToggleDisplay;
	}
#ifdef TODO
	if (config.bConvertToSdr != m_bConvertToSdr) {
		m_bConvertToSdr = config.bConvertToSdr;
		if (SourceIsHDR()) {
			if (m_D3D11VP.IsReady()) {
				changeNumTextures = true;
				changeVP = true; // temporary solution
			}
			else {
				changeConvertShader = true;
			}
		}
	}
#endif
	// apply new settings

	if (changeWindow) {
		ReleaseSwapChain();
		EXECUTE_ASSERT(S_OK == m_pFilter->Init(true));

		if (changeHDR && (SourceIsHDR()) || m_iHdrToggleDisplay) {
			m_srcVideoTransferFunction = 0;
			InitMediaType(&m_pFilter->m_inputMT);
		}
		return;
	}

	if (changeHDR) {
		if (SourceIsHDR() || m_iHdrToggleDisplay) {
			if (m_iSwapEffect == SWAPEFFECT_Discard) {
				ReleaseSwapChain();
				m_pFilter->Init(true);
			}

			m_srcVideoTransferFunction = 0;
			InitMediaType(&m_pFilter->m_inputMT);

			return;
		}
	}

	if (changeVP) {
		InitMediaType(&m_pFilter->m_inputMT);
		m_pFilter->m_bValidBuffer = false;
		return; // need some test
	}
#ifdef TODO
	if (changeTextures) {
		UpdateTexParams(m_srcParams.CDepth);
		if (m_D3D11VP.IsReady()) {
			// update m_D3D11OutputFmt
			EXECUTE_ASSERT(S_OK == InitializeD3D11VP(m_srcParams, m_srcWidth, m_srcHeight));
		}
		UpdateTexures();
		UpdatePostScaleTexures();
	}
#endif
	if (changeConvertShader) {
		reloadPipeline = true;
		//UpdateConvertColorShader();
	}

	if (changeUpscalingShader) {
		reloadPipeline = true;//UpdateUpscalingShaders();
	}
	if (changeDowndcalingShader) {
		reloadPipeline = true;//UpdateDownscalingShaders();
	}

	if (changeNumTextures) {
		reloadPipeline = true;// UpdatePostScaleTexures();
	}

	if (changeResizeStats) {
		SetGraphSize();
	}

	UpdateStatsStatic();
	return;
}

void CDX12VideoProcessor::SetRotation(int value)
{
}

void CDX12VideoProcessor::Flush()
{
	return;
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
	//TODO
	if (InitMediaType(&mt)) {
		const auto& FmtParams = GetFmtConvParams(&mt);

		if (FmtParams.cformat == CF_RGB24) {
			Size.cx = ALIGN(Size.cx, 4);
		}
		else if (FmtParams.cformat == CF_RGB48 || FmtParams.cformat == CF_BGR48) {
			Size.cx = ALIGN(Size.cx, 2);
		}
		else if (FmtParams.cformat == CF_BGRA64 || FmtParams.cformat == CF_B64A) {
			// nothing
		}
		else 
		{
			//if (!m_TexSrcVideo.pTexture) {
			//	return FALSE;
			//}

			UINT RowPitch = 0;
			D3D11_MAPPED_SUBRESOURCE mappedResource = {};
			//if (SUCCEEDED(m_pDeviceContext->Map(m_TexSrcVideo.pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			//	RowPitch = mappedResource.RowPitch;
			//	m_pDeviceContext->Unmap(m_TexSrcVideo.pTexture, 0);
			//}

			//if (!RowPitch) {
			//	return FALSE;
			//}
			
			Size.cx = RowPitch / FmtParams.Packsize;
			//temporary
			Size.cx = m_srcRect.right - m_srcRect.left;
		}

		if (FmtParams.cformat == CF_RGB24 || FmtParams.cformat == CF_XRGB32 || FmtParams.cformat == CF_ARGB32) {
			Size.cy = -abs(Size.cy); // only for biCompression == BI_RGB
		}
		else {
			Size.cy = abs(Size.cy);
		}

		return TRUE;

	}

	return FALSE;
}

void CDX12VideoProcessor::SetShaderConvertColorParams()
{
	mp_csp_params csp_params;
	set_colorspace(m_srcExFmt, csp_params.color);
	csp_params.brightness = DXVA2FixedToFloat(m_DXVA2ProcAmpValues.Brightness) / 255;
	csp_params.contrast = DXVA2FixedToFloat(m_DXVA2ProcAmpValues.Contrast);
	csp_params.hue = DXVA2FixedToFloat(m_DXVA2ProcAmpValues.Hue) / 180 * acos(-1);
	csp_params.saturation = DXVA2FixedToFloat(m_DXVA2ProcAmpValues.Saturation);
	csp_params.gray = m_srcParams.CSType == CS_GRAY;

	csp_params.input_bits = csp_params.texture_bits = m_srcParams.CDepth;

	m_PSConvColorData =
		m_srcParams.CSType == CS_YUV ||
		m_srcParams.cformat == CF_GBRP8 || m_srcParams.cformat == CF_GBRP16 ||
		csp_params.gray ||
		fabs(csp_params.brightness) > 1e-4f || fabs(csp_params.contrast - 1.0f) > 1e-4f;

	mp_cmat cmatrix;
	mp_get_csp_matrix(&csp_params, &cmatrix);
	m_pBufferVar = {
		{cmatrix.m[0][0], cmatrix.m[0][1], cmatrix.m[0][2], 0},
		{cmatrix.m[1][0], cmatrix.m[1][1], cmatrix.m[1][2], 0},
		{cmatrix.m[2][0], cmatrix.m[2][1], cmatrix.m[2][2], 0},
		{cmatrix.c[0],    cmatrix.c[1],    cmatrix.c[2],    0},
	};

	if (m_srcParams.cformat == CF_Y410 || m_srcParams.cformat == CF_Y416) {
		std::swap(m_pBufferVar.cm_r.x, m_pBufferVar.cm_r.y);
		std::swap(m_pBufferVar.cm_g.x, m_pBufferVar.cm_g.y);
		std::swap(m_pBufferVar.cm_b.x, m_pBufferVar.cm_b.y);
	}
	else if (m_srcParams.cformat == CF_GBRP8 || m_srcParams.cformat == CF_GBRP16) {
		std::swap(m_pBufferVar.cm_r.x, m_pBufferVar.cm_r.y); std::swap(m_pBufferVar.cm_r.y, m_pBufferVar.cm_r.z);
		std::swap(m_pBufferVar.cm_g.x, m_pBufferVar.cm_g.y); std::swap(m_pBufferVar.cm_g.y, m_pBufferVar.cm_g.z);
		std::swap(m_pBufferVar.cm_b.x, m_pBufferVar.cm_b.y); std::swap(m_pBufferVar.cm_b.y, m_pBufferVar.cm_b.z);
	}
	else if (csp_params.gray) {
		m_pBufferVar.cm_g.x = m_pBufferVar.cm_g.y;
		m_pBufferVar.cm_g.y = 0;
		m_pBufferVar.cm_b.x = m_pBufferVar.cm_b.z;
		m_pBufferVar.cm_b.z = 0;
	}

}

HRESULT CDX12VideoProcessor::ProcessSample(IMediaSample* pSample)
{

	DXGI_SWAP_CHAIN_DESC desc2;
	if (!m_pDXGISwapChain1)
		Reset();
	m_pDXGISwapChain1->GetDesc(&desc2);
	if (desc2.OutputWindow != m_hWnd)
		Reset();
	if (D3D12Public::g_hWnd != m_hWnd)
		assert(0);
	
	BYTE* data = nullptr;
	HRESULT hr = S_OK;
	const long size = pSample->GetActualDataLength();
	REFERENCE_TIME rtStart, rtEnd;
	if (FAILED(pSample->GetTime(&rtStart, &rtEnd)))
	{
		rtStart = m_pFilter->m_FrameStats.GeTimestamp();
	}
	const REFERENCE_TIME rtFrameDur = m_pFilter->m_FrameStats.GetAverageFrameDuration();
	rtEnd = rtStart + rtFrameDur;

	m_rtStart = rtStart;
	CRefTime rtClock(rtStart);
	D3D12_RESOURCE_DESC desc = {};
	CComQIPtr<ID3D12Resource> pD3D12Resource;
	int textureIndex = 0;
	if (CComQIPtr<IMediaSampleD3D12> pMSD3D12 = pSample)
	{
		hr = pMSD3D12->GetD3D12Texture(&pD3D12Resource,&textureIndex);
		desc = pD3D12Resource->GetDesc();
	}
	
	if (m_pTemporaryTexture[textureIndex].GetIndex() == -1)
	{
		m_pTemporaryTexture[textureIndex].SetTexture(pD3D12Resource, textureIndex);
		m_pTemporaryTexture[textureIndex].InitHeap();
		m_pTemporaryTexture[textureIndex].CreateShaderRessource();
	}
	
#if 0 
	TemporaryTexture newtemptexture;
	for (TemporaryTexture n : m_pTemporaryTexture)
	{
		if (n.GetIndex() == textureIndex)
		{
			newtemptexture = n;
			break;
		}
	}
	if (newtemptexture.GetIndex() == -1)
	{
		newtemptexture.SetTexture(pD3D12Resource, textureIndex);
		
		newtemptexture.InitHeap();
		
	}
#endif
	
	
	//SetupConstantBuffers(m_pTemporaryTexture[textureIndex].GetContantBufferViewHeap(0));
	
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2];
	UINT64 pitch_plane[2];
	UINT rows_plane[2];
	UINT64 RequiredSize;
	D3D12Public::g_Device->GetCopyableFootprints(&desc,
		0, 2, 0, layoutplane, rows_plane, pitch_plane, &RequiredSize);
	if (desc.Format != DXGI_FORMAT_P010)// DXGI_FORMAT_NV12
	{
		m_pScalingResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, DXGI_FORMAT_R8_UNORM);
		m_pScalingResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, DXGI_FORMAT_R8G8_UNORM);
	}
	else
	{
		m_pScalingResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, DXGI_FORMAT_R16_UNORM);
		m_pScalingResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, DXGI_FORMAT_R16G16_UNORM);
	}
	
	GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Render Video");
	
	D3D12_TEXTURE_COPY_LOCATION dst;
	D3D12_TEXTURE_COPY_LOCATION src;
	for (int i = 0; i < 2; i++) {
		dst = CD3DX12_TEXTURE_COPY_LOCATION(m_pScalingResource[i].GetResource());
		src = CD3DX12_TEXTURE_COPY_LOCATION(pD3D12Resource, i);
		pVideoContext.GetCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}
	
	//pVideoContext.CopyTextureRegion(m_pScalingResource,0,0,0,)
	//if (resetquad)
		//UpdateQuad();
	
	
	
	
	//D3D12_RESOURCE_STATE_COPY_DEST
	
	//pVideoContext.SetDynamicDescriptor(0, 0, D3D12_CPU_VIRTUAL_ADDRESS_UNKNOWN);
	//ImageScaling::SetPipelineBilinear(pVideoContext);
	pVideoContext.SetRootSignature(m_RootSig);
	pVideoContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//pVideoContext.TransitionResource(m_pTemporaryTexture[textureIndex], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pVideoContext.TransitionResourceShutUp(m_pScalingResource[0], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pVideoContext.TransitionResourceShutUp(m_pScalingResource[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pVideoContext.SetPipelineState(m_VideoPSO);
	//pVideoContext.SetDynamicDescriptor(0, 0, SwapChainBufferColor[p_CurrentBuffer].GetSRV());
	
	//float col[4] = { 1.0f, 0.5f, 0.0f, 1.0f };
	//pVideoContext.ClearColor(SwapChainBufferColor[p_CurrentBuffer], col, &m_windowRect);
	
	//pVideoContext.SetDynamicConstantBufferView(1, sizeof(m_pBufferVar), &m_pBufferVar);
	//pVideoContext.SetDynamicConstantBufferView(2, sizeof(m_sShaderConstants), &m_sShaderConstants);
	//pVideoContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_pTemporaryTexture[textureIndex].GetHeapPointer());
	
	//pVideoContext.SetDescriptorTable(0, m_pTemporaryTexture[textureIndex].GetGPUHandle(0));
	//pVideoContext.SetDescriptorTable(0, m_pTemporaryTexture[textureIndex].GetGPUHandle(1));//not sure for this calculation for the gpuhandle
	
	
	
	//pVideoContext.SetDynamicConstantBufferView(1, sizeof(m_pBufferVar), &m_pBufferVar);
	//pVideoContext.Draw(3);
	pVideoContext.SetDynamicDescriptor(0, 0, SwapChainBufferColor[p_CurrentBuffer].GetSRV());
	pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET);
	pVideoContext.SetRenderTarget(SwapChainBufferColor[p_CurrentBuffer].GetRTV());
	//pVideoContext.SetVertexBuffers(1, 1, &m_pVertexBufferView);
	pVideoContext.SetViewportAndScissor(0, 0, m_windowRect.Width(), m_windowRect.Height());
	pVideoContext.SetDynamicDescriptor(0, 0, m_pScalingResource[0].GetSRV());
	pVideoContext.SetDynamicDescriptor(0, 1, m_pScalingResource[1].GetSRV());
	//pVideoContext.DrawInstanced(3,2);
	//pVideoContext.SetIndexBuffer(m_pIndexBufferView);
	pVideoContext.Draw(3);
	//pVideoContext.Draw((6, 1, 0, 0, 0);


	Display(pVideoContext, 10, 10, m_windowRect.Width(), m_windowRect.Height());


	pVideoContext.TransitionResource(SwapChainBufferColor[p_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);
	pVideoContext.Finish();
	DXGI_PRESENT_PARAMETERS presentParams = { 0 };
	m_pDXGISwapChain4->Present1(0, 0, &presentParams);
	p_CurrentBuffer = (p_CurrentBuffer + 1) % 3;


	
	//m_pPixelShaderConstants->Unmap(0,NULL);
	

	return S_OK;
}

void CDX12VideoProcessor::Display(GraphicsContext& Context, float x, float y, float w, float h)
{
	std::wstring str;
	str.reserve(700);
	str.assign(m_strStatsHeader);
	str.append(m_strStatsDispInfo);
	str += fmt::format(L"\nGraph. Adapter: {}", m_strAdapterDescription);
	wchar_t frametype = 'p';//(m_SampleFormat != D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE) ? 'i' : 'p';
	str += fmt::format(
		L"\nFrame rate    : {:7.3f}{},{:7.3f}",
		m_pFilter->m_FrameStats.GetAverageFps(),
		frametype,
		m_pFilter->m_DrawStats.GetAverageFps()
	);

	str.append(m_strStatsInputFmt);
	str.append(m_strStatsVProc);
	const int dstW = m_videoRect.Width();
	const int dstH = m_videoRect.Height();
	str += fmt::format(L"\nScaling       : {}x{} -> {}x{}", m_srcRectWidth, m_srcRectHeight, dstW, dstH);
	str.append(L" D3D12");
	str.append(m_strStatsHDR);
	str.append(m_strStatsPresent);
	str += fmt::format(L"\nFrames: {:5}, skipped: {}/{}, failed: {}",
		m_pFilter->m_FrameStats.GetFrames(), m_pFilter->m_DrawStats.m_dropped, m_RenderStats.dropped2, m_RenderStats.failed);
	str += fmt::format(L"\nTimes(ms): Copy{:3}, Paint{:3} [DX9Subs{:3}], Present{:3}",
		m_RenderStats.copyticks * 1000 / GetPreciseTicksPerSecondI(),
		m_RenderStats.paintticks * 1000 / GetPreciseTicksPerSecondI(),
		m_RenderStats.substicks * 1000 / GetPreciseTicksPerSecondI(),
		m_RenderStats.presentticks * 1000 / GetPreciseTicksPerSecondI());
	str += fmt::format(L"\nSync offset   : {:+3} ms", (m_RenderStats.syncoffset + 5000) / 10000);
	
	TextContext Text(Context);
	Text.Begin();
	Text.EnableDropShadow(true);
	Text.SetTextSize(20.0f);
	Text.SetColor(Color(255.0f, 1.0f, 1.0f,255.0f));
	Text.DrawString(str.c_str());
	
	Text.End();
	Context.SetScissor(0, 0, w, h);
	return;

	Text.DrawFormattedString("YO BITCH TES BELLE");
	Text.ResetCursor(x, y);
	float hScale = w / 1920.0f;
	float vScale = h / 1080.0f;
//Context.SetScissor((uint32_t)Math::Floor(x * hScale), (uint32_t)Math::Floor(y * vScale),
//		(uint32_t)Math::Ceiling((x + w) * hScale), (uint32_t)Math::Ceiling((y + h) * vScale));
	Text.ResetCursor(x, y + 100);
	Text.SetColor(Color(0.5f, 1.0f, 1.0f));
	Text.DrawString("Engine Tuning\n");
	Text.SetTextSize(20.0f);
	Text.End();
	Context.SetScissor(0, 0, w, h);
}

HRESULT CDX12VideoProcessor::Render(int field)
{

	return E_NOTIMPL;
	return S_OK;
	GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Render Video");
	D3D12_VIEWPORT VP;
	VP.TopLeftX = 0;
	VP.TopLeftY = 0;
	VP.Width = 1280;
	VP.Height = 528;
	VP.MinDepth = 0.0f;
	VP.MaxDepth = 1.0f;
	pVideoContext.SetRootSignature(m_RootSig);
	
	
	
	//pVideoContext.CopyTextureRegion(D3D12Public::g_OverlayBuffer, 0, 0, 0, m_pVideoBuffer, rect);
	//pVideoContext.ClearColor(D3D12Public::g_OverlayBuffer);
	
	pVideoContext.SetRenderTarget(D3D12Public::g_OverlayBuffer.GetRTV());
	
	pVideoContext.SetPipelineState(m_VideoPSO);
	pVideoContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pVideoContext.SetViewportAndScissor(0,0,1280,528);// SetDynamicConstantBufferView(m_DefaultPSO)
	
#if 0
	VideoQuadVertex verts[4];
	verts[0] = { { 0.0f,528.0f,0.0f },{0.0f,1.0f} };
	verts[1] = { { 1280.0f,528.0f, 0.0f}, {1.0f,1.0f} };
	verts[2] = { { 1280.0f,0.0f, 0.0f},  {1.0f, 0.0f} };
	verts[3] = { { 0.0f,0.0f, 0.0f},  {0.0f, 0.0f} };
	D3D12_VERTEX_BUFFER_VIEW vertt;
	const UINT vertexBufferSize = sizeof(verts);
	D3D12_GPU_VIRTUAL_ADDRESS addrs = m_pVideoBuffer.GetResource()->GetGPUVirtualAddress();
	vertt.BufferLocation = m_pVideoBuffer.GetGpuVirtualAddress();
	vertt.StrideInBytes = sizeof(VideoQuadVertex);
	vertt.SizeInBytes = vertexBufferSize;

	pVideoContext.SetDynamicVB(0, 4,sizeof(VideoQuadVertex),verts);
	pVideoContext.DrawInstanced(6, 1, 0, 0);
#endif
	//pVideoContext.SetDynamicDescriptors(2, 0, 1, &m_pVideoBuffer.GetSRV());
	pVideoContext.Finish();
	Display::Present();

	
	
	return S_OK;
    
}

HRESULT CDX12VideoProcessor::FillBlack()
{
	//paint the background black
	/*GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");
	gfxContext.TransitionResource(D3D12Public::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	
	gfxContext.ClearColor(D3D12Public::g_SceneColorBuffer);

	gfxContext.Finish();*/
    return S_OK;
}

void CDX12VideoProcessor::SetVideoRect(const CRect& videoRect)
{
	m_videoRect = videoRect;
	
	resetquad = true;
}

HRESULT CDX12VideoProcessor::SetWindowRect(const CRect& windowRect)
{
	m_windowRect = windowRect;
	resetquad = true;
	return S_OK;
}

HRESULT CDX12VideoProcessor::Reset()
{
	for (int i = 0; i < 3; i++)
	{
		SwapChainBufferColor[i].Destroy();
		if(SwapChainBuffer[i])
			SwapChainBuffer[i]->Release();
	}

	m_pDXGISwapChain1.Release();

	CComPtr<IDXGIFactory4> dxgiFactory;
	EXECUTE_ASSERT(S_OK == CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = 1920;
	swapChainDesc.Height = 1080;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// DXGI_FORMAT_R10G10B10A2_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 3;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
	fsSwapChainDesc.Windowed = TRUE;

	
	HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(
		D3D12Public::g_CommandManager.GetCommandQueue(),
		D3D12Public::g_hWnd,
		&swapChainDesc,
		&fsSwapChainDesc,
		nullptr,
		&m_pDXGISwapChain1);

	hr = m_pDXGISwapChain1->QueryInterface(MY_IID_PPV_ARGS(&m_pDXGISwapChain4)); 
	EsramAllocator esram;
	for (UINT i = 0; i < 3; i++)
	{
		hr = m_pDXGISwapChain4->GetBuffer(i, IID_ID3D12Resource, (void**)&SwapChainBuffer[i]);
		SwapChainBufferColor[i].CreateFromSwapChain(L"Primary SwapChain Buffer", SwapChainBuffer[i]);
	}

	dxgiFactory.Release();
	DXGI_SWAP_CHAIN_DESC desc2;
	m_pDXGISwapChain1->GetDesc(&desc2);
	
	if (desc2.OutputWindow != m_hWnd)
		assert(0);
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