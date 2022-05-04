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
#include "CommandAllocatorPool.h"

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type) :
    m_cCommandListType(Type),
    m_Device(nullptr)
{
  assert(m_ReadyAllocators.empty());
}

CommandAllocatorPool::~CommandAllocatorPool()
{
    Shutdown();
}

void CommandAllocatorPool::Create(ID3D12Device * pDevice)
{
  m_Device = pDevice;
}

void CommandAllocatorPool::Shutdown()
{
  std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);
    for (size_t i = 0; i < m_AllocatorPool.size(); ++i)
        m_AllocatorPool[i]->Release();

    m_AllocatorPool.clear();
    ID3D12CommandAllocator* pAllocator = nullptr;
    while (m_ReadyAllocators.size())
    {
      pAllocator = m_ReadyAllocators.front().second;
      if (pAllocator)
        pAllocator->Release();
      m_ReadyAllocators.pop();
    }
    
}

ID3D12CommandAllocator * CommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
{
    std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

    ID3D12CommandAllocator* pAllocator = nullptr;

    if (!m_ReadyAllocators.empty())
    {
        std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = m_ReadyAllocators.front();

        if (AllocatorPair.first <= CompletedFenceValue)
        {
            pAllocator = AllocatorPair.second;
            EXECUTE_ASSERT(S_OK == pAllocator->Reset());
            m_ReadyAllocators.pop();
        }
    }

    // If no allocator's were ready to be reused, create a new one
    if (pAllocator == nullptr)
    {
        EXECUTE_ASSERT(S_OK == m_Device->CreateCommandAllocator(m_cCommandListType, IID_PPV_ARGS(&pAllocator)));
        wchar_t AllocatorName[32];
        swprintf(AllocatorName, 32, L"CommandAllocator %zu", m_AllocatorPool.size());
        pAllocator->SetName(AllocatorName);
        m_AllocatorPool.push_back(pAllocator);
    }

    return pAllocator;
}

void CommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator * Allocator)
{
    std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

    // That fence value indicates we are free to reset the allocator
    m_ReadyAllocators.push(std::make_pair(FenceValue, Allocator));
}
