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

//#include "stdafx.h"
#include "GpuResource.h"

class CommandContext;
class EsramAllocator;
class UploadBuffer;

class GpuBuffer : public GpuResource
{
public:
    virtual ~GpuBuffer() { Destroy(); }

    // Create a buffer.  If initial data is provided, it will be copied into the buffer using the default command context.
    void Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
        const void* initialData = nullptr );

    void Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
        const UploadBuffer& srcData, uint32_t srcOffset = 0 );

    // Create a buffer in ESRAM.  On Windows, ESRAM is not used.
    void Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
        EsramAllocator& Allocator, const void* initialData = nullptr);

    // Sub-Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command context.
    void CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize,
        const void* initialData = nullptr);

    const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return m_UAV; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return m_SRV; }

    D3D12_GPU_VIRTUAL_ADDRESS RootConstantBufferView(void) const { return m_GpuVirtualAddress; }

    D3D12_CPU_DESCRIPTOR_HANDLE CreateConstantBufferView( uint32_t Offset, uint32_t Size ) const;

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t BaseVertexIndex = 0) const
    {
        size_t Offset = BaseVertexIndex * m_ElementSize;
        return VertexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize);
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit = false) const;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t StartIndex = 0) const
    {
        size_t Offset = StartIndex * m_ElementSize;
        return IndexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize == 4);
    }

    size_t GetBufferSize() const { return m_BufferSize; }
    uint32_t GetElementCount() const { return m_ElementCount; }
    uint32_t GetElementSize() const { return m_ElementSize; }

protected:

    GpuBuffer(void) : m_BufferSize(0), m_ElementCount(0), m_ElementSize(0)
    {
        m_ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        m_UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    D3D12_RESOURCE_DESC DescribeBuffer(void);
    virtual void CreateDerivedViews(void) = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE m_UAV;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRV;

    size_t m_BufferSize;
    uint32_t m_ElementCount;
    uint32_t m_ElementSize;
    D3D12_RESOURCE_FLAGS m_ResourceFlags;
};

inline D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
{
    D3D12_VERTEX_BUFFER_VIEW VBView;
    VBView.BufferLocation = m_GpuVirtualAddress + Offset;
    VBView.SizeInBytes = Size;
    VBView.StrideInBytes = Stride;
    return VBView;
}

inline D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit) const
{
    D3D12_INDEX_BUFFER_VIEW IBView;
    IBView.BufferLocation = m_GpuVirtualAddress + Offset;
    IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    IBView.SizeInBytes = Size;
    return IBView;
}

class ByteAddressBuffer : public GpuBuffer
{
public:
    virtual void CreateDerivedViews(void) override;
};

class IndirectArgsBuffer : public ByteAddressBuffer
{
public:
    IndirectArgsBuffer(void)
    {
    }
};

class StructuredBuffer : public GpuBuffer
{
public:
    virtual void Destroy(void) override
    {
        m_CounterBuffer.Destroy();
        GpuBuffer::Destroy();
    }

    virtual void CreateDerivedViews(void) override;

    ByteAddressBuffer& GetCounterBuffer(void) { return m_CounterBuffer; }

    const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterSRV(CommandContext& Context);
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterUAV(CommandContext& Context);

private:
    ByteAddressBuffer m_CounterBuffer;
};

class TypedBuffer : public GpuBuffer
{
public:
    TypedBuffer( DXGI_FORMAT Format ) : m_DataFormat(Format) {}
    virtual void CreateDerivedViews(void) override;

protected:
    DXGI_FORMAT m_DataFormat;
};

