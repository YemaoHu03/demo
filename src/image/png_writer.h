#pragma once
#include <string>
#include "image/rgba_surface.h"

namespace demo
{

    /**
     * @brief 将 RGBA surface 输出为 PNG。
     *
     * 注意：
     * - 输出包含 alpha
     * - 用于最终结果落盘（调试/交付）
     */
    class PngWriter
    {
    public:
        static bool save(const std::string &path, const RgbaSurface &img);
    };

} // namespace demo
