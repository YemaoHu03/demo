#pragma once
#include <string>

#include "core/color.h"
#include "image/rgba_surface.h"
#include "text/font_spec.h"
#include "text/glyph_cache.h"
#include "text/text_layout.h"

namespace demo
{

    /**
     * @brief 文本栅格化器：把“文本 + 样式”渲染成一个小图（RgbaSurface）。
     *
     * 职责边界：
     * - measure：由 TextLayout 完成
     * - glyph 获取：由 GlyphCache 完成
     * - 绘制：由本类完成（底色填充 + 覆盖率 bitmap 混合到 RGBA）
     */
    class TextRasterizer
    {
    public:
        TextRasterizer(GlyphCache &cache, TextLayout &layout);

        /**
         * @brief 渲染文本到一个新的 surface（小图）。
         *
         * @param utf8      文本内容（UTF-8）
         * @param spec      字体规格（size/bold/italic）
         * @param bg        文本底色（用于小图背景）
         * @param fg        文本颜色
         * @param padding   小图内边距（避免字形裁切）
         */
        RgbaSurface renderToSurface(
            const std::string &utf8,
            const FontSpec &spec,
            Color bg,
            Color fg,
            int padding);

    private:
        /**
         * @brief 将灰度 coverage bitmap 绘制到 RGBA surface 上（与 fg 做 alpha 混合）。
         */
        static void drawGlyphCoverage(RgbaSurface &dst, int x, int y, const GlyphBitmap &g, Color fg);

    private:
        GlyphCache &cache_;
        TextLayout &layout_;
    };

} // namespace demo
