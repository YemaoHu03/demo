#pragma once
#include <string>
#include "image/rgba_surface.h"

namespace demo
{

    /**
     * @brief 将 RGBA surface 输出为 PPM(P6)。
     *
     * 注意：
     * - PPM 不支持 alpha，本实现只输出 RGB。
     * - 目的主要是快速验证渲染效果，避免引入 PNG 依赖。
     */
    class PpmWriter
    {
    public:
        static void saveP6(const std::string &path, const RgbaSurface &img);
    };

} // namespace demo
