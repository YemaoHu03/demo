#pragma once
#include <cstdint>

namespace demo
{

    /**
     * @brief RGBA 颜色（8-bit）。
     *
     * 约定：
     * - r/g/b/a 范围均为 [0,255]
     * - a=0 表示全透明；a=255 表示不透明
     */
    struct Color
    {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 0;
    };

} // namespace demo
