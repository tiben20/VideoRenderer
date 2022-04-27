/*
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

#include "stdafx.h"
#include "RenderThread.h"
#include "../Include/ID3DVideoMemoryConfiguration.h"
#include <mfapi.h>
#include <Mfidl.h>
#include "Helper.h"
//#include "VideoProcessor.h"

CRenderThread::CRenderThread()
{
  m_ePlaybackInit.Set();
  m_iQueueSize = 0;
  m_nRenderState = Shutdown;
  
}


HRESULT CRenderThread::ProcessSample(IMediaSample* pSample)
{
  //locking here so we dont touch the input queue during the process
  CAutoLock lock(&m_VideoInputLock);
  HRESULT hr = CopySample(pSample);
  
  return hr;
}

void CRenderThread::SetDestRect(CRect dstRect)
{
  m_destRect = dstRect;
}

bool CRenderThread::StartThreads()
{
  DWORD dwThreadId;

  if (m_nRenderState == Shutdown) {
    m_hVideoThreadFlush = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    m_hVideoThreadQuit = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    m_hVideoInputThread = ::CreateThread(nullptr, 0, VideoInput, (LPVOID)this, 0, &dwThreadId);
    SetThreadPriority(m_hVideoInputThread, THREAD_PRIORITY_TIME_CRITICAL);
    m_hVideoProcessThread = ::CreateThread(nullptr, 0, VideoProcess, (LPVOID)this, 0, &dwThreadId);
    SetThreadPriority(m_hVideoProcessThread, THREAD_PRIORITY_HIGHEST);
    m_hVideoPresentThread = ::CreateThread(nullptr, 0, VideoPresent, (LPVOID)this, 0, &dwThreadId);
    SetThreadPriority(m_hVideoPresentThread, THREAD_PRIORITY_HIGHEST);

    m_nRenderState = Stopped;
    DLog(L"Video input, process and present thread are started");
    
  }
  return false;
}

void CRenderThread::StopThreads()
{
  if (m_nRenderState != Shutdown) {
    SetEvent(m_hVideoThreadFlush);
    m_bThreadFlush = true;
    SetEvent(m_hVideoThreadQuit);
    m_bThreadQuit = true;
    if (m_hVideoInputThread && WaitForSingleObject(m_hVideoInputThread, 10000) == WAIT_TIMEOUT) {
      ASSERT(FALSE);
      TerminateThread(m_hVideoInputThread, 0xDEAD);
    }
    if (m_hVideoProcessThread && WaitForSingleObject(m_hVideoProcessThread, 10000) == WAIT_TIMEOUT) {
      ASSERT(FALSE);
      TerminateThread(m_hVideoProcessThread, 0xDEAD);
    }
    if (m_hVideoPresentThread && WaitForSingleObject(m_hVideoPresentThread, 10000) == WAIT_TIMEOUT) {
      ASSERT(FALSE);
      TerminateThread(m_hVideoPresentThread, 0xDEAD);
    }

  }
}

DWORD __stdcall CRenderThread::VideoInput(LPVOID lpParam)
{
  SetThreadName(DWORD(-1), "CRenderThread::VideoInputThread");
  CRenderThread* pThis = (CRenderThread*)lpParam;
  pThis->VideoInputThread();
  return 0;
}

DWORD __stdcall CRenderThread::VideoProcess(LPVOID lpParam)
{
  SetThreadName(DWORD(-1), "CRenderThread::VideoProcessThread");
  CRenderThread* pThis = (CRenderThread*)lpParam;
  pThis->VideoProcessThread();
  return 0;
}

DWORD __stdcall CRenderThread::VideoPresent(LPVOID lpParam)
{
  SetThreadName(DWORD(-1), "CRenderThread::VideoPresentThread");
  CRenderThread* pThis = (CRenderThread*)lpParam;
  pThis->VideoPresentThread();
  return 0;
}

void CRenderThread::VideoInputThread()
{
  HANDLE   hEvts[] = { m_hVideoThreadQuit,m_hVideoThreadFlush };
  bool     bQuit = false;
  while (!bQuit) {
    DWORD dwObject = WaitForMultipleObjects(_countof(hEvts), hEvts, FALSE, 1);
    switch (dwObject) {
    case WAIT_OBJECT_0:
      bQuit = true;
      break;
    case WAIT_OBJECT_0 + 1:
      FlushInput();

      ResetEvent(m_hVideoThreadFlush);
      break;
    case WAIT_TIMEOUT: {
      ProcessInputQueue();
    }
    break;
    }
  }
}

void CRenderThread::VideoProcessThread()
{
  HANDLE   hEvts[] = { m_hVideoThreadQuit ,m_hVideoThreadFlush };
  bool     bQuit = false;
  while (!bQuit) {
    DWORD dwObject = WaitForMultipleObjects(_countof(hEvts), hEvts, FALSE, 1);
    switch (dwObject) {
    case WAIT_OBJECT_0:
      bQuit = true;
      break;
    case WAIT_OBJECT_0 + 1:
      FlushProcess();

      ResetEvent(m_hVideoThreadFlush);
      break;
    case WAIT_TIMEOUT: {
      ProcessRenderQueue();
    }
                     break;
    }
  }
}

void CRenderThread::VideoPresentThread()
{
  HANDLE   hEvts[] = { m_hVideoThreadQuit,m_hVideoThreadFlush };
  bool     bQuit = false;
  while (!bQuit) {
    DWORD dwObject = WaitForMultipleObjects(_countof(hEvts), hEvts, FALSE, 1);
    switch (dwObject) {
    case WAIT_OBJECT_0:
      bQuit = true;
      break;
    case WAIT_OBJECT_0 + 1:
      FlushPresent();

      ResetEvent(m_hVideoThreadFlush);
      break;
    case WAIT_TIMEOUT: {
      ProcessPresentQueue();
    }
    break;
    }
  }
  
}

HRESULT CRenderThread::GetInputSample(CVideoBuffer& current)
{
  CAutoLock lock(&m_VideoInputLock);
  if (m_pReadyInputRenderQueue.size() == 0)
  {
    DLog(L"CD3D12InputQueue::GetReadySample empty");
    return E_FAIL;
  }

  current = m_pReadyInputRenderQueue.front();
  m_pReadyInputRenderQueue.pop();
  return S_OK;
}

void CRenderThread::ReturnInputSample(CVideoBuffer& buf)
{
  m_pInputRenderQueue.push(buf);
}

void CRenderThread::FreeVideoBuffer()
{
#define FreeBuffer(x) x.pVideoTexture.Release();x.pVideoTexture =nullptr; x.pShaderResource.Release();x.pShaderResource = nullptr;
#define BufferLoop(y) if (y.size() > 0){for(;;){CVideoBuffer buf = y.front(); FreeBuffer(buf) y.pop(); if(y.size()==0){break;}}}
  BufferLoop(m_pInputRenderQueue)
  BufferLoop(m_pReadyInputRenderQueue)
  BufferLoop(m_pProcessRenderQueue)
  BufferLoop(m_pReadyProcessRenderQueue)
  BufferLoop(m_pPresentQueue)
  BufferLoop(m_pReadyPresentQueue)

#undef BufferLoop
#undef FreeBuffer
}

void CRenderThread::FlushInput()
{
}

void CRenderThread::FlushPresent()
{
}

void CRenderThread::FlushProcess()
{
}

HRESULT CRenderThread::ProcessInputQueue()
{
  CAutoLock lock(&m_VideoInputLock);
  HRESULT hr = GetInputSample(m_pCurInputProcessed);
  if (FAILED(hr))
    return S_OK;
  //CAutoLock lock(this);
  
  //CVideoBuffer outBuf = m_pInputRenderQueue.front();
  //outBuf.rtDuration =m_pCurInputProcessed.rtDuration;
  //outBuf.rtStart = m_pCurInputProcessed.rtStart;
  //outBuf.rtStop = m_pCurInputProcessed.rtStop;
  //clear the video resource and set it as a render target since were rendering everything onto it
  
  //should be the window rect viewport
  if (m_destRect.Width() == 0)
    m_destRect = m_srcRect;
  ProcessShader(m_pCurInputProcessed);
  
  ReturnInputSample(m_pCurInputProcessed);
  //put the output surface in the ready queue and remove it form the input queue
  //m_pProcessRenderQueue.push(outBuf);
  //m_pInputRenderQueue.pop();
  return S_OK;
}

HRESULT CRenderThread::ProcessRenderQueue()
{
  CAutoLock lock(&m_VideoInputLock);
  HRESULT hr = (m_pProcessRenderQueue.size() > 0) ? S_OK : E_FAIL;
  if (FAILED(hr))
    return hr;
  CVideoBuffer buf = m_pProcessRenderQueue.front();

  hr = ProcessShader(buf);
  if (SUCCEEDED(hr))
  {
    m_pProcessRenderQueue.pop();
    m_pReadyProcessRenderQueue.push(buf);
  }
  
    

  
}

HRESULT CRenderThread::ProcessPresentQueue()
{

  return E_NOTIMPL;
}

HRESULT CRenderThread::GetPresentSurface(CVideoBuffer* buf)
{
  //CAutoLock lock(this);
  if (m_pReadyInputRenderQueue.size() == 0)
  {
    //DLog(L"CD3D12InputQueue::GetReadySample empty");
    return E_FAIL;
  }
  //*buf = m_pReadyInputRenderQueue.front();
  //m_pReadyRenderQueue.pop();
  return S_OK;
}
void CRenderThread::ReleaseSurface(CVideoBuffer buf)
{
  //CAutoLock lock(this);
  //m_pInputRenderQueue.push(buf);
}

#if 0
CD3D12InputQueue::CD3D12InputQueue(CDX12VideoProcessor* pInputProcessor)
  : m_pInputProcessor(pInputProcessor)
{
  m_pRenderThread = new CRenderThread(pInputProcessor,this);
  
  m_pInputQueueLock = new CCritSec();
  m_iQueueSize = 0;
}

CD3D12InputQueue::~CD3D12InputQueue()
{
  
  ReleaseBuffers();
  m_pInputQueueLock = nullptr;
}

void CD3D12InputQueue::InitHWBuffers(int size, int width, int height, DXGI_FORMAT fmt)
{
  for (int x = 0; x < size; x++)
  {


  }
}

void CD3D12InputQueue::InitSWBuffers()
{
  CAutoLock lck(m_pInputQueueLock);
  for (int x = 0; x < m_iQueueSize; x++)
  {
    {
      CVideoBuffer buf = {};
      std::wstring strInputSurface;
      strInputSurface = fmt::format(L"Input surface nbr {}", x);
      //todo hdr
      buf.videoSurface.Create(strInputSurface, m_srcRect.Width(), m_srcRect.Height(), 1, DXGI_FORMAT_R8G8B8A8_UNORM);
      m_pInputBufferQueue.push(buf);
    }

  }
  
  //todo fix buffer size;
  m_pRenderThread->Init(m_iQueueSize, m_srcRect, m_dstRect);
  m_pRenderThread->StartThread();
}

void CD3D12InputQueue::ReleaseBuffers()
{
  CAutoLock lck(m_pInputQueueLock);
  while (true)
  {
    if (m_pInputBufferQueue.size() == 0)
      break;
    CVideoBuffer buf = m_pInputBufferQueue.front();
    buf.videoSurface.DestroyBuffer();
    m_pInputBufferQueue.pop();
  }
  while (true)
  {
    if (m_pReadyBufferQueue.size() == 0)
      break;
    CVideoBuffer buf = m_pReadyBufferQueue.front();
    buf.videoSurface.DestroyBuffer();
    m_pReadyBufferQueue.pop();
  }
  
}

void CD3D12InputQueue::Flush()
{
}

HRESULT CD3D12InputQueue::Init(int bufferCount,CRect srcRect, int srcPitch, FmtConvParams_t srcParams, CONSTANT_BUFFER_VAR bufferVar)
{
  m_srcRect = srcRect;
  m_srcPitch = srcPitch;
  m_srcParams = srcParams;
  m_pBufferVar = bufferVar;
  if (m_iQueueSize != bufferCount)
  {
    m_iQueueSize = bufferCount;
    InitSWBuffers();
  }
  

  return S_OK;
}

void CD3D12InputQueue::Resize(int size)
{
  CAutoLock lck(m_pInputQueueLock);
  ReleaseBuffers();
  m_iQueueSize = size;
  InitSWBuffers();
}


HRESULT CD3D12InputQueue::CopySample(IMediaSample* pSample, CRect dstRect)
{
  HRESULT hr = S_OK;
  m_dstRect = dstRect;
  //check if its a hw decoder from lavfilter
  if (CComQIPtr<IMediaSampleD3D12> pMSD3D12 = pSample)
  {
    CComQIPtr<ID3D12Resource> pD3D12Resource;
    int index = 0;
    hr = pMSD3D12->GetD3D12Texture(&pD3D12Resource, &index);
    if (FAILED(hr)) {
      DLog(L"CDX12VideoProcessor::CopySample() : GetD3D11Texture() failed with error {}", HR2Str(hr));
      return hr;
    }
    REFERENCE_TIME start, stop;
    pSample->GetTime(&start, &stop);
    GpuResource gpuResource = GpuResource(pD3D12Resource, D3D12_RESOURCE_STATE_COPY_SOURCE);
    hr = CopySampleHW(gpuResource, start,stop,(stop-start));
  }
  else if (CComQIPtr<IMFGetService> pService = pSample)
  {
    //TODO
    assert(0);
  }
  else
  {
    BYTE* data = nullptr;
    const long size = pSample->GetActualDataLength();
    if (size > 0 && S_OK == pSample->GetPointer(&data)) {
      // do not use UpdateSubresource for D3D11 VP here
      // because it can cause green screens and freezes on some configurations
      REFERENCE_TIME start, stop;
      pSample->GetTime(&start, &stop);
      hr = CopySampleSW(data, m_srcPitch, size, start,stop);

    }
  }
  return hr;
}

HRESULT CD3D12InputQueue::CopySampleSW(const BYTE* srcData, const int srcPitch, size_t bufferlength, REFERENCE_TIME start, REFERENCE_TIME stop)
{
  HRESULT hr = S_FALSE;

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2];
  UINT64 pitch_plane[2];
  UINT rows_plane[2];
  UINT64 RequiredSize;
  D3D12_RESOURCE_DESC desc;
  if (m_srcParams.DX11Format == DXGI_FORMAT_NV12 || m_srcParams.DX11Format == DXGI_FORMAT_P010)
  {
    desc = CD3DX12_RESOURCE_DESC::Tex2D(m_srcParams.DX11Format, m_srcRect.Width(), m_srcRect.Height(), 1, 1);
    D3D12Engine::g_Device->GetCopyableFootprints(&desc, 0, 2, 0, layoutplane, rows_plane, pitch_plane, &RequiredSize);
    layoutplane[0].Footprint.Format = m_srcParams.pDX11Planes->FmtPlane1;
    layoutplane[1].Footprint.Format = m_srcParams.pDX11Planes->FmtPlane2;
  }

  size_t firstplane = pitch_plane[0] * m_srcRect.Height();

  if (m_pPlaneTexture[0].GetWidth() > 0)
  {
    m_pPlaneTexture[0].Destroy();
    m_pPlaneTexture[1].Destroy();
    m_pPlaneTexture[0].Create2D(srcPitch, m_srcRect.Width(), m_srcRect.Height(), m_srcParams.pDX11Planes->FmtPlane1, srcData);
    m_pPlaneTexture[1].Create2D(srcPitch, layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, m_srcParams.pDX11Planes->FmtPlane2, srcData + firstplane);
  }
  else
  {
    m_pPlaneTexture[0].Create2D(srcPitch, m_srcRect.Width(), m_srcRect.Height(), m_srcParams.pDX11Planes->FmtPlane1, srcData);
    m_pPlaneTexture[1].Create2D(srcPitch, layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, m_srcParams.pDX11Planes->FmtPlane2, srcData + firstplane);
  }

  CAutoLock lck(m_pInputQueueLock);
  if (m_pInputBufferQueue.size() == 0)
  {
    DLog(L"InputBufferFull trigger rendering");
    return E_FAIL;
  }

  GraphicsContext& Context = GraphicsContext::Begin(L"Copy Input SW Context");

  Context.SetRootSignature(ImageScaling::s_PresentRSColor);
  Context.SetPipelineState(ImageScaling::ColorConvertNV12PS);
  Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_BUFFER_VAR), &m_pBufferVar);

  Context.TransitionResource(m_pPlaneTexture[0], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  Context.TransitionResource(m_pPlaneTexture[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

  CVideoBuffer dest = m_pInputBufferQueue.front();

  //set timing info
  dest.rtDuration = stop-start;
  dest.rtStart = start;
  dest.rtStop = stop;
  Context.TransitionResource(dest.videoSurface, D3D12_RESOURCE_STATE_RENDER_TARGET);
  Context.SetRenderTarget(dest.videoSurface.GetRTV());
  if (m_dstRect.Width() == 0 && m_dstRect.Height() == 0)
    Context.SetViewportAndScissor(0, 0, m_dstRect.Width(), m_dstRect.Height());
  else
    Context.SetViewportAndScissor(m_dstRect.left, m_dstRect.top, m_dstRect.Width(), m_dstRect.Height());
  Context.SetDynamicDescriptor(0, 0, m_pPlaneTexture[0].GetSRV());
  Context.SetDynamicDescriptor(0, 1, m_pPlaneTexture[1].GetSRV());
  Context.Draw(3);
  Context.Finish(true);
  m_pReadyBufferQueue.push(dest);
  m_pInputBufferQueue.pop();

  return S_OK;
}

HRESULT CD3D12InputQueue::CopySampleHW(GpuResource resource, REFERENCE_TIME start, REFERENCE_TIME stop, REFERENCE_TIME duration)
{
	D3D12_RESOURCE_DESC desc;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT layoutplane[2];
  UINT64 pitch_plane[2];
  UINT rows_plane[2];
  //UINT64 RequiredSize;

  //get the input resource description
  desc = resource->GetDesc();
  //Get the footprint for the planes
	D3D12Engine::g_Device->GetCopyableFootprints(&desc,
		0, 2, 0, layoutplane, rows_plane, pitch_plane,NULL);

	//we can't use the format from the footprint its always a typeless format which can't be handled by the copy
	if (desc.Format != DXGI_FORMAT_P010 && m_pPlaneResource[0].GetWidth() == 0)// DXGI_FORMAT_NV12
	{
		m_pPlaneResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, DXGI_FORMAT_R8_UNORM);
		m_pPlaneResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, DXGI_FORMAT_R8G8_UNORM);
		
	}
	else if (m_pPlaneResource[0].GetWidth() == 0)
	{
		m_pPlaneResource[0].Create(L"Scaling Resource", layoutplane[0].Footprint.Width, layoutplane[0].Footprint.Height, 1, DXGI_FORMAT_R16_UNORM);
		m_pPlaneResource[1].Create(L"Scaling Resource", layoutplane[1].Footprint.Width, layoutplane[1].Footprint.Height, 1, DXGI_FORMAT_R16G16_UNORM);
	}

  CAutoLock lck(m_pInputQueueLock);
  if (m_pInputBufferQueue.size() == 0)
  {
    DLog(L"InputBufferFull trigger rendering");
    return E_FAIL;
  }
	GraphicsContext& Context = GraphicsContext::Begin(L"Copy Input Context");

	for (int i = 0; i < 2; i++)
	{
    Context.CopyTextureRegion(m_pPlaneResource[i], -1, resource, i);
	}

  Context.SetRootSignature(ImageScaling::s_PresentRSColor);
  Context.SetPipelineState(ImageScaling::ColorConvertNV12PS);
  Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  Context.SetDynamicConstantBufferView(1, sizeof(CONSTANT_BUFFER_VAR), &m_pBufferVar);

  Context.TransitionResource(m_pPlaneResource[0], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  Context.TransitionResource(m_pPlaneResource[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  
  CVideoBuffer dest = m_pInputBufferQueue.front();
  
  //set timing info
  dest.rtDuration = duration;
  dest.rtStart = start;
  dest.rtStop = stop;
  Context.TransitionResource(dest.videoSurface, D3D12_RESOURCE_STATE_RENDER_TARGET);
  Context.SetRenderTarget(dest.videoSurface.GetRTV());
  if (m_dstRect.Width() == 0 && m_dstRect.Height() == 0)
    Context.SetViewportAndScissor(0, 0, m_dstRect.Width(), m_dstRect.Height());
  else
    Context.SetViewportAndScissor(m_dstRect.left, m_dstRect.top, m_dstRect.Width(), m_dstRect.Height());
  Context.SetDynamicDescriptor(0, 0, m_pPlaneResource[0].GetSRV());
  Context.SetDynamicDescriptor(0, 1, m_pPlaneResource[1].GetSRV());
  Context.Draw(3);
  Context.Finish(true);
  m_pReadyBufferQueue.push(dest);
  m_pInputBufferQueue.pop();
	return S_OK;
}

 HRESULT CD3D12InputQueue::GetReadySample(CVideoBuffer* buf)
{
  CAutoLock lock(m_pInputQueueLock);
  if (m_pReadyBufferQueue.size() == 0)
  {
    //DLog(L"CD3D12InputQueue::GetReadySample empty");
    return E_FAIL;
  }
  *buf = m_pReadyBufferQueue.front();
  m_pReadyBufferQueue.pop();
  return S_OK;
}

 void CD3D12InputQueue::ReturnReadySample(CVideoBuffer buf)
 {
   m_pInputBufferQueue.push(buf);
 }


 CD3D12PresentThread::CD3D12PresentThread(CRenderThread* pRenderThread)
 {
   m_pRenderThread = pRenderThread;
   Create();
 }

 DWORD CD3D12PresentThread::ThreadProc()
 {
   Utility::SetThreadName(-1, "CD3D12PresentThread BackBufferCopy");
   
   for (DWORD cmd = (DWORD)-1;; cmd = GetRequest())
   {
     if (cmd == CMD_EXIT)
     {
       Reply(S_OK);
       //m_ePlaybackInit.Set();
       return 0;
     }
     //m_ePlaybackInit.Reset();

     if (cmd != (DWORD)-1)
       Reply(S_OK);

     // Wait for the end of any flush
     //m_eEndFlush.Wait();
     //m_ePlaybackInit.Set();
     HRESULT hr = S_OK;
     while (SUCCEEDED(hr) && !CheckRequest(&cmd))
     {
       hr = ProcessPresentQueue();
     }

     // If we didnt exit by request, deliver end-of-stream
     if (!CheckRequest(&cmd))
     {
       assert(0);
     }
   }
   assert(0);
   return 0;
 }
 

 HRESULT CD3D12PresentThread::ProcessPresentQueue()
 {
   CVideoBuffer buf;
   HRESULT hr = m_pRenderThread->GetPresentSurface(&buf);
   if (FAILED(hr))
     return S_OK;
   GraphicsContext& Context = GraphicsContext::Begin(L"Present Back Buffer");
   D3D12Engine::PresentBackBuffer(Context, buf.videoSurface);
   Context.Finish();

   D3D12Engine::WaitForVBlank();

   g_bPresentT = true;
   D3D12Engine::Present();

   g_bPresentT = false;
   m_pRenderThread->ReleaseSurface(buf);
   return hr;
   /*
   if (m_pReadyBufferQueue.size() == 0)
   {
     DLog(L"CD3D12InputQueue::GetReadySample empty");
     return E_FAIL;
   }
   *buf = m_pReadyBufferQueue.front();
   m_pReadyBufferQueue.pop();
   return S_OK;*/
 
 }
#endif