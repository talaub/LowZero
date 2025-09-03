#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    LOW_EXPORT void initialize();
    LOW_EXPORT void tick(float p_Delta);
    LOW_EXPORT void cleanup();

    struct Project
    {
      String dataPath;
      String rootPath;
      String assetCachePath;
      String editorImagesPath;

      String engineRootPath;
      String engineDataPath;
    };

    LOW_EXPORT const Project &get_project();
  } // namespace Util
} // namespace Low
