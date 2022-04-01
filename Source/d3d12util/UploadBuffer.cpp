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
#include "UploadBuffer.h"
#include "DX12Helper.h"

using namespace D3D12Engine;

void UploadBuffer::Create( const std::wstring& name, size_t BufferSize )
{
    Destroy();

    m_BufferSize = BufferSize;

    // Create an upload buffer.  This is CPU-visible, but it's write combined memory, so
    // avoid reading back from it.
    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    // Upload buffers must be 1-dimensional
    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Width = m_BufferSize;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    EXECUTE_ASSERT(S_OK ==  g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, MY_IID_PPV_ARGS(&m_pResource)) );

    m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

#ifdef RELEASE
    (name);
#else
    m_pResource->SetName(name.c_str());
#endif
}


void* UploadBuffer::Map(void)
{
    void* Memory;
    m_pResource->Map(0, &CD3DX12_RANGE(0, m_BufferSize), &Memory);
    return Memory;
}

void UploadBuffer::Unmap(size_t begin, size_t end)
{
    m_pResource->Unmap(0, &CD3DX12_RANGE(begin, std::min(end, m_BufferSize)));
}
