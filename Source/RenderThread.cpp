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

void CRenderThread::FreeVideoBuffer()
{
  m_pInputBuffers->FreeBuffers();
  m_pProcessBuffers->FreeBuffers();
  m_pPresentBuffers->FreeBuffers();
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
  CVideoBuffer inBuf;
  CVideoBuffer outBuf;
  HRESULT hr = m_pInputBuffers->GetReadyBuffer(inBuf);
  if (FAILED(hr))
    return hr;
  if (FAILED(m_pProcessBuffers->GetFreeBuffer(outBuf)))
  {
    //return the the input buffer for later use
    m_pInputBuffers->ReturnReadyBuffer(inBuf);
    return E_FAIL;
  }
  //set the timing that was provided by the sample
  outBuf.rtDuration = inBuf.rtDuration;
  outBuf.rtStart = inBuf.rtStart;
  outBuf.rtStop = inBuf.rtStop;
  
  
  //should be the window rect viewport
  if (m_destRect.Width() == 0)
    m_destRect = m_srcRect;
  hr = ProcessShader(inBuf,outBuf);
  if (FAILED(hr))
  {
    assert(0);
  }
  //those can't fail since there no process behind it
  m_pInputBuffers->ReturnFreeBuffer(inBuf);
  m_pProcessBuffers->SetBufferReady(outBuf);
  return S_OK;
}

HRESULT CRenderThread::ProcessRenderQueue()
{
  CVideoBuffer inBuf;
  CVideoBuffer outBuf;
  HRESULT hr = m_pProcessBuffers->GetReadyBuffer(inBuf);
  if (FAILED(hr))
    return hr;
  if (FAILED(m_pPresentBuffers->GetFreeBuffer(outBuf)))
  {
    //return the the input buffer for later use
    m_pProcessBuffers->ReturnReadyBuffer(inBuf);
    return E_FAIL;
  }
  //set the timing that was provided by the sample
  outBuf.rtDuration = inBuf.rtDuration;
  outBuf.rtStart = inBuf.rtStart;
  outBuf.rtStop = inBuf.rtStop;


  //should be the window rect viewport
  if (m_destRect.Width() == 0)
    m_destRect = m_srcRect;

  hr = ProcessSubs(inBuf, outBuf);

  if (FAILED(hr))
  {
    assert(0);
  }
  //those can't fail since there no process behind it
  m_pProcessBuffers->ReturnFreeBuffer(inBuf);
  m_pPresentBuffers->SetBufferReady(outBuf);
  return S_OK;
}

HRESULT CRenderThread::ProcessPresentQueue()
{
  CVideoBuffer inBuf;
  HRESULT hr = m_pPresentBuffers->GetReadyBuffer(inBuf);
  if (FAILED(hr))
    return hr;
  
  hr = ProcessPresents(inBuf);
  //hr = ProcessShader(inBuf, outBuf);
  if (FAILED(hr))
  {
    assert(0);
  }
  //those can't fail since there no process behind it
  m_pPresentBuffers->ReturnFreeBuffer(inBuf);
  
  return S_OK;
  
}
