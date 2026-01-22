#pragma once
#include <string>

#include "elements/rich_text_element.h"
#include "image/rgba_surface.h"

namespace demo
{

    /**
     * @brief 通过 Chromium headless 渲染富文本并输出 RGBA surface。
     */
    class RichTextRenderer
    {
    public:
        explicit RichTextRenderer(std::string chromium_path, std::string work_dir = "/tmp");

        RgbaSurface renderRichTextElement(const RichTextElement &element) const;

    private:
        std::string chromium_path_;
        std::string work_dir_;
    };

} // namespace demo
