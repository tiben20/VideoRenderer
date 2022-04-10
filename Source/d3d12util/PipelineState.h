/*
 * Original developed by Minigraph author James Stanard
 *
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
#include "d3dx12.h"
class CommandContext;
class RootSignature;
class VertexShader;
class GeometryShader;
class HullShader;
class DomainShader;
class PixelShader;
class ComputeShader;

class PSO
{
public:

  PSO(const wchar_t* Name) : m_Name(Name), m_RootSignature(nullptr), m_PSO(nullptr) {}

  static void DestroyAll(void);

  void SetRootSignature(const RootSignature& BindMappings)
  {
    m_RootSignature = &BindMappings;
  }

  const RootSignature& GetRootSignature(void) const
  {
    ASSERT(m_RootSignature != nullptr);
    return *m_RootSignature;
  }

  ID3D12PipelineState* GetPipelineStateObject(void) const { return m_PSO; }

  void SetName(const wchar_t* Name) { m_Name = Name; }

protected:

    const wchar_t* m_Name;

    const RootSignature* m_RootSignature;

    ID3D12PipelineState* m_PSO;
};

class GraphicsPSO : public PSO
{
    friend class CommandContext;

public:

    // Start with empty state
    GraphicsPSO(const wchar_t* Name = L"Unnamed Graphics PSO");

    void SetBlendState( const D3D12_BLEND_DESC& BlendDesc );
    void SetRasterizerState( const D3D12_RASTERIZER_DESC& RasterizerDesc );
    void SetDepthStencilState( const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc );
    void SetSampleMask( UINT SampleMask );
    void SetPrimitiveTopologyType( D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType );
    void SetDepthTargetFormat( DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0 );
    void SetRenderTargetFormat( DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0 );
    void SetRenderTargetFormats( UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0 );
    void SetInputLayout( UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs );
    void SetPrimitiveRestart( D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps );

    // These const_casts shouldn't be necessary, but we need to fix the API to accept "const void* pShaderBytecode"
    void SetVertexShader( const void* Binary, size_t Size ) { m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
    void SetPixelShader( const void* Binary, size_t Size ) { m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
    void SetGeometryShader( const void* Binary, size_t Size ) { m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
    void SetHullShader( const void* Binary, size_t Size ) { m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
    void SetDomainShader( const void* Binary, size_t Size ) { m_PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }

    void SetVertexShader( const D3D12_SHADER_BYTECODE& Binary ) { m_PSODesc.VS = Binary; }
    void SetPixelShader( const D3D12_SHADER_BYTECODE& Binary ) { m_PSODesc.PS = Binary; }
    void SetGeometryShader( const D3D12_SHADER_BYTECODE& Binary ) { m_PSODesc.GS = Binary; }
    void SetHullShader( const D3D12_SHADER_BYTECODE& Binary ) { m_PSODesc.HS = Binary; }
    void SetDomainShader( const D3D12_SHADER_BYTECODE& Binary ) { m_PSODesc.DS = Binary; }

    // Perform validation and compute a hash value for fast state block comparisons
    void Finalize();
    void FreePSO();
private:

    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
    std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_InputLayouts;
};


class ComputePSO : public PSO
{
    friend class CommandContext;

public:
    ComputePSO(const wchar_t* Name = L"Unnamed Compute PSO");

    void SetComputeShader( const void* Binary, size_t Size ) { m_PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
    void SetComputeShader( const D3D12_SHADER_BYTECODE& Binary ) { m_PSODesc.CS = Binary; }

    void Finalize();
    void FreePSO();
private:

    D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc;
};
