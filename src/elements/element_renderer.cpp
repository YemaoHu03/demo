#include "elements/element_renderer.h"

namespace demo
{

    ElementRenderer::ElementRenderer(TextRasterizer &textRasterizer)
        : textRasterizer_(textRasterizer) {}

    RgbaSurface ElementRenderer::renderTextElement(const TextElement &e)
    {
        // 区域说明：
        // - 这里是“元素级别”的渲染：TextElement -> 小图
        // - 具体的文字布局/字形绘制全部交给 TextRasterizer

        return textRasterizer_.renderToSurface(
            e.utf8_text,
            e.style.font,
            e.style.bg_color,
            e.style.fg_color,
            e.style.padding);
    }

} // namespace demo
