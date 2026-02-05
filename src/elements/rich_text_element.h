#pragma once
#include <string>
#include "core/geometry.h"

namespace demo
{

    /**
     * @brief 富文本元素：使用 Chromium 渲染 HTML/CSS 成小图，再贴到大图。
     */
    struct RichTextElement
    {
        std::string html;
        Point position;
        int width = 0;
        int height = 0;
    };

} // namespace demo
