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
#include "DX12Engine.h"
#include "CommandListManager.h"

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE Type) :
    m_Type(Type),
    m_CommandQueue(nullptr),
    m_pFence(nullptr),
    m_NextFenceValue((uint64_t)Type << 56 | 1),
    m_LastCompletedFenceValue((uint64_t)Type << 56),
    m_AllocatorPool(Type)
{
}

CommandQueue::~CommandQueue()
{
    Shutdown();
}

void CommandQueue::Shutdown()
{
    if (m_CommandQueue == nullptr)
        return;

    m_AllocatorPool.Shutdown();
    
    CloseHandle(m_FenceEventHandle);

    m_pFence->Release();
    m_pFence = nullptr;

    m_CommandQueue->Release();
    m_CommandQueue = nullptr;
}

CommandListManager::CommandListManager() :
    m_Device(nullptr),
    m_GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
    m_ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
    m_CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY),
    m_VideoQueue(D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS)
{
}

CommandListManager::~CommandListManager()
{
    Shutdown();
    

}

void CommandListManager::Shutdown()
{
    m_GraphicsQueue.Shutdown();
    m_ComputeQueue.Shutdown();
    m_CopyQueue.Shutdown();
    m_VideoQueue.Shutdown();
}

void CommandQueue::Create(ID3D12Device* pDevice)
{
    ASSERT(pDevice != nullptr);
    ASSERT(!IsReady());
    ASSERT(m_AllocatorPool.Size() == 0);

    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.Type = m_Type;
    QueueDesc.NodeMask = 1;
    pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_CommandQueue));
    m_CommandQueue->SetName(L"CommandListManager::m_CommandQueue");

    EXECUTE_ASSERT(S_OK == pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
    m_pFence->SetName(L"CommandListManager::m_pFence");
    m_pFence->Signal((uint64_t)m_Type << 56);

    m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
    ASSERT(m_FenceEventHandle != NULL);

    m_AllocatorPool.Create(pDevice);

    ASSERT(IsReady());
}

void CommandListManager::Create(ID3D12Device* pDevice)
{
    ASSERT(pDevice != nullptr);

    m_Device = pDevice;

    m_GraphicsQueue.Create(pDevice);
    m_ComputeQueue.Create(pDevice);
    m_CopyQueue.Create(pDevice);
    m_VideoQueue.Create(pDevice);
}

void CommandListManager::CreateNewVideoCommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12VideoProcessCommandList** List, ID3D12CommandAllocator** Allocator)
{
  ASSERT(Type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
  switch (Type)
  {
  case D3D12_COMMAND_LIST_TYPE_DIRECT: *Allocator = m_GraphicsQueue.RequestAllocator(); break;
  case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
  case D3D12_COMMAND_LIST_TYPE_COMPUTE: *Allocator = m_ComputeQueue.RequestAllocator(); break;
  case D3D12_COMMAND_LIST_TYPE_COPY: *Allocator = m_CopyQueue.RequestAllocator(); break;
  case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS: *Allocator = m_VideoQueue.RequestAllocator(); break;
  }

  if (Type == D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS)
  {
    EXECUTE_ASSERT(S_OK == m_Device->CreateCommandList(1, Type, *Allocator, nullptr, IID_ID3D12VideoProcessCommandList, (void**)List));

  }
  if (Type != D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS)
  {
    EXECUTE_ASSERT(S_OK == m_Device->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List)));
    (*List)->SetName(L"CommandList");
  }

}

void CommandListManager::CreateNewCommandList( D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList** List, ID3D12CommandAllocator** Allocator )
{
    ASSERT(Type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
    switch (Type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT: *Allocator = m_GraphicsQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE: *Allocator = m_ComputeQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_COPY: *Allocator = m_CopyQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS: *Allocator = m_VideoQueue.RequestAllocator(); break;
    }

    if (Type == D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS)
    {
      EXECUTE_ASSERT(S_OK == m_Device->CreateCommandList(1, Type, *Allocator, nullptr, IID_ID3D12VideoProcessCommandList, (void**)List));
      
    }
    if (Type != D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS)
    {
       EXECUTE_ASSERT(S_OK ==  m_Device->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List)) );
       (*List)->SetName(L"CommandList");
    }
    
}

uint64_t CommandQueue::ExecuteCommandList( ID3D12CommandList* List )
{
    std::lock_guard<std::mutex> LockGuard(m_FenceMutex);

    EXECUTE_ASSERT(S_OK == ((ID3D12GraphicsCommandList*)List)->Close());

    // Kickoff the command list
    m_CommandQueue->ExecuteCommandLists(1, &List);

    // Signal the next fence value (with the GPU)
    m_CommandQueue->Signal(m_pFence, m_NextFenceValue);

    // And increment the fence value.  
    return m_NextFenceValue++;
}

uint64_t CommandQueue::IncrementFence(void)
{
    std::lock_guard<std::mutex> LockGuard(m_FenceMutex);
    m_CommandQueue->Signal(m_pFence, m_NextFenceValue);
    return m_NextFenceValue++;
}

bool CommandQueue::IsFenceComplete(uint64_t FenceValue)
{
    // Avoid querying the fence value by testing against the last one seen.
    // The max() is to protect against an unlikely race condition that could cause the last
    // completed fence value to regress.
    if (FenceValue > m_LastCompletedFenceValue)
        m_LastCompletedFenceValue = std::max(m_LastCompletedFenceValue, m_pFence->GetCompletedValue());

    return FenceValue <= m_LastCompletedFenceValue;
}

namespace D3D12Engine
{
    extern CommandListManager g_CommandManager;
}

void CommandQueue::StallForFence(uint64_t FenceValue)
{
    CommandQueue& Producer = D3D12Engine::g_CommandManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    m_CommandQueue->Wait(Producer.m_pFence, FenceValue);
}

void CommandQueue::StallForProducer(CommandQueue& Producer)
{
    ASSERT(Producer.m_NextFenceValue > 0);
    m_CommandQueue->Wait(Producer.m_pFence, Producer.m_NextFenceValue - 1);
}

void CommandQueue::WaitForFence(uint64_t FenceValue)
{
    if (IsFenceComplete(FenceValue))
        return;

    // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
    // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
    // the fence can only have one event set on completion, then thread B has to wait for 
    // 100 before it knows 99 is ready.  Maybe insert sequential events?
    {
        std::lock_guard<std::mutex> LockGuard(m_EventMutex);

        m_pFence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
        WaitForSingleObject(m_FenceEventHandle, INFINITE);
        m_LastCompletedFenceValue = FenceValue;
    }
}

void CommandListManager::WaitForFence(uint64_t FenceValue)
{
    CommandQueue& Producer = D3D12Engine::g_CommandManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    Producer.WaitForFence(FenceValue);
}

ID3D12CommandAllocator* CommandQueue::RequestAllocator()
{
    uint64_t CompletedFence = m_pFence->GetCompletedValue();

    return m_AllocatorPool.RequestAllocator(CompletedFence);
}

void CommandQueue::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
{
    m_AllocatorPool.DiscardAllocator(FenceValue, Allocator);
}
