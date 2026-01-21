#pragma once
#include <string>
#include "text/font_spec.h"
#include "text/glyph_cache.h"

namespace demo
{

    /**
     * @brief 文本布局（measure 阶段）。
     *
     * 目标：
     * - 输入一段 UTF-8 文本 + FontSpec
     * - 通过遍历 glyph metrics 估算文本边界
     * - 输出小图所需尺寸与 baseline 位置
     *
     * 注意：
     * - 这里不做复杂 shaping（如 HarfBuzz），适用于中文/中英混排 demo
     */
    class TextLayout
    {
    public:
        struct Metrics
        {
            int width = 0;
            int height = 0;
            int baseline_y = 0; // baseline 在小图内的 y 坐标
        };

        explicit TextLayout(GlyphCache &cache);

        Metrics measure(const std::string &utf8, const FontSpec &spec, int padding);

    private:
        GlyphCache &cache_;
    };

} // namespace demo
