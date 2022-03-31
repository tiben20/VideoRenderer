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

#include "Color.h"
#include "Math/Vector.h"
#include "BufferManager.h"
#include <string>

class Color;
class GraphicsContext;

namespace GeometryRenderer
{
  void Initialize(void);
  void Shutdown(void);
  
}

__declspec(align(16)) struct POINTVERTEX12 {
  DirectX::XMFLOAT3 Pos;
  DirectX::XMFLOAT4 Color;
};

struct GraphRectangle
{
  D3DCOLOR graphcolor;
  RECT graphrect;
};

struct GraphLine
{
  POINT linepoints[2];
  D3DCOLOR linecolor;
  UINT linesize;
  SIZE linertsize;
};

class GeometryContext
{
public:
  GeometryContext(GraphicsContext& CmdContext);

  GraphicsContext& GetCommandContext() const { return m_Context; }

  //
  // Rendering commands
  //

  // Begin and end drawing commands
  void Begin(bool EnableHDR = false);
  void End(void);
  void DrawQuadrilateral(const SIZE& rtSize);
  void DrawRectangle(const RECT& rect, const SIZE& rtSize, const D3DCOLOR color);
  void DrawLine(GraphLine line);
  void DrawGFPoints(int Xstart, int Xstep, int Yaxis, int Yscale, int* Ydata, UINT Yoffset, const UINT size, const D3DCOLOR color, SIZE& newRTSize);
protected:
  bool m_bAlphaBlend = false;
  POINTVERTEX12 m_Vertices[4] = {};
  HRESULT SetupVertex(const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const float x4, const float y4, const D3DCOLOR color);

private:

  GraphicsContext& m_Context;
  BOOL m_HDR;
};
