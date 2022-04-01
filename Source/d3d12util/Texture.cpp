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
#include "Texture.h"
//#include "DDSTextureLoader.h"
//#include "FileUtility.h"
//#include "DX12Engine.h"
#include "CommandContext.h"
#include <map>
#include <thread>

using namespace std;
using namespace D3D12Engine;

//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
size_t BitsPerPixel( _In_ DXGI_FORMAT fmt );

static UINT BytesPerPixel( DXGI_FORMAT Format )
{
    return (UINT)BitsPerPixel(Format) / 8;
};

void Texture::Create2D( size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData )
{
    Destroy();

    m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

    m_Width = (uint32_t)Width;
    m_Height = (uint32_t)Height;
    m_Depth = 1;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = Width;
    texDesc.Height = (UINT)Height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = Format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    EXECUTE_ASSERT(S_OK == g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        m_UsageState, nullptr, MY_IID_PPV_ARGS(&m_pResource)));

    m_pResource->SetName(L"Texture");

    D3D12_SUBRESOURCE_DATA texResource;
    texResource.pData = InitialData;
    texResource.RowPitch = RowPitchBytes;
    texResource.SlicePitch = RowPitchBytes * Height;

    CommandContext::InitializeTexture(*this, 1, &texResource);

    if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_hCpuDescriptorHandle = D3D12Engine::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_Device->CreateShaderResourceView(m_pResource, nullptr, m_hCpuDescriptorHandle);
}
