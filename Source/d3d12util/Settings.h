/*
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

#pragma once


#include <string>
#include <map>
#include <variant>
#include "../external/tinyxml2/tinyxml2.h"

class XMLElement;
/*!
 \brief Wrapper around CSettingsManager responsible for properly setting up
 the settings manager and registering all the callbacks, handlers and custom
 setting types.
 \sa CSettingsManager
 */
class CSettings
{

public:
  CSettings();
  ~CSettings();
  static constexpr auto SETTING_RENDERER_DOUBLERATE = "renderer.interlace.doublerate";
  static constexpr auto SETTING_RENDERER_UPSCALER = "renderer.upscaler";
  static constexpr auto SETTING_RENDERER_DOWNSCALER = "renderer.downscaler";
  static constexpr auto SETTING_RENDERER_CHROMAUPSAMPLER = "renderer.chromaupsampler";
  static constexpr auto SETTING_RENDERER_SWAPEFFECT = "renderer.swapeffect";
  static constexpr auto SETTING_RENDERER_EXCLUSIVEFULLSCREEN = "renderer.exclusivefullscreen";
  static constexpr auto SETTING_RENDERER_WAITVBLANK = "renderer.waitvblank";
  static constexpr auto SETTING_RENDERER_SHOWSTATS = "renderer.showstats";
  static constexpr auto SETTING_RENDERER_USEDITHER = "renderer.usedither";
  static constexpr auto SETTING_RENDERER_HDR_PASSTHROUGH = "renderer.hdrpassthrough";
  static constexpr auto SETTING_RENDERER_HDR_TOGGLEDISPLAY = "renderer.hdrtoggledisplay";
  static constexpr auto SETTING_RENDERER_CONVERTTOSDR = "renderer.converttosdr";
  static constexpr auto SETTING_RENDERER_TEXTURE_FORMAT = "renderer.textureformat";
  static constexpr auto SETTING_RENDERER_RESIZESTATS = "renderer.resizestats";
  static constexpr auto SETTING_RENDERER_INTERPORLATEAT50PCT = "renderer.interpolateat50pct";
  
  
  // values for SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS
  static const int RENDERER_SWAPMETHOD_DISCARD = 0;
  static const int RENDERER_SWAPMETHOD_FLIP = 1;
  
  // values for SETTING_VIDEOLIBRARY_ARTWORK_LEVEL
  static const int RENDERER_TEXFMT_AUTOINT = 0;
  static const int RENDERER_TEXFMT_8INT = 1;
  static const int RENDERER_TEXFMT_10INT = 2;
  static const int RENDERER_TEXFMT_16FLOAT = 3;

  // values for SETTING_VIDEOLIBRARY_ARTWORK_LEVEL
  static const int RENDERER_HDRTD_OFF = 0;
  static const int RENDERER_HDRTD_FULLSCREEN = 1;
  static const int RENDERER_HDRTD_ALWAYS = 2;

  static CSettings& GetInstance();

  
  bool Initialize();

  
  bool Load();
  bool Save();

  /*!
   \brief Loads setting values from the given (XML) file.

   \param file Path to an XML file containing setting values
   \return True if the setting values were successfully loaded, false otherwise
   */
  bool Load(const std::string& file);
  /*!
  \brief Loads setting values from the given XML element.

  \param root XML element containing setting values
  \return True if the setting values were successfully loaded, false otherwise
  */
  bool Load(const tinyxml2::XMLElement* root);
 
  /*!
   \brief Saves the setting values to the given (XML) file.

   \param file Path to an XML file
   \return True if the setting values were successfully saved, false otherwise
   */
  bool Save(const std::string& file);
  /*!
   \brief Saves the setting values to the given XML node.

   \param root XML node
   \return True if the setting values were successfully saved, false otherwise
   */
  bool Save(tinyxml2::XMLNode* root) const;

  /*!
   \brief Loads the setting being represented by the given XML node with the
   given identifier.

   \param node XML node representing the setting to load
   \param settingId Setting identifier
   \return True if the setting was successfully loaded from the given XML node, false otherwise
   */
  bool LoadSetting(const tinyxml2::XMLNode* node, const std::string& settingId);

  // overwrite (not override) from CSettingsBase
  bool GetBool(const std::string& id) const;
  void SetBool(const std::string& id, bool value);

  bool ToggleBool(const std::string& id);

  int GetInt(const std::string& id) const;
  bool SetInt(const std::string& id, int value);

  double GetNumber(const std::string& id) const;
  bool SetNumber(const std::string& id, double value);

  std::string GetString(const std::string& id) const;
  bool SetString(const std::string& id, const std::string& value);

  bool CreateSettingsFile();
protected:
  bool m_initialized = false;
  CCritSec m_critical;
private:
  CSettings(const CSettings&) = delete;
  CSettings const& operator=(CSettings const&) = delete;

  bool Load(const tinyxml2::XMLElement* root, bool& updated);

  // implementation of ISubSettings
  bool Load(const tinyxml2::XMLNode* settings);

  bool Initialize(const std::string& file);
  bool Reset();

  std::shared_ptr<CSettings> m_settings;
  std::string m_pSettingsPath;

  std::map<std::string, std::variant<int, bool,double, std::string>> m_pSettingsMap;
  std::map<std::string, std::variant<int, bool, double, std::string>>::iterator SettingsIterator;
};
