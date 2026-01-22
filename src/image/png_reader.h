#pragma once
#include <string>

#include "image/rgba_surface.h"

namespace demo
{

    /**
     * @brief 读取 PNG 文件到 RGBA surface。
     */
    class PngReader
    {
    public:
        static bool load(const std::string &path, RgbaSurface &out);
    };

} // namespace demo
