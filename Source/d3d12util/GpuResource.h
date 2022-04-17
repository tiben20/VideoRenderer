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


class GpuResource
{
    friend class CommandContext;
    friend class GraphicsContext;
    friend class ComputeContext;
    friend class VideoCopyContext;
	friend class VideoProcessorContext;

public:
    GpuResource() : 
        m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        m_UsageState(D3D12_RESOURCE_STATE_COMMON),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1)
    {
    }

    GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
        m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        m_pResource(pResource),
        m_UsageState(CurrentState),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1)
    {
    }

    ~GpuResource() { Destroy(); }

    virtual void Destroy()
    {
        m_pResource = nullptr;
        m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        ++m_VersionID;
    }

    ID3D12Resource* operator->() { return m_pResource; } 
    const ID3D12Resource* operator->() const { return m_pResource; }

    ID3D12Resource* GetResource() { return m_pResource; } 
    const ID3D12Resource* GetResource() const { return m_pResource; }

    ID3D12Resource** GetAddressOf() { return &m_pResource; }

    D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }

    uint32_t GetVersionID() const { return m_VersionID; }

protected:

    CComPtr<ID3D12Resource> m_pResource;
    D3D12_RESOURCE_STATES m_UsageState;
    D3D12_RESOURCE_STATES m_TransitioningState;
    D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;

    // Used to identify when a resource changes so descriptors can be copied etc.
    uint32_t m_VersionID = 0;
};
