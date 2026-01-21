#pragma once
#include "elements/text_element.h"
#include "image/rgba_surface.h"
#include "text/text_rasterizer.h"

namespace demo
{

    /**
     * @brief 元素渲染器：将“元素描述”转成“可贴图的小图”。
     *
     * 该层的意义：
     * - 主渲染流程（scene）不应该关心文本渲染细节
     * - 不同元素类型在这里分发到不同的渲染路径
     */
    class ElementRenderer
    {
    public:
        explicit ElementRenderer(TextRasterizer &textRasterizer);

        /**
         * @brief 渲染一个文本元素到小图。
         */
        RgbaSurface renderTextElement(const TextElement &e);

    private:
        TextRasterizer &textRasterizer_;
    };

} // namespace demo
