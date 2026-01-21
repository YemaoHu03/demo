#pragma once
#include <cstdint>
#include <vector>
#include "core/color.h"
#include "core/geometry.h"

namespace demo
{

    /**
     * @brief 一个简单的 RGBA 位图（CPU 内存），用于：
     * - 大画布（背景黑色）
     * - 元素小图（文本底色 + 文本）
     *
     * 设计点：
     * - 使用 std::vector<uint8_t> 连续存储
     * - 像素布局为 RGBA，byte order 固定
     * - 提供最小够用的清屏与填充接口
     */
    class RgbaSurface
    {
    public:
        RgbaSurface() = default;
        RgbaSurface(int w, int h);

        void reset(int w, int h);

        int width() const { return w_; }
        int height() const { return h_; }

        std::uint8_t *data() { return px_.data(); }
        const std::uint8_t *data() const { return px_.data(); }

        /**
         * @brief 用指定颜色清空整个 surface。
         * 注意：这是逐像素写入，O(W*H)。
         */
        void clear(Color c);

        /**
         * @brief 填充矩形区域（越界会自动裁剪）。
         */
        void fillRect(const Rect &r, Color c);

    private:
        inline void setPixel(int x, int y, Color c);

    private:
        int w_ = 0;
        int h_ = 0;
        std::vector<std::uint8_t> px_; // RGBA
    };

} // namespace demo
