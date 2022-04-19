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

#include <string>

class Color;
class GraphicsContext;

namespace TextRenderer
{
  void Initialize(void);
  void Shutdown(void);
  HRESULT LoadFont(const WCHAR* strFontName, const UINT fontHeight, const UINT fontFlags);
  SIZE GetMaxCharMetric();
}
__declspec(align(16)) struct CONSTANT_BUFFER_COLOR {
  DirectX::XMFLOAT4 col;
};

class FontContext
{
public:
  FontContext(GraphicsContext& CmdContext);

  GraphicsContext& GetCommandContext() const { return m_Context; }

  //
  // Rendering commands
  //

  // Begin and end drawing commands
  void Begin(bool EnableHDR = false);
  void End(void);

  void DrawText(const SIZE& rtSize, float sx, float sy, D3DCOLOR color, const WCHAR* strText);

protected:
  bool m_bAlphaBlend = false;
  
  //POINTVERTEX12 m_Vertices[4] = {};

private:
  
  GraphicsContext& m_Context;
  BOOL m_HDR;
};
