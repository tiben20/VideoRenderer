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

#include "PixelBuffer.h"
#include "Color.h"
#include "GpuBuffer.h"

class EsramAllocator;

class ColorBuffer : public PixelBuffer
{
public:
    ColorBuffer( Color ClearColor = Color(0.0f, 0.0f, 0.0f, 0.0f)  )
        : m_ClearColor(ClearColor), m_NumMipMaps(0), m_FragmentCount(1), m_SampleCount(1)
    {
        m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        for (int i = 0; i < _countof(m_UAVHandle); ++i)
            m_UAVHandle[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }
    std::wstring GetName()
    {
      return m_Name;
    }

    void SetState(D3D12_RESOURCE_STATES NewState) { m_UsageState = NewState; }

    void DestroyBuffer()
    {
      //clear the value in case we use public buffer
      m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
      m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
      for (int i = 0; i < _countof(m_UAVHandle); ++i)
        m_UAVHandle[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
      m_Width = 0;
      m_Height = 0;
      m_Format = DXGI_FORMAT_UNKNOWN;
      Destroy();
    }
    
    
    // Create a color buffer from a swap chain buffer.  Unordered access is restricted.
    void CreateFromSwapChain( const std::wstring& Name, ID3D12Resource* BaseResource );

    // Create a color buffer.  If an address is supplied, memory will not be allocated.
    // The vmem address allows you to alias buffers (which can be especially useful for
    // reusing ESRAM across a frame.)
    void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
        DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
    
    // Create a color buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
    // this functions the same as Create() without a video address.
    void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
        DXGI_FORMAT Format, EsramAllocator& Allocator);

    // Create a color buffer.  If an address is supplied, memory will not be allocated.
    // The vmem address allows you to alias buffers (which can be especially useful for
    // reusing ESRAM across a frame.)
    void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
        DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
    
    // Create a color buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
    // this functions the same as Create() without a video address.
    void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
        DXGI_FORMAT Format, EsramAllocator& Allocator);

    void CreateShared(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
      DXGI_FORMAT Format, ID3D12Resource* resource);

    // Get pre-created CPU-visible descriptor handles
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return m_SRVHandle; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV(void) const { return m_RTVHandle; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return m_UAVHandle[0]; }

    void SetClearColor( Color ClearColor ) { m_ClearColor = ClearColor; }

    void SetMsaaMode( uint32_t NumColorSamples, uint32_t NumCoverageSamples )
    {
        ASSERT(NumCoverageSamples >= NumColorSamples);
        m_FragmentCount = NumColorSamples;
        m_SampleCount = NumCoverageSamples;
    }

    Color GetClearColor(void) const { return m_ClearColor; }

    
protected:
  std::wstring m_Name;
    D3D12_RESOURCE_FLAGS CombineResourceFlags( void ) const
    {
        D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

        if (Flags == D3D12_RESOURCE_FLAG_NONE && m_FragmentCount == 1)
            Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | Flags;
    }

    // Compute the number of texture levels needed to reduce to 1x1.  This uses
    // _BitScanReverse to find the highest set bit.  Each dimension reduces by
    // half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
    // as the dimension 511 (0x1FF).
    static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
    {
        uint32_t HighBit;
        _BitScanReverse((unsigned long*)&HighBit, Width | Height);
        return HighBit + 1;
    }

    void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);

    Color m_ClearColor;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
    uint32_t m_NumMipMaps; // number of texture sublevels
    uint32_t m_FragmentCount;
    uint32_t m_SampleCount;
};
