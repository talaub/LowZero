#include "LowUtilLogger.h"

#include "LowAssetManagerImage.h"

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>

#include <stdint.h>
#include <stdlib.h>

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char *pName, int flags, unsigned debugFlags,
                     const char *file, int line)
{
  return malloc(size);
}

int main()
{
  LOW_LOG_INFO("Assetprep");

  Low::AssetManager::Image::Image2D l_Image;

  Low::AssetManager::Image::load_png(
      "L:\\zero\\data\\raw_assets\\image2d\\wood_box.png", l_Image);

  Low::AssetManager::Image::process("P:\\data\\assets\\img2d\\out_wb", l_Image);

  return 0;
}
