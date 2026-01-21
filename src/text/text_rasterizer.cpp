#include "text/text_rasterizer.h"
#include "core/utf8.h"
#include <cstddef>
#include <cstdint>

namespace demo
{

    TextRasterizer::TextRasterizer(GlyphCache &cache, TextLayout &layout)
        : cache_(cache), layout_(layout) {}

    RgbaSurface TextRasterizer::renderToSurface(
        const std::string &utf8,
        const FontSpec &spec,
        Color bg,
        Color fg,
        int padding)
    {
        // 区域说明：
        // 1) measure 计算小图尺寸与 baseline
        // 2) 创建小图并填充底色
        // 3) 遍历 glyph，按 baseline 与 bearing 定位并绘制 coverage

        TextLayout::Metrics m = layout_.measure(utf8, spec, padding);

        RgbaSurface out(m.width, m.height);
        // out.clear(bg);

        int pen_x = padding;
        int base_y = m.baseline_y;

        const char *p = utf8.data();
        const char *end = p + utf8.size();

        while (p < end)
        {
            std::uint32_t cp;
            if (!utf8_next(p, end, cp))
                break;

            const GlyphBitmap &g = cache_.get(cp, spec);

            // glyph 绘制位置：
            // x = pen_x + left
            // y = baseline - top
            int gx = pen_x + g.left;
            int gy = base_y - g.top;

            drawGlyphCoverage(out, gx, gy, g, fg);
            pen_x += g.advance;
        }

        return out;
    }

    void TextRasterizer::drawGlyphCoverage(RgbaSurface &dst, int x, int y, const GlyphBitmap &g, Color fg)
    {
        if (g.w <= 0 || g.h <= 0)
            return;
        if (g.coverage.empty() || g.pitch == 0)
            return;

        // 区域说明：
        // - coverage 是 0..255，表示该像素对 glyph 的覆盖率
        // - 实际写入 alpha = fg.a * coverage
        // - 与 dst 原有像素做 straight alpha blend

        for (int yy = 0; yy < g.h; ++yy)
        {
            int py = y + yy;
            if (py < 0 || py >= dst.height())
                continue;

            const std::uint8_t *row = g.coverage.data() + static_cast<std::size_t>(yy) * static_cast<std::size_t>(g.pitch);

            for (int xx = 0; xx < g.w; ++xx)
            {
                int px = x + xx;
                if (px < 0 || px >= dst.width())
                    continue;

                std::uint8_t cov = row[xx];
                if (cov == 0)
                    continue;

                // 有效 alpha：fg.a 与 coverage 相乘
                std::uint8_t srcA = static_cast<std::uint8_t>((static_cast<int>(fg.a) * static_cast<int>(cov) + 127) / 255);
                int invA = 255 - static_cast<int>(srcA);

                std::size_t i = (static_cast<std::size_t>(py) * static_cast<std::size_t>(dst.width()) + static_cast<std::size_t>(px)) * 4u;
                std::uint8_t *dp = dst.data() + i;

                std::uint8_t dR = dp[0], dG = dp[1], dB = dp[2], dA = dp[3];

                dp[0] = static_cast<std::uint8_t>((static_cast<int>(fg.r) * srcA + static_cast<int>(dR) * invA + 127) / 255);
                dp[1] = static_cast<std::uint8_t>((static_cast<int>(fg.g) * srcA + static_cast<int>(dG) * invA + 127) / 255);
                dp[2] = static_cast<std::uint8_t>((static_cast<int>(fg.b) * srcA + static_cast<int>(dB) * invA + 127) / 255);
                dp[3] = static_cast<std::uint8_t>((255 * static_cast<int>(srcA) + static_cast<int>(dA) * invA + 127) / 255);
            }
        }
    }

} // namespace demo
