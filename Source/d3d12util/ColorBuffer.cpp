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

#include "stdafx.h"
#include "ColorBuffer.h"
#include "CommandContext.h"


using namespace D3D12Engine;

void ColorBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips)
{
    ASSERT(ArraySize == 1 || NumMips == 1, "We don't support auto-mips on texture arrays");

    m_NumMipMaps = NumMips - 1;

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

    RTVDesc.Format = Format;
    UAVDesc.Format = GetUAVFormat(Format);
    SRVDesc.Format = Format;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (ArraySize > 1)
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        RTVDesc.Texture2DArray.MipSlice = 0;
        RTVDesc.Texture2DArray.FirstArraySlice = 0;
        RTVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        UAVDesc.Texture2DArray.MipSlice = 0;
        UAVDesc.Texture2DArray.FirstArraySlice = 0;
        UAVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        SRVDesc.Texture2DArray.MipLevels = NumMips;
        SRVDesc.Texture2DArray.MostDetailedMip = 0;
        SRVDesc.Texture2DArray.FirstArraySlice = 0;
        SRVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;
    }
    else if (m_FragmentCount > 1)
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }
    else 
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        RTVDesc.Texture2D.MipSlice = 0;

        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        UAVDesc.Texture2D.MipSlice = 0;

        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = NumMips;
        SRVDesc.Texture2D.MostDetailedMip = 0;
    }

    if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_RTVHandle = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_SRVHandle = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    ID3D12Resource* Resource = m_pResource;

    // Create the render target view
    Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle);

    // Create the shader resource view
    Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);

    if (m_FragmentCount > 1)
        return;

    // Create the UAVs for each mip level (RWTexture2D)
    for (uint32_t i = 0; i < NumMips; ++i)
    {
        if (m_UAVHandle[i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
            m_UAVHandle[i] = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        Device->CreateUnorderedAccessView(Resource, nullptr, &UAVDesc, m_UAVHandle[i]);

        UAVDesc.Texture2D.MipSlice++;
    }
}

void ColorBuffer::CreateFromSwapChain( const std::wstring& Name, ID3D12Resource* BaseResource )
{
    AssociateWithResource(D3D12Engine::g_Device, Name, BaseResource, D3D12_RESOURCE_STATE_PRESENT);

    //m_UAVHandle[0] = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    //D3D12Engine::g_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, nullptr, m_UAVHandle[0]);

    m_RTVHandle = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12Engine::g_Device->CreateRenderTargetView(m_pResource, nullptr, m_RTVHandle);
}

void ColorBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
    DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMem)
{
  m_Name = Name;
    NumMips = (NumMips == 0 ? ComputeNumMips(Width, Height) : NumMips);
    D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
    D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, NumMips, Format, Flags);

    ResourceDesc.SampleDesc.Count = m_FragmentCount;
    ResourceDesc.SampleDesc.Quality = 0;

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
    ClearValue.Color[0] = m_ClearColor.R();
    ClearValue.Color[1] = m_ClearColor.G();
    ClearValue.Color[2] = m_ClearColor.B();
    ClearValue.Color[3] = m_ClearColor.A();

    CreateTextureResource(D3D12Engine::g_Device, Name, ResourceDesc, ClearValue, VidMem);
    CreateDerivedViews(D3D12Engine::g_Device, Format, 1, NumMips);
}

void ColorBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
    DXGI_FORMAT Format, EsramAllocator&)
{
    Create(Name, Width, Height, NumMips, Format);
}

void ColorBuffer::CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
    DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMem )
{
    D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
    D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, ArrayCount, 1, Format, Flags);

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
    ClearValue.Color[0] = m_ClearColor.R();
    ClearValue.Color[1] = m_ClearColor.G();
    ClearValue.Color[2] = m_ClearColor.B();
    ClearValue.Color[3] = m_ClearColor.A();

    CreateTextureResource(D3D12Engine::g_Device, Name, ResourceDesc, ClearValue, VidMem);
    CreateDerivedViews(D3D12Engine::g_Device, Format, ArrayCount, 1);
}

void ColorBuffer::CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
    DXGI_FORMAT Format, EsramAllocator& )
{
    CreateArray(Name, Width, Height, ArrayCount, Format);
}

void ColorBuffer::CreateShared(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
  DXGI_FORMAT Format,ID3D12Resource* resource)
{
  D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
  D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, 1, Format, Flags);

  ResourceDesc.SampleDesc.Count = m_FragmentCount;
  ResourceDesc.SampleDesc.Quality = 0;

  D3D12_CLEAR_VALUE ClearValue = {};
  ClearValue.Format = Format;
  ClearValue.Color[0] = m_ClearColor.R();
  ClearValue.Color[1] = m_ClearColor.G();
  ClearValue.Color[2] = m_ClearColor.B();
  ClearValue.Color[3] = m_ClearColor.A();
  ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
  CreateSharedTextureResource(D3D12Engine::g_Device, Name, ResourceDesc, ClearValue, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, resource);
  D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
  D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
  D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

  RTVDesc.Format = Format;
  UAVDesc.Format = GetUAVFormat(Format);
  SRVDesc.Format = Format;
  SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
  RTVDesc.Texture2D.MipSlice = 0;

  UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
  UAVDesc.Texture2D.MipSlice = 0;

  SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  SRVDesc.Texture2D.MipLevels = 1;
  SRVDesc.Texture2D.MostDetailedMip = 0;


  if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
  {
    //m_RTVHandle = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_SRVHandle = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  }

// Create the render target view
//D3D12Engine::g_Device->CreateRenderTargetView(m_pResource, &RTVDesc, m_RTVHandle);

// Create the shader resource view
D3D12Engine::g_Device->CreateShaderResourceView(m_pResource, &SRVDesc, m_SRVHandle);

if (m_FragmentCount > 1)
return;

// Create the UAVs for each mip level (RWTexture2D)
/*for (uint32_t i = 0; i < 1; ++i)
{
  if (m_UAVHandle[i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    m_UAVHandle[i] = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  Device->CreateUnorderedAccessView(Resource, nullptr, &UAVDesc, m_UAVHandle[i]);

  UAVDesc.Texture2D.MipSlice++;
}*/

}
