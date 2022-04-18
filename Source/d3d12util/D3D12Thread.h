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

#include <d3d12.h>
#include <d3d12video.h>
#include "d3dx12.h"
#include "PipelineState.h"
#include "ColorBuffer.h"
#include "gpubuffer.h"
#include "CommandContext.h"
#include "uploadbuffer.h"
#include "ImageScaling.h"
#include "DX12VideoProcessor.h"
struct VideoBuffer
{
  REFERENCE_TIME rtDuration;
  REFERENCE_TIME rtStart;
  REFERENCE_TIME rtStop;
  ColorBuffer    videoSurface;
};
class CDX12VideoProcessor;
class CD3D12InputQueue;
class CD3D12PresentThread;

class CD3D12RenderThread : 
  protected CAMThread
  , public  CCritSec
{
public:
  CD3D12RenderThread(CDX12VideoProcessor* pInputProcessor, CD3D12InputQueue* pInputQueue);
  virtual ~CD3D12RenderThread();

  HRESULT Init(int bufferCount, CRect srcRect, CRect destRect);
  void SetDestRect(CRect dstRect);
  void Flush();
  void ReleaseBuffers();
  void Resize(int size);
  bool StartThread()
  {
    return Create();
  };

  HRESULT GetPresentSurface(VideoBuffer* buf);
  void ReleaseSurface(VideoBuffer buf);
protected:
  CDX12VideoProcessor* m_pInputProcessor;
  CD3D12InputQueue* m_pInputQueue;
  CD3D12PresentThread* m_pPresentThread;
private:
  
  std::queue<VideoBuffer> m_pInputRenderQueue;
  std::queue<VideoBuffer> m_pReadyRenderQueue;
  VideoBuffer m_pCurrentlyProcessed;
  CAMEvent m_ePlaybackInit{ TRUE };
  CRect m_srcRect;
  CRect m_destRect;
  // flushing
  bool m_fFlushing = FALSE;
  CAMEvent m_eEndFlush;
  int m_iQueueSize = 0;
protected:
  enum
  {
    CMD_EXIT,
    CMD_FLUSH
  };
  DWORD ThreadProc();

  HRESULT ProcessInputQueue();
};

class CD3D12InputQueue
{
public:
  CD3D12InputQueue(CDX12VideoProcessor* pInputProcessor);
  virtual ~CD3D12InputQueue();

  HRESULT Init(int bufferCount, CRect srcRect, int srcPitch, FmtConvParams_t srcParams, CONSTANT_BUFFER_VAR bufferVar);
  void Flush();
  void ReleaseBuffers();
  void Resize(int size);
  //called from the processor which is coming from the input pin
  HRESULT CopySample(IMediaSample* pSample, CRect dstRect);
  //
  HRESULT GetReadySample(VideoBuffer* buf);
  void    ReturnReadySample(VideoBuffer buf);
protected:
  CDX12VideoProcessor* m_pInputProcessor = nullptr;
private:
  CD3D12RenderThread* m_pRenderThread;

  void InitHWBuffers(int size, int width, int height, DXGI_FORMAT fmt);
  void InitSWBuffers();

  

  HRESULT CopySampleSW(const BYTE* srcData, const int srcPitch, size_t bufferlength, REFERENCE_TIME start, REFERENCE_TIME stop);
  HRESULT CopySampleHW(GpuResource resource, REFERENCE_TIME start, REFERENCE_TIME stop, REFERENCE_TIME duration);

  //D3D Resource
  //We only need 2 they are only used to copy the resource coming from the filter
  ColorBuffer m_pPlaneResource[2];//HW
  Texture m_pPlaneTexture[2];//SW
  //Contant buffer to ajust color used when merging plane
  CONSTANT_BUFFER_VAR m_pBufferVar;

  std::queue<VideoBuffer> m_pInputBufferQueue;
  std::queue<VideoBuffer> m_pReadyBufferQueue;
  //lock for when we play with the queues
  CCritSec* m_pInputQueueLock;
  int m_iQueueSize = 0;
  
  //input param
  FmtConvParams_t m_srcParams = {};
  int m_srcPitch;
  CRect m_srcRect;
  CRect m_dstRect;

  bool m_bQueueInit = false;
};

class CD3D12PresentThread :
  protected CAMThread
{
public:
  CD3D12PresentThread(CD3D12RenderThread* pRenderThread);
  virtual ~CD3D12PresentThread() {}

  HRESULT ProcessPresentQueue();

protected:
  enum
  {
    CMD_EXIT,
    CMD_FLUSH
  };
  DWORD ThreadProc();


private:
  CD3D12RenderThread* m_pRenderThread;
};
