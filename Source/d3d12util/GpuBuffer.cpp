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
#include "GpuBuffer.h"
//#include "DX12Helper.h"
//#include "EsramAllocator.h"
#include "CommandContext.h"
#include "BufferManager.h"
#include "UploadBuffer.h"

using namespace D3D12Engine;

void GpuBuffer::Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const void* initialData )
{
    Destroy();

    m_ElementCount = NumElements;
    m_ElementSize = ElementSize;
    m_BufferSize = NumElements * ElementSize;

    D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

    m_UsageState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    EXECUTE_ASSERT(S_OK ==  g_Device->CreateCommittedResource( &HeapProps, D3D12_HEAP_FLAG_NONE,
        &ResourceDesc, m_UsageState, nullptr, MY_IID_PPV_ARGS(&m_pResource)) );

    m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

    if (initialData)
        CommandContext::InitializeBuffer(*this, initialData, m_BufferSize);

#ifdef RELEASE
    (name);
#else
    m_pResource->SetName(name.c_str());
#endif

    CreateDerivedViews();
}

void GpuBuffer::Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const UploadBuffer& srcData, uint32_t srcOffset )
{
    Destroy();

    m_ElementCount = NumElements;
    m_ElementSize = ElementSize;
    m_BufferSize = NumElements * ElementSize;

    D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

    m_UsageState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    EXECUTE_ASSERT(S_OK ==  
        g_Device->CreateCommittedResource( &HeapProps, D3D12_HEAP_FLAG_NONE,
            &ResourceDesc, m_UsageState, nullptr, MY_IID_PPV_ARGS(&m_pResource)) );

    m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

    CommandContext::InitializeBuffer(*this, srcData, srcOffset);

#ifdef RELEASE
    (name);
#else
    m_pResource->SetName(name.c_str());
#endif

    CreateDerivedViews();
}

// Sub-Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command context.
void GpuBuffer::CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize,
    const void* initialData)
{
    m_ElementCount = NumElements;
    m_ElementSize = ElementSize;
    m_BufferSize = NumElements * ElementSize;

    D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

    m_UsageState = D3D12_RESOURCE_STATE_COMMON;

    EXECUTE_ASSERT(S_OK == g_Device->CreatePlacedResource(pBackingHeap, HeapOffset, &ResourceDesc, m_UsageState, nullptr, MY_IID_PPV_ARGS(&m_pResource)));

    m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

    if (initialData)
        CommandContext::InitializeBuffer(*this, initialData, m_BufferSize);

#ifdef RELEASE
    (name);
#else
    m_pResource->SetName(name.c_str());
#endif

    CreateDerivedViews();

}

void GpuBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, EsramAllocator& Allocator, const void* initialData)
{
    (void)Allocator;
    Create(name, NumElements, ElementSize, initialData);
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuBuffer::CreateConstantBufferView(uint32_t Offset, uint32_t Size) const
{
    ASSERT(Offset + Size <= m_BufferSize);

    Size = Math::AlignUp(Size, 16);

    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    CBVDesc.BufferLocation = m_GpuVirtualAddress + (size_t)Offset;
    CBVDesc.SizeInBytes = Size;

    D3D12_CPU_DESCRIPTOR_HANDLE hCBV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_Device->CreateConstantBufferView(&CBVDesc, hCBV);
    return hCBV;
}

D3D12_RESOURCE_DESC GpuBuffer::DescribeBuffer(void)
{
    ASSERT(m_BufferSize != 0);

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Alignment = 0;
    Desc.DepthOrArraySize = 1;
    Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags = m_ResourceFlags;
    Desc.Format = DXGI_FORMAT_UNKNOWN;
    Desc.Height = 1;
    Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.MipLevels = 1;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Width = (UINT64)m_BufferSize;
    return Desc;
}

void ByteAddressBuffer::CreateDerivedViews(void)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
    SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

    if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_SRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_Device->CreateShaderResourceView(m_pResource, &SRVDesc, m_SRV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    UAVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

    if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_UAV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_Device->CreateUnorderedAccessView( m_pResource, nullptr, &UAVDesc, m_UAV );
}

void StructuredBuffer::CreateDerivedViews(void)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Buffer.NumElements = m_ElementCount;
    SRVDesc.Buffer.StructureByteStride = m_ElementSize;
    SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_SRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_Device->CreateShaderResourceView(m_pResource, &SRVDesc, m_SRV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    UAVDesc.Buffer.CounterOffsetInBytes = 0;
    UAVDesc.Buffer.NumElements = m_ElementCount;
    UAVDesc.Buffer.StructureByteStride = m_ElementSize;
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    m_CounterBuffer.Create(L"StructuredBuffer::Counter", 1, 4);

    if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_UAV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_Device->CreateUnorderedAccessView(m_pResource, m_CounterBuffer.GetResource(), &UAVDesc, m_UAV);
}

void TypedBuffer::CreateDerivedViews(void)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = m_DataFormat;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Buffer.NumElements = m_ElementCount;
    SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_SRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_Device->CreateShaderResourceView(m_pResource, &SRVDesc, m_SRV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = m_DataFormat;
    UAVDesc.Buffer.NumElements = m_ElementCount;
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_UAV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_Device->CreateUnorderedAccessView(m_pResource, nullptr, &UAVDesc, m_UAV);
}

const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::GetCounterSRV(CommandContext& Context)
{
    Context.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
    return m_CounterBuffer.GetSRV();
}

const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::GetCounterUAV(CommandContext& Context)
{
    Context.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    return m_CounterBuffer.GetUAV();
}

