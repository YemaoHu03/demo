#include "text/text_layout.h"
#include "core/utf8.h"
#include <algorithm>

namespace demo
{

    TextLayout::TextLayout(GlyphCache &cache) : cache_(cache) {}

    TextLayout::Metrics TextLayout::measure(const std::string &utf8, const FontSpec &spec, int padding)
    {
        // 区域说明：
        // 1) 遍历每个 codepoint，从 glyph cache 获取 metrics
        // 2) 累积 glyph bbox，得到整体 min/max
        // 3) 计算最终 surface 尺寸与 baseline 位置

        int pen_x = padding;

        int min_x = 1e9, min_y = 1e9;
        int max_x = -1e9, max_y = -1e9;

        const char *p = utf8.data();
        const char *end = p + utf8.size();

        while (p < end)
        {
            std::uint32_t cp;
            if (!utf8_next(p, end, cp))
                break;

            const GlyphBitmap &g = cache_.get(cp, spec);

            // glyph bbox 坐标系约定：
            // - baseline 在 y=0
            // - glyph bitmap 左上角为 (pen_x + left, -top)
            int gx0 = pen_x + g.left;
            int gy0 = -g.top;
            int gx1 = gx0 + g.w;
            int gy1 = gy0 + g.h;

            min_x = std::min(min_x, gx0);
            min_y = std::min(min_y, gy0);
            max_x = std::max(max_x, gx1);
            max_y = std::max(max_y, gy1);

            pen_x += g.advance;
        }

        // 空字符串或全空 glyph 的防御：给一个最小尺寸
        if (max_x < min_x || max_y < min_y)
        {
            return Metrics{padding * 2, padding * 2, padding};
        }

        // 右/下额外留 padding，避免贴边裁切
        int w = (max_x - min_x) + padding;
        int h = (max_y - min_y) + padding;

        // baseline 在小图中的位置：让 (y=0) 落在小图内部
        int baseline = -min_y;

        return Metrics{w, h, baseline};
    }

} // namespace demo
