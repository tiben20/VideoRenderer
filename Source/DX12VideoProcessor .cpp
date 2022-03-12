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

#include "stdafx.h"
#include <uuids.h>
#include <mfapi.h> // for MR_BUFFER_SERVICE
#include <Mferror.h>
#include <Mfidl.h>
#include <dwmapi.h>
#include "Helper.h"
#include "Times.h"
#include "resource.h"
#include "VideoRenderer.h"
#include "../Include/Version.h"

#include "../Include/ID3DVideoMemoryConfiguration.h"
#include "Shaders.h"
#include "Utils/CPUInfo.h"

#include "../external/minhook/include/MinHook.h"

#include "DX12VideoProcessor .h"

CDX12VideoProcessor::CDX12VideoProcessor(CMpcVideoRenderer* pFilter, const Settings_t& config, HRESULT& hr)
  : CVideoProcessor(pFilter)
{
}

CDX12VideoProcessor::~CDX12VideoProcessor()
{
}

bool CDX12VideoProcessor::Initialized()
{
  return false;
}

void CDX12VideoProcessor::ReleaseVP()
{
}

void CDX12VideoProcessor::ReleaseDevice()
{
}
