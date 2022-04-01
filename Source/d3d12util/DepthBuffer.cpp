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
#include "DepthBuffer.h"
#include "DX12Helper.h"

#include "DescriptorHeap.h"

using namespace D3D12Engine;

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr )
{
    Create(Name, Width, Height, 1, Format, VidMemPtr);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Samples, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr )
{
    D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ResourceDesc.SampleDesc.Count = Samples;

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
    CreateTextureResource(D3D12Engine::g_Device, Name, ResourceDesc, ClearValue, VidMemPtr);
    CreateDerivedViews(D3D12Engine::g_Device, Format);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, EsramAllocator& Allocator )
{
    Create(Name, Width, Height, 1, Format, Allocator);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Samples, DXGI_FORMAT Format, EsramAllocator& )
{
    Create(Name, Width, Height, Samples, Format);
}

void DepthBuffer::CreateDerivedViews( ID3D12Device* Device, DXGI_FORMAT Format )
{
    ID3D12Resource* Resource = m_pResource;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Format = GetDSVFormat(Format);
    if (Resource->GetDesc().SampleDesc.Count == 1)
    {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
    }
    else
    {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    }

    if (m_hDSV[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_hDSV[0] = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_hDSV[1] = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[0]);

    dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[1]);

    DXGI_FORMAT stencilReadFormat = GetStencilFormat(Format);
    if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
    {
        if (m_hDSV[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        {
            m_hDSV[2] = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            m_hDSV[3] = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }

        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
        Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[2]);

        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
        Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[3]);
    }
    else
    {
        m_hDSV[2] = m_hDSV[0];
        m_hDSV[3] = m_hDSV[1];
    }

    if (m_hDepthSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_hDepthSRV = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create the shader resource view
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = GetDepthFormat(Format);
    if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
    {
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;
    }
    else
    {
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    Device->CreateShaderResourceView( Resource, &SRVDesc, m_hDepthSRV );

    if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
    {
        if (m_hStencilSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
            m_hStencilSRV = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        SRVDesc.Format = stencilReadFormat;
        Device->CreateShaderResourceView( Resource, &SRVDesc, m_hStencilSRV );
    }
}
