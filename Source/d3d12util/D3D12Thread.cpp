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
#include "D3D12Thread.h"
#include "../Include/ID3DVideoMemoryConfiguration.h"
#include <mfapi.h>
#include <Mfidl.h>
#include "ImageScaling.h"//todo remove
CD3D12RenderThread::CD3D12RenderThread(CDX12VideoProcessor* pInputProcessor, CD3D12InputQueue* pInputQueue)
  : m_pInputProcessor(pInputProcessor)
  , m_pInputQueue(pInputQueue)
{
  m_ePlaybackInit.Set();
  m_iQueueSize = 0;
  m_pPresentThread = new CD3D12PresentThread(this);
}

CD3D12RenderThread::~CD3D12RenderThread()
{
}

void CD3D12RenderThread::SetDestRect(CRect dstRect)
{
  m_destRect = dstRect;
}

HRESULT CD3D12RenderThread::Init(int bufferCount, CRect srcRect,CRect destRect)
{
  CAutoLock cAutoLock(this);
  m_srcRect = srcRect;
  m_destRect = destRect;
  if (bufferCount != m_iQueueSize)
  {
    ReleaseBuffers();
    m_iQueueSize = bufferCount;
    for (int x = 0; x < m_iQueueSize; x++)
    {
      {
        VideoBuffer buf = {};
        std::wstring strInputSurface;
        strInputSurface = fmt::format(L"Render Surface nbr {}", x);
        //todo hdr
        buf.videoSurface.Create(strInputSurface, m_srcRect.Width(), m_srcRect.Height(), 1, DXGI_FORMAT_R8G8B8A8_UNORM);
        m_pInputRenderQueue.push(buf);
      }

    }
  }
  return S_OK;
}
void CD3D12RenderThread::Flush()
{
}

void CD3D12RenderThread::ReleaseBuffers()
{
  
  while (true)
  {
    if (m_pInputRenderQueue.size() == 0)
      break;
    VideoBuffer buf = m_pInputRenderQueue.front();
    buf.videoSurface.DestroyBuffer();
    m_pInputRenderQueue.pop();
  }
  while (true)
  {
    if (m_pReadyRenderQueue.size() == 0)
      break;
    VideoBuffer buf = m_pReadyRenderQueue.front();
    buf.videoSurface.DestroyBuffer();
    m_pReadyRenderQueue.pop();
  }
}

void CD3D12RenderThread::Resize(int size)
{
  Init(size, m_srcRect, m_destRect);
}

DWORD CD3D12RenderThread::ThreadProc()
{
  Utility::SetThreadName(-1, "CD3D12RenderThread ShaderProcess");
  m_fFlushing = false;
  m_eEndFlush.Set();
  for (DWORD cmd = (DWORD)-1;; cmd = GetRequest())
  {
    if (cmd == CMD_EXIT)
    {
      Reply(S_OK);
      m_ePlaybackInit.Set();
      return 0;
    }
    m_ePlaybackInit.Reset();
    
    if (cmd != (DWORD)-1)
      Reply(S_OK);
    
    // Wait for the end of any flush
    m_eEndFlush.Wait();
    m_ePlaybackInit.Set();
    HRESULT hr = S_OK;
    while (SUCCEEDED(hr) && !CheckRequest(&cmd))
    {
      
      hr = ProcessInputQueue();

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

HRESULT CreateVertex(const UINT srcW, const UINT srcH, const RECT& srcRect, VERTEX_SUBPIC vertices[4])
{

  const float src_dx = 1.0f / srcW;
  const float src_dy = 1.0f / srcH;
  float src_l = src_dx * srcRect.left;
  float src_r = src_dx * srcRect.right;
  const float src_t = src_dy * srcRect.top;
  const float src_b = src_dy * srcRect.bottom;

  POINT points[4];
  points[0] = { -1, -1 };
  points[1] = { -1, +1 };
  points[2] = { +1, -1 };
  points[3] = { +1, +1 };

  vertices[0].Pos = { (float)points[0].x, (float)points[0].y, 0 };
  vertices[0].TexCoord = { src_l, src_b };
  vertices[1].Pos = { (float)points[1].x, (float)points[1].y, 0 };
  vertices[1].TexCoord = { src_l, src_t };
  vertices[2].Pos = { (float)points[2].x, (float)points[2].y, 0 };
  vertices[2].TexCoord = { src_r, src_b };
  vertices[3].Pos = { (float)points[3].x, (float)points[3].y, 0 };
  vertices[3].TexCoord = { src_r, src_t };

  return S_OK;
}

HRESULT CD3D12RenderThread::ProcessInputQueue()
{
  
  HRESULT hr = m_pInputQueue->GetReadySample(&m_pCurrentlyProcessed);
  if (FAILED(hr))
    return S_OK;
  //CAutoLock lock(this);
  DLog(L"m_pCurrentlyProcessed is {}", m_pCurrentlyProcessed.videoSurface.GetName());
  if (m_pCurrentlyProcessed.videoSurface.GetName() == L"Input surface nbr 0")
    DLog(L"m_pCurrentlyProcessed is {}", m_pCurrentlyProcessed.videoSurface.GetName());
  GraphicsContext& pVideoContext = GraphicsContext::Begin(L"Video Scaling");
  VideoBuffer outBuf = m_pInputRenderQueue.front();
  outBuf.rtDuration =m_pCurrentlyProcessed.rtDuration;
  outBuf.rtStart = m_pCurrentlyProcessed.rtStart;
  outBuf.rtStop = m_pCurrentlyProcessed.rtStop;
  //clear the video resource and set it as a render target since were rendering everything onto it

  //should be the window rect viewport
  if (m_destRect.Width() == 0)
    m_destRect = m_srcRect;
  pVideoContext.SetViewportAndScissor(0, 0, m_destRect.Width(), m_destRect.Height());

  CRect rSrc = m_srcRect;

  if (rSrc.Width() != m_destRect.Width() && rSrc.Height() != m_destRect.Height())
  {
    /*if (rSrc.Width() > m_destRect.Width() || rSrc.Height() > m_destRect.Height())
    {
      std::wstring scurrentscaler = D3D12Engine::g_D3D12Options->GetCurrentDownscaler();
      D3D12Engine::Downscale(pVideoContext, m_srcRect, m_destRect, m_bSWRendering, scurrentscaler);
    }
    else
    {
      std::wstring scurrentscaler = D3D12Engine::g_D3D12Options->GetCurrentUpscaler();
      D3D12Engine::Upscale(pVideoContext, srcRect, dstRect, m_bSWRendering, scurrentscaler);
    }*/
  }
  else
  {
    pVideoContext.TransitionResource(outBuf.videoSurface, D3D12_RESOURCE_STATE_RENDER_TARGET);
    pVideoContext.SetRenderTarget(outBuf.videoSurface.GetRTV());
    pVideoContext.ClearColor(outBuf.videoSurface);

    if (m_destRect.Width() > 0 && m_destRect.Height() > 0)
      pVideoContext.SetViewportAndScissor(m_destRect.left, m_destRect.top, m_destRect.Width(), m_destRect.Height());
    //even with no scale we need a dstrect if we resize the window only in x or y position
    pVideoContext.SetRootSignature(ImageScaling::s_SubPicRS);
    pVideoContext.SetPipelineState(ImageScaling::VideoRessourceCopyPS);
    pVideoContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);// D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pVideoContext.TransitionResource(m_pCurrentlyProcessed.videoSurface, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    pVideoContext.SetViewportAndScissor(0, 0, m_pCurrentlyProcessed.videoSurface.GetWidth(), m_pCurrentlyProcessed.videoSurface.GetHeight());
    pVideoContext.SetDynamicDescriptor(0, 0, m_pCurrentlyProcessed.videoSurface.GetSRV());
    pVideoContext.SetRenderTarget(outBuf.videoSurface.GetRTV());

    
    VERTEX_SUBPIC subpicvertex[4];

    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = outBuf.videoSurface.GetWidth();
    rect.bottom = outBuf.videoSurface.GetHeight();
    CreateVertex(outBuf.videoSurface.GetWidth(), outBuf.videoSurface.GetHeight(), rect, subpicvertex);
    pVideoContext.SetDynamicVB(0, 4, sizeof(VERTEX_SUBPIC), subpicvertex);

    pVideoContext.SetViewportAndScissor(rect.left, rect.top, rect.Width(), rect.Height());

    pVideoContext.Draw(4);
    
    
  }
  //we have a sample lets render it
  m_pInputProcessor->RenderSubpics(outBuf.videoSurface, pVideoContext);

  pVideoContext.Finish(true);

  //return the current video surface

  m_pInputQueue->ReturnReadySample(m_pCurrentlyProcessed);
  //put the output surface in the ready queue and remove it form the input queue
  m_pReadyRenderQueue.push(outBuf);
  m_pInputRenderQueue.pop();
  return S_OK;
}

HRESULT CD3D12RenderThread::GetPresentSurface(VideoBuffer* buf)
{
  CAutoLock lock(this);
  if (m_pReadyRenderQueue.size() == 0)
  {
    //DLog(L"CD3D12InputQueue::GetReadySample empty");
    return E_FAIL;
  }
  *buf = m_pReadyRenderQueue.front();
  m_pReadyRenderQueue.pop();
  return S_OK;
}
void CD3D12RenderThread::ReleaseSurface(VideoBuffer buf)
{
  CAutoLock lock(this);
  m_pInputRenderQueue.push(buf);
}

CD3D12InputQueue::CD3D12InputQueue(CDX12VideoProcessor* pInputProcessor)
  : m_pInputProcessor(pInputProcessor)
{
  m_pRenderThread = new CD3D12RenderThread(pInputProcessor,this);
  
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
      VideoBuffer buf = {};
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
    VideoBuffer buf = m_pInputBufferQueue.front();
    buf.videoSurface.DestroyBuffer();
    m_pInputBufferQueue.pop();
  }
  while (true)
  {
    if (m_pReadyBufferQueue.size() == 0)
      break;
    VideoBuffer buf = m_pReadyBufferQueue.front();
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

  VideoBuffer dest = m_pInputBufferQueue.front();

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
  
  VideoBuffer dest = m_pInputBufferQueue.front();
  
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

 HRESULT CD3D12InputQueue::GetReadySample(VideoBuffer* buf)
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

 void CD3D12InputQueue::ReturnReadySample(VideoBuffer buf)
 {
   m_pInputBufferQueue.push(buf);
 }


 CD3D12PresentThread::CD3D12PresentThread(CD3D12RenderThread* pRenderThread)
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
 bool g_bPresentT = false;
 HRESULT CD3D12PresentThread::ProcessPresentQueue()
 {
   VideoBuffer buf;
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