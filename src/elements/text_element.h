#pragma once
#include <string>

#include "core/color.h"
#include "core/geometry.h"
#include "text/font_spec.h"

namespace demo
{

    /**
     * @brief 文本元素样式。
     *
     * 说明：
     * - FontSpec 专注字体栅格化参数
     * - bg/fg 负责元素底色与文字颜色
     * - padding 决定小图留白，避免字形裁切
     */
    struct TextStyle
    {
        FontSpec font;
        Color bg_color{0, 0, 0, 0};
        Color fg_color{255, 255, 255, 255};
        int padding = 8;
    };

    /**
     * @brief 文本元素：描述“要渲染什么、贴到哪里、样式是什么”。
     *
     * 这是“元素系统”的最小单元之一；
     * 未来你可以增加 ImageElement、TableElement 等。
     */
    struct TextElement
    {
        std::string utf8_text;
        TextStyle style;
        Point position; // 贴到大图的左上角坐标
    };

} // namespace demo
