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
#include "DX9Helper.h"
#include "DX11Helper.h"
#include <variant>
struct CVideoInputBuffer {
  Tex11Video_t   pVideoTexture;
  REFERENCE_TIME rtDuration;
  REFERENCE_TIME rtStart;
  REFERENCE_TIME rtStop;
  
};

struct CVideoBuffer {
  std::variant < CComPtr<ID3D11Texture2D>, Tex9Video_t> pVideoTexture;
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
class CVideoBufferManager : public CCritSec
{
  friend class CRenderThread;
public:
  CVideoBufferManager() { };
  ~CVideoBufferManager() { FreeBuffers(); };
  //the alloc is called in each independant buffermanager
  virtual HRESULT Alloc(int buffer, D3D11_TEXTURE2D_DESC texdesc) = 0;

  HRESULT GetFreeBuffer(CVideoBuffer& buf)
  {
    //lock the bufferlist
    CAutoLock lock(this);
    //return no buffer if the list if empty
    if (m_pBufferFreeList.size() == 0)
      return E_FAIL;
    buf = m_pBufferFreeList.front();
    //if we pass S_OK here we will stop requesting buffers
    //The renderer will put buffer in ready state right after this return when its processed
    return S_OK;
  }

  HRESULT GetReadyBuffer(CVideoBuffer& buf)
  {
    //lock the bufferlist
    CAutoLock lock(this);
    if (m_pBufferReadyList.size() == 0)
      return E_FAIL;
    buf = m_pBufferReadyList.front();
    m_pBufferReadyList.pop();
    return S_OK;
  }

  void SetBufferReady(CVideoBuffer buf)
  {
    //lock the bufferlist
    CAutoLock lock(this);
    //this buffer was previously free
    m_pBufferReadyList.push(buf);
  
  }

  //this occur when the buffer was ready but the thread right after didnt have any buffer ready
  //We still want to process it as soon as possible
  void ReturnReadyBuffer(CVideoBuffer buf)
  {
    //lock the bufferlist
    CAutoLock lock(this);
    m_pBufferReadyList.push(buf);

  }

  void ReturnFreeBuffer(CVideoBuffer buf)
  {
    //lock the bufferlist
    CAutoLock lock(this);
    m_pBufferFreeList.push(buf);
  }

  void FreeBuffers()
  {
    CAutoLock lock(this);
    if (m_pBufferFreeList.size() > 0) {
      for (;;) {
        CVideoBuffer buf = m_pBufferFreeList.front();
        std::get<CComPtr<ID3D11Texture2D>>(buf.pVideoTexture).Release();
        buf.pShaderResource.Release();
        std::get<CComPtr<ID3D11Texture2D>>(buf.pVideoTexture) = nullptr;
        buf.pShaderResource = nullptr;
        m_pBufferFreeList.pop();
      }
    }
    if (m_pBufferReadyList.size() > 0) {
      for (;;) {
        CVideoBuffer buf = m_pBufferReadyList.front();
        std::get<CComPtr<ID3D11Texture2D>>(buf.pVideoTexture).Release();
        buf.pShaderResource.Release();
        std::get<CComPtr<ID3D11Texture2D>>(buf.pVideoTexture) = nullptr;
        buf.pShaderResource = nullptr;
        m_pBufferReadyList.pop();
      }
    }
  }
protected:
  
  std::queue<CVideoBuffer> m_pBufferFreeList;
  std::queue<CVideoBuffer> m_pBufferReadyList;
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
  virtual HRESULT ProcessShader(CVideoBuffer& inBuf, CVideoBuffer& outBuf) = 0;
  virtual HRESULT ProcessSubs(CVideoBuffer& inBuf, CVideoBuffer& outBuf) = 0;
  virtual HRESULT ProcessPresents(CVideoBuffer& inBuf) = 0;
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
  CVideoBufferManager* m_pInputBuffers;
  CVideoBufferManager* m_pProcessBuffers;
  CVideoBufferManager* m_pPresentBuffers;

  /*std::queue<CVideoBuffer> m_pInputRenderQueue;
  std::queue<CVideoBuffer> m_pReadyInputRenderQueue;

  std::queue<CVideoBuffer> m_pProcessRenderQueue;
  std::queue<CVideoBuffer> m_pReadyProcessRenderQueue;

  std::queue<CVideoBuffer> m_pPresentQueue;
  std::queue<CVideoBuffer> m_pReadyPresentQueue;*/
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
