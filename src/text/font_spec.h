#pragma once

namespace demo
{

    /**
     * @brief 字体规格（与“元素样式”解耦）。
     *
     * 设计动机：
     * - font 模块尽量不要依赖 elements 模块
     * - GlyphCache 只关心 size/bold/italic 这类“字体栅格化参数”
     */
    struct FontSpec
    {
        int size_px = 36;
        bool bold = false;
        bool italic = false;
    };

} // namespace demo
