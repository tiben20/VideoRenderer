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

/*struct CVideoBuffer
{
  REFERENCE_TIME rtDuration;
  REFERENCE_TIME rtStart;
  REFERENCE_TIME rtStop;

  ColorBuffer    videoSurface;
};*/


#include <queue>
#include <d3d11.h>
#include "Helper.h"
#include "DX11Helper.h"

struct CVideoInputBuffer {
  Tex11Video_t   pVideoTexture;
  REFERENCE_TIME rtDuration;
  REFERENCE_TIME rtStart;
  REFERENCE_TIME rtStop;
};

struct CVideoBuffer {
  CComPtr<ID3D11Texture2D> pVideoTexture;
  CComPtr<ID3D11ShaderResourceView> pShaderResource;
  REFERENCE_TIME rtDuration;
  REFERENCE_TIME rtStart;
  REFERENCE_TIME rtStop;
};

enum RENDER_STATE {
  Stopped = State_Stopped,
  Paused = State_Paused,
  Started = State_Running,
  Shutdown = State_Running + 1
};

class CVideoProcessor;

class CRenderThread
{
protected:
  CVideoBuffer m_pCurInputProcessed;
  CVideoProcessor* m_pVideoProcessor;
  CAMEvent m_EventSend;
  CAMEvent m_EventComplete;
  HANDLE                           m_hVideoInputThread = nullptr;
  HANDLE                           m_hVideoProcessThread = nullptr;
  HANDLE                           m_hVideoPresentThread = nullptr;
  HANDLE                           m_hVideoThreadQuit = nullptr;
  HANDLE                           m_hVideoThreadFlush = nullptr;
  bool                             m_bThreadQuit = false;
  bool                             m_bThreadFlush = false;
  CCritSec                         m_VideoPresentLock;
  CCritSec                         m_VideoProcessLock;
  CCritSec                         m_VideoInputLock;
  CCritSec                         m_ThreadsLock;
public:

  CRenderThread();

  virtual ~CRenderThread() = default;

  virtual HRESULT Init(int inputBufferCount, int processBufferCount, int presentBufferCount, FmtConvParams_t param) = 0;
  virtual void Resize(int size) = 0;
  virtual void ReleaseBuffers() = 0;
  virtual void Flush() = 0;
  virtual HRESULT ProcessShader(CVideoBuffer& buf) = 0;
  //Called from present thread
  //virtual HRESULT ProcessPresentQueue() {};
  HRESULT ProcessSample(IMediaSample* pSample);
  virtual HRESULT CopySample(IMediaSample* pSample) = 0;
  void SetDestRect(CRect dstRect);

  bool StartThreads();
  void StopThreads();
  
  
  static DWORD WINAPI              VideoInput(LPVOID lpParam);
  static DWORD WINAPI              VideoProcess(LPVOID lpParam);
  static DWORD WINAPI              VideoPresent(LPVOID lpParam);
  void                             VideoInputThread();
  void                             VideoProcessThread();
  void                             VideoPresentThread();

  HRESULT GetInputSample(CVideoBuffer& current);
  void    ReturnInputSample(CVideoBuffer& buf);

  HRESULT GetPresentSurface(CVideoBuffer* buf);
  void ReleaseSurface(CVideoBuffer buf);
protected:

private:
  
  RENDER_STATE                     m_nRenderState;

  CVideoBuffer m_pCurrentlyProcessed;
  CAMEvent m_ePlaybackInit{ TRUE };
  CRect m_srcRect;
  CRect m_destRect;
  // flushing
  bool m_fFlushing = FALSE;
  CAMEvent m_eEndFlush;
  int m_iQueueSize = 0;
protected:
  void FreeVideoBuffer();
  std::queue<CVideoBuffer> m_pInputRenderQueue;
  std::queue<CVideoBuffer> m_pReadyInputRenderQueue;

  std::queue<CVideoBuffer> m_pProcessRenderQueue;
  std::queue<CVideoBuffer> m_pReadyProcessRenderQueue;

  std::queue<CVideoBuffer> m_pPresentQueue;
  std::queue<CVideoBuffer> m_pReadyPresentQueue;
  enum
  {
    CMD_EXIT,
    CMD_FLUSH
  };
  void FlushInput();
  void FlushPresent();
  void FlushProcess();
  HRESULT ProcessInputQueue();
  HRESULT ProcessRenderQueue();
  HRESULT ProcessPresentQueue();
};
#if 0
class CVideoInputQueue
{
public:
  CVideoInputQueue(CVideoProcessor* pInputProcessor);
  virtual ~CVideoInputQueue();

  HRESULT Init(int bufferCount, CRect srcRect, int srcPitch, FmtConvParams_t srcParams, CONSTANT_BUFFER_VAR bufferVar);
  void Flush();
  void ReleaseBuffers();
  void Resize(int size);
  //called from the processor which is coming from the input pin
  HRESULT CopySample(IMediaSample* pSample, CRect dstRect);
  //
  HRESULT GetReadySample(CVideoBuffer* buf);
  void    ReturnReadySample(CVideoBuffer buf);
protected:
  CVideoProcessor* m_pInputProcessor = nullptr;
private:
  CRenderThread* m_pRenderThread;

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

  std::queue<CVideoBuffer> m_pInputBufferQueue;
  std::queue<CVideoBuffer> m_pReadyBufferQueue;
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

class CVideoPresentQueue :
  protected CAMThread
{
public:
  CVideoPresentQueue(CRenderThread* pRenderThread);
  virtual ~CVideoPresentQueue() {}

  HRESULT ProcessPresentQueue();

protected:
  enum
  {
    CMD_EXIT,
    CMD_FLUSH
  };
  DWORD ThreadProc();


private:
  CRenderThread* m_pRenderThread;
};

#endif