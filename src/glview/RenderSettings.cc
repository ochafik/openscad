#include "RenderSettings.h"

RenderSettings *RenderSettings::inst(bool erase)
{
  static auto instance = new RenderSettings;
  if (erase) {
    delete instance;
    instance = nullptr;
  }
  return instance;
}

RenderSettings::RenderSettings()
{
  manifoldEnabled = true;
  openCSGTermLimit = 100000;
  far_gl_clip_limit = 100000.0;
  img_width = 512;
  img_height = 512;
  colorscheme = "Cornfield";
}
