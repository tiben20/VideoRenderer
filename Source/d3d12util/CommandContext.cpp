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
#include "CommandContext.h"
#include "ColorBuffer.h"
#include "DX12Engine.h"
#include "DescriptorHeap.h"

#include "UploadBuffer.h"
#include "ReadbackBuffer.h"
#include "DX12Engine.h"





void ContextManager::DestroyAllContexts(void)
{
  for (uint32_t i = 0; i < 6; ++i)
    sm_ContextPool[i].clear();
  for (uint32_t i = 0; i < 6; ++i)
  {
    if (!sm_AvailableContexts->empty())
    {
      while (!sm_AvailableContexts->empty())
        sm_AvailableContexts[i].pop();
    }
  }
}



CommandContext* ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE Type)
{
    std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);

    auto& AvailableContexts = sm_AvailableContexts[Type];

    CommandContext* ret = nullptr;
    if (AvailableContexts.empty())
    {
        ret = new CommandContext(Type);
        sm_ContextPool[Type].emplace_back(ret);
        ret->Initialize();
    }
    else
    {
        ret = AvailableContexts.front();
        AvailableContexts.pop();
        ret->Reset();
    }
    ASSERT(ret != nullptr);

    ASSERT(ret->m_Type == Type);

    return ret;
}

void ContextManager::FreeContext(CommandContext* UsedContext)
{
    ASSERT(UsedContext != nullptr);
    std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);
    sm_AvailableContexts[UsedContext->m_Type].push(UsedContext);
}

void CommandContext::DestroyAllContexts(void)
{
    LinearAllocator::DestroyAll();
    DynamicDescriptorHeap::DestroyAll();
    D3D12Engine::g_ContextManager.DestroyAllContexts();
}

CommandContext& CommandContext::BeginVideo(const std::wstring ID)
{
  CommandContext* NewContext = D3D12Engine::g_ContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS);
  NewContext->SetID(ID);
  //if (ID.length() > 0)
  //  assert(0);//EngineProfiling::BeginBlock(ID, NewContext);
  return *NewContext;
}

CommandContext& CommandContext::Begin( const std::wstring ID )
{
    CommandContext* NewContext = D3D12Engine::g_ContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
    NewContext->SetID(ID);
    //if (ID.length() > 0)
    //  assert(0);//EngineProfiling::BeginBlock(ID, NewContext);
    return *NewContext;
}

ComputeContext& ComputeContext::Begin(const std::wstring& ID, bool Async)
{
    ComputeContext& NewContext = D3D12Engine::g_ContextManager.AllocateContext(
        Async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT)->GetComputeContext();
    NewContext.SetID(ID);
    if (ID.length() > 0)
      assert(0);// EngineProfiling::BeginBlock(ID, &NewContext);
    return NewContext;
}

VideoCopyContext& VideoCopyContext::Begin(const std::wstring& ID)
{
  VideoCopyContext& NewContext = D3D12Engine::g_ContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_COPY)->GetCopyContext();
  NewContext.SetID(ID);
  //if (ID.length() > 0)
  //  assert(0);//EngineProfiling::BeginBlock(ID, NewContext);
  return NewContext;
}

void VideoCopyContext::CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION* pDst, const D3D12_TEXTURE_COPY_LOCATION* pSrc)
{
  //the state should already be set
  FlushResourceBarriers();
  m_CommandList->CopyTextureRegion(pDst, 0, 0, 0, pSrc, nullptr);
}
uint64_t CommandContext::Flush(bool WaitForCompletion)
{
    FlushResourceBarriers();

    ASSERT(m_CurrentAllocator != nullptr);

    uint64_t FenceValue = D3D12Engine::g_CommandManager.GetQueue(m_Type).ExecuteCommandList(m_CommandList);

    if (WaitForCompletion)
      D3D12Engine::g_CommandManager.WaitForFence(FenceValue);

    //
    // Reset the command list and restore previous state
    //

    m_CommandList->Reset(m_CurrentAllocator, nullptr);

    if (m_CurGraphicsRootSignature)
    {
        m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature);
    }
    if (m_CurComputeRootSignature)
    {
        m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature);
    }
    if (m_CurPipelineState)
    {
        m_CommandList->SetPipelineState(m_CurPipelineState);
    }

    BindDescriptorHeaps();

    return FenceValue;
}

uint64_t CommandContext::FinishVideo(bool WaitForCompletion)
{
  ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS);

  if (m_NumBarriersToFlush > 0)
  {
    m_VideoCommandList->ResourceBarrier(m_NumBarriersToFlush, m_ResourceBarrierBuffer);
    m_NumBarriersToFlush = 0;
  }

  //if (m_ID.length() > 0)
  //  assert(0);//EngineProfiling::EndBlock(this);

  ASSERT(m_CurrentAllocator != nullptr);

  CommandQueue& Queue = D3D12Engine::g_CommandManager.GetQueue(m_Type);

  uint64_t FenceValue = Queue.ExecuteCommandList(m_VideoCommandList);
  Queue.DiscardAllocator(FenceValue, m_CurrentAllocator);
  m_CurrentAllocator = nullptr;

  m_CpuLinearAllocator.CleanupUsedPages(FenceValue);
  m_GpuLinearAllocator.CleanupUsedPages(FenceValue);
  m_DynamicViewDescriptorHeap.CleanupUsedHeaps(FenceValue);
  m_DynamicSamplerDescriptorHeap.CleanupUsedHeaps(FenceValue);

  if (WaitForCompletion)
    D3D12Engine::g_CommandManager.WaitForFence(FenceValue);

  D3D12Engine::g_ContextManager.FreeContext(this);

  return FenceValue;
}

uint64_t CommandContext::Finish( bool WaitForCompletion )
{
    ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_DIRECT || m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE);

    FlushResourceBarriers();

    ASSERT(m_CurrentAllocator != nullptr);

    CommandQueue& Queue = D3D12Engine::g_CommandManager.GetQueue(m_Type);

    uint64_t FenceValue = Queue.ExecuteCommandList(m_CommandList);
    Queue.DiscardAllocator(FenceValue, m_CurrentAllocator);
    m_CurrentAllocator = nullptr;

    m_CpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_GpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_DynamicViewDescriptorHeap.CleanupUsedHeaps(FenceValue);
    m_DynamicSamplerDescriptorHeap.CleanupUsedHeaps(FenceValue);

    if (WaitForCompletion)
      D3D12Engine::g_CommandManager.WaitForFence(FenceValue);

    D3D12Engine::g_ContextManager.FreeContext(this);

    return FenceValue;
}

CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE Type) :
    m_Type(Type),
    m_DynamicViewDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
    m_DynamicSamplerDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
    m_CpuLinearAllocator(kCpuWritable), 
    m_GpuLinearAllocator(kGpuExclusive)
{
    m_OwningManager = nullptr;
    m_CommandList = nullptr;
    m_VideoCommandList = nullptr;
    m_CurrentAllocator = nullptr;
    ZeroMemory(m_CurrentDescriptorHeaps, sizeof(m_CurrentDescriptorHeaps));

    m_CurGraphicsRootSignature = nullptr;
    m_CurComputeRootSignature = nullptr;
    m_CurPipelineState = nullptr;
    m_NumBarriersToFlush = 0;
}

void CommandContext::FreeContext()
{
  m_CpuLinearAllocator.ResetToZero();
  m_GpuLinearAllocator.ResetToZero();
}

CommandContext::~CommandContext( void )
{
  FreeContext();
    if (m_CommandList != nullptr)
        m_CommandList->Release();
    if (m_VideoCommandList != nullptr)
      m_VideoCommandList->Release();
}

void CommandContext::Initialize(void)
{
  if (m_Type != D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS)
    D3D12Engine::g_CommandManager.CreateNewCommandList(m_Type, &m_CommandList, &m_CurrentAllocator);
  else
    D3D12Engine::g_CommandManager.CreateNewVideoCommandList(m_Type, &m_VideoCommandList, &m_CurrentAllocator);
  
}

void CommandContext::Reset( void )
{
    // We only call Reset() on previously freed contexts.  The command list persists, but we must
    // request a new allocator.
  if (m_Type == D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS)
  {
    ASSERT(m_VideoCommandList != nullptr && m_CurrentAllocator == nullptr);
    m_CurrentAllocator = D3D12Engine::g_CommandManager.GetQueue(m_Type).RequestAllocator();
    m_VideoCommandList->Reset(m_CurrentAllocator);

    m_CurGraphicsRootSignature = nullptr;
    m_CurComputeRootSignature = nullptr;
    m_CurPipelineState = nullptr;
    m_NumBarriersToFlush = 0;

    BindDescriptorHeaps();
    return;
  }
    ASSERT(m_CommandList != nullptr && m_CurrentAllocator == nullptr);
    m_CurrentAllocator = D3D12Engine::g_CommandManager.GetQueue(m_Type).RequestAllocator();
    m_CommandList->Reset(m_CurrentAllocator, nullptr);

    m_CurGraphicsRootSignature = nullptr;
    m_CurComputeRootSignature = nullptr;
    m_CurPipelineState = nullptr;
    m_NumBarriersToFlush = 0;

    BindDescriptorHeaps();
}

void CommandContext::BindDescriptorHeaps( void )
{
    UINT NonNullHeaps = 0;
    ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        ID3D12DescriptorHeap* HeapIter = m_CurrentDescriptorHeaps[i];
        if (HeapIter != nullptr)
            HeapsToBind[NonNullHeaps++] = HeapIter;
    }

    if (NonNullHeaps > 0)
        m_CommandList->SetDescriptorHeaps(NonNullHeaps, HeapsToBind);
}

void GraphicsContext::SetRenderTargets( UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV )
{
    m_CommandList->OMSetRenderTargets( NumRTVs, RTVs, FALSE, &DSV );
}

void GraphicsContext::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[])
{
    m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr);
}

void GraphicsContext::BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
    m_CommandList->BeginQuery(QueryHeap, Type, HeapIndex);
}

void GraphicsContext::EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
    m_CommandList->EndQuery(QueryHeap, Type, HeapIndex);
}

void GraphicsContext::ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset)
{
    m_CommandList->ResolveQueryData(QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer, DestinationBufferOffset);
}

void GraphicsContext::ClearUAV( GpuBuffer& Target )
{
    FlushResourceBarriers();

    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    const UINT ClearColor[4] = {};
    m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

void ComputeContext::ClearUAV( GpuBuffer& Target )
{
    FlushResourceBarriers();

    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    const UINT ClearColor[4] = {};
    m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

void GraphicsContext::ClearUAV( ColorBuffer& Target )
{
    FlushResourceBarriers();

    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

    //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
    const float* ClearColor = Target.GetClearColor().GetPtr();
    m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}

void ComputeContext::ClearUAV( ColorBuffer& Target )
{
    FlushResourceBarriers();

    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

    //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
    const float* ClearColor = Target.GetClearColor().GetPtr();
    m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}

void GraphicsContext::ClearColor( ColorBuffer& Target, D3D12_RECT* Rect )
{
    FlushResourceBarriers();
    m_CommandList->ClearRenderTargetView(Target.GetRTV(), Target.GetClearColor().GetPtr(), (Rect == nullptr) ? 0 : 1, Rect);
}

void GraphicsContext::ClearColor(ColorBuffer& Target, float Colour[4], D3D12_RECT* Rect)
{
    FlushResourceBarriers();
    m_CommandList->ClearRenderTargetView(Target.GetRTV(), Colour, (Rect == nullptr) ? 0 : 1, Rect);
}

void GraphicsContext::SetViewportAndScissor( const D3D12_VIEWPORT& vp, const D3D12_RECT& rect )
{
    ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    m_CommandList->RSSetViewports( 1, &vp );
    m_CommandList->RSSetScissorRects( 1, &rect );
}

void GraphicsContext::SetViewport( const D3D12_VIEWPORT& vp )
{
    m_CommandList->RSSetViewports( 1, &vp );
}

void GraphicsContext::SetViewport( FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth, FLOAT maxDepth )
{
    D3D12_VIEWPORT vp;
    vp.Width = w;
    vp.Height = h;
    vp.MinDepth = minDepth;
    vp.MaxDepth = maxDepth;
    vp.TopLeftX = x;
    vp.TopLeftY = y;
    m_CommandList->RSSetViewports( 1, &vp );
}

void GraphicsContext::SetScissor( const D3D12_RECT& rect )
{
    ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    m_CommandList->RSSetScissorRects( 1, &rect );
}

void CommandContext::TransitionResource(ID3D12Resource *Resource, D3D12_RESOURCE_STATES NewState, D3D12_RESOURCE_STATES OldState ,bool FlushImmediate)
{
  

  if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
  {
    ASSERT((OldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == OldState);
    ASSERT((NewState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == NewState);
  }

  if (OldState != NewState)
  {
    ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    BarrierDesc.Transition.pResource = Resource;
    BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    BarrierDesc.Transition.StateBefore = OldState;
    BarrierDesc.Transition.StateAfter = NewState;

    
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

  }
  

  if (FlushImmediate || m_NumBarriersToFlush == 16)
    FlushResourceBarriers();
}

void CommandContext::TransitionResourceShutUp(GpuResource& Resource, D3D12_RESOURCE_STATES NewState)
{
  Resource.m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;
  D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;
  if (1)
  {
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    BarrierDesc.Transition.pResource = Resource.GetResource();
    BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    BarrierDesc.Transition.StateBefore = OldState;
    BarrierDesc.Transition.StateAfter = NewState;

    // Check to see if we already started the transition
    if (NewState == Resource.m_TransitioningState)
    {
      BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
      Resource.m_TransitioningState = (D3D12_RESOURCE_STATES)-1;
    }
    else
      BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

    Resource.m_UsageState = NewState;
  }
  else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    InsertUAVBarrier(Resource, false);

  if (m_NumBarriersToFlush == 16)
    FlushResourceBarriers();
}

void CommandContext::TransitionResource(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
    D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

    if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        ASSERT((OldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == OldState);
        ASSERT((NewState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == NewState);
    }

    if (OldState != NewState)
    {
        ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = Resource.GetResource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = OldState;
        BarrierDesc.Transition.StateAfter = NewState;

        // Check to see if we already started the transition
        if (NewState == Resource.m_TransitioningState)
        {
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
            Resource.m_TransitioningState = (D3D12_RESOURCE_STATES)-1;
        }
        else
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        Resource.m_UsageState = NewState;
    }
    else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        InsertUAVBarrier(Resource, FlushImmediate);

    if (FlushImmediate || m_NumBarriersToFlush == 16)
        FlushResourceBarriers();
}

void CommandContext::BeginResourceTransition(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
    // If it's already transitioning, finish that transition
    if (Resource.m_TransitioningState != (D3D12_RESOURCE_STATES)-1)
        TransitionResource(Resource, Resource.m_TransitioningState);

    D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

    if (OldState != NewState)
    {
        ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = Resource.GetResource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = OldState;
        BarrierDesc.Transition.StateAfter = NewState;

        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

        Resource.m_TransitioningState = NewState;
    }

    if (FlushImmediate || m_NumBarriersToFlush == 16)
        FlushResourceBarriers();
}

void CommandContext::InsertUAVBarrier(GpuResource& Resource, bool FlushImmediate)
{
    ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.UAV.pResource = Resource.GetResource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

void CommandContext::InsertAliasBarrier(GpuResource& Before, GpuResource& After, bool FlushImmediate)
{
    ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.Aliasing.pResourceBefore = Before.GetResource();
    BarrierDesc.Aliasing.pResourceAfter = After.GetResource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

void CommandContext::WriteBuffer( GpuResource& Dest, size_t DestOffset, const void* BufferData, size_t NumBytes )
{
    ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
    DynAlloc TempSpace = m_CpuLinearAllocator.Allocate( NumBytes, 512 );
    SIMDMemCopy(TempSpace.DataPtr, BufferData, Math::DivideByMultiple(NumBytes, 16));
    CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes );
}

void CommandContext::FillBuffer( GpuResource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes )
{
    DynAlloc TempSpace = m_CpuLinearAllocator.Allocate( NumBytes, 512 );
    __m128 VectorValue = _mm_set1_ps(Value.Float);
    SIMDMemFill(TempSpace.DataPtr, VectorValue, Math::DivideByMultiple(NumBytes, 16));
    CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes );
}

void CommandContext::InitializeTexture( GpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[] )
{
    UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.GetResource(), 0, NumSubresources);

    CommandContext& InitContext = CommandContext::Begin();

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    DynAlloc mem = InitContext.ReserveUploadMemory(uploadBufferSize);
    UpdateSubresources(InitContext.m_CommandList, Dest.GetResource(), mem.Buffer.GetResource(), 0, 0, NumSubresources, SubData);
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    InitContext.Finish(true);
}

void CommandContext::CopySubresource(GpuResource& Dest, UINT DestSubIndex, GpuResource& Src, UINT SrcSubIndex)
{
    FlushResourceBarriers();

    D3D12_TEXTURE_COPY_LOCATION DestLocation =
    {
        Dest.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        DestSubIndex
    };

    D3D12_TEXTURE_COPY_LOCATION SrcLocation =
    {
        Src.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        SrcSubIndex
    };

    m_CommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
}

void CommandContext::InitializeTextureArraySlice(GpuResource& Dest, UINT SliceIndex, GpuResource& Src)
{
    CommandContext& Context = CommandContext::Begin();

    Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    Context.FlushResourceBarriers();

    const D3D12_RESOURCE_DESC& DestDesc = Dest.GetResource()->GetDesc();
    const D3D12_RESOURCE_DESC& SrcDesc = Src.GetResource()->GetDesc();

    ASSERT(SliceIndex < DestDesc.DepthOrArraySize &&
        SrcDesc.DepthOrArraySize == 1 &&
        DestDesc.Width == SrcDesc.Width &&
        DestDesc.Height == SrcDesc.Height &&
        DestDesc.MipLevels <= SrcDesc.MipLevels
        );

    UINT SubResourceIndex = SliceIndex * DestDesc.MipLevels;

    for (UINT i = 0; i < DestDesc.MipLevels; ++i)
    {
        D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
        {
            Dest.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            SubResourceIndex + i
        };

        D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
        {
            Src.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            i
        };

        Context.m_CommandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
    }

    Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);
    Context.Finish(true);
}

uint32_t CommandContext::ReadbackTexture(ReadbackBuffer& DstBuffer, PixelBuffer& SrcBuffer)
{
    uint64_t CopySize = 0;

    // The footprint may depend on the device of the resource, but we assume there is only one device.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
    D3D12Engine::g_Device->GetCopyableFootprints(&SrcBuffer.GetResource()->GetDesc(), 0, 1, 0,
        &PlacedFootprint, nullptr, nullptr, &CopySize);

    DstBuffer.Create(L"Readback", (uint32_t)CopySize, 1);

    TransitionResource(SrcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

    m_CommandList->CopyTextureRegion(
        &CD3DX12_TEXTURE_COPY_LOCATION(DstBuffer.GetResource(), PlacedFootprint), 0, 0, 0,
        &CD3DX12_TEXTURE_COPY_LOCATION(SrcBuffer.GetResource(), 0), nullptr);

    return PlacedFootprint.Footprint.RowPitch;
}

void CommandContext::InitializeBuffer( GpuBuffer& Dest, const void* BufferData, size_t NumBytes, size_t DestOffset)
{
    CommandContext& InitContext = CommandContext::Begin();

    DynAlloc mem = InitContext.ReserveUploadMemory(NumBytes);
    SIMDMemCopy(mem.DataPtr, BufferData, Math::DivideByMultiple(NumBytes, 16));

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
    InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, mem.Buffer.GetResource(), 0, NumBytes);
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    InitContext.Finish(true);
}

void CommandContext::InitializeBuffer( GpuBuffer& Dest, const UploadBuffer& Src, size_t SrcOffset, size_t NumBytes, size_t DestOffset )
{
    CommandContext& InitContext = CommandContext::Begin();

    size_t MaxBytes = std::min<size_t>(Dest.GetBufferSize() - DestOffset, Src.GetBufferSize() - SrcOffset);
    NumBytes = std::min<size_t>(MaxBytes, NumBytes);

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
    InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, (ID3D12Resource*)Src.GetResource(), SrcOffset, NumBytes);
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    InitContext.Finish(true);
}

void CommandContext::PIXBeginEvent(const wchar_t* label)
{
#ifdef RELEASE
	(label);
#else
  assert(0);// ::PIXBeginEvent(m_CommandList, 0, label);
#endif
}

void CommandContext::PIXEndEvent(void)
{
#ifndef RELEASE
  assert(0);//::PIXEndEvent(m_CommandList);
#endif
}

void CommandContext::PIXSetMarker(const wchar_t* label)
{
#ifdef RELEASE
	(label);
#else
  assert(0);//::PIXSetMarker(m_CommandList, 0, label);
#endif
}

void VideoProcessorContext::ProcessFrames(ID3D12VideoProcessor* pVideoProcessor, const D3D12_VIDEO_PROCESS_OUTPUT_STREAM_ARGUMENTS* pOutputArguments,
  UINT NumInputStreams, const D3D12_VIDEO_PROCESS_INPUT_STREAM_ARGUMENTS* pInputArguments)
{
  m_VideoCommandList->ProcessFrames(pVideoProcessor, pOutputArguments, NumInputStreams, pInputArguments);
}
