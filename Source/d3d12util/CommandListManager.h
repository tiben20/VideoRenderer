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

#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>
#include "CommandAllocatorPool.h"

class CommandQueue
{
    friend class CommandListManager;
    friend class CommandContext;

public:
    CommandQueue(D3D12_COMMAND_LIST_TYPE Type);
    ~CommandQueue();

    void Create(ID3D12Device* pDevice);
    void Shutdown();

    inline bool IsReady()
    {
        return m_CommandQueue != nullptr;
    }

    uint64_t IncrementFence(void);
    bool IsFenceComplete(uint64_t FenceValue);
    void StallForFence(uint64_t FenceValue);
    void StallForProducer(CommandQueue& Producer);
    void WaitForFence(uint64_t FenceValue);
    void WaitForIdle(void) { WaitForFence(IncrementFence()); }

    ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue; }

    uint64_t GetNextFenceValue() { return m_NextFenceValue; }

private:

    uint64_t ExecuteCommandList(ID3D12CommandList* List);
    ID3D12CommandAllocator* RequestAllocator(void);
    void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);

    ID3D12CommandQueue* m_CommandQueue;

    const D3D12_COMMAND_LIST_TYPE m_Type;

    CommandAllocatorPool m_AllocatorPool;
    std::mutex m_FenceMutex;
    std::mutex m_EventMutex;

    // Lifetime of these objects is managed by the descriptor cache
    ID3D12Fence* m_pFence;
    uint64_t m_NextFenceValue;
    uint64_t m_LastCompletedFenceValue;
    HANDLE m_FenceEventHandle;

};

class CommandListManager
{
    friend class CommandContext;

public:
    CommandListManager();
    ~CommandListManager();

    void Create(ID3D12Device* pDevice);
    void Shutdown();

    CommandQueue& GetGraphicsQueue(void) { return m_GraphicsQueue; }
    CommandQueue& GetComputeQueue(void) { return m_ComputeQueue; }
    CommandQueue& GetCopyQueue(void) { return m_CopyQueue; }

    CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        switch (Type)
        {
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_ComputeQueue;
        case D3D12_COMMAND_LIST_TYPE_COPY: return m_CopyQueue;
        default: return m_GraphicsQueue;
        }
    }

    ID3D12CommandQueue* GetCommandQueue()
    {
        return m_GraphicsQueue.GetCommandQueue();
    }

    void CreateNewCommandList(
        D3D12_COMMAND_LIST_TYPE Type,
        ID3D12GraphicsCommandList** List,
        ID3D12CommandAllocator** Allocator);

    // Test to see if a fence has already been reached
    bool IsFenceComplete(uint64_t FenceValue)
    {
        return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue);
    }

    // The CPU will wait for a fence to reach a specified value
    void WaitForFence(uint64_t FenceValue);

    // The CPU will wait for all command queues to empty (so that the GPU is idle)
    void IdleGPU(void)
    {
        m_GraphicsQueue.WaitForIdle();
        m_ComputeQueue.WaitForIdle();
        m_CopyQueue.WaitForIdle();
    }

private:

    ID3D12Device* m_Device;

    CommandQueue m_GraphicsQueue;
    CommandQueue m_ComputeQueue;
    CommandQueue m_CopyQueue;
};
