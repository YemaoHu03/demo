#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "text/font_spec.h"
#include "text/font_face.h"

namespace demo
{

    /**
     * @brief 单个字形的栅格化结果（灰度 coverage bitmap）。
     *
     * coverage 是 8-bit（0..255）覆盖率；绘制时再与文本颜色 alpha 相乘。
     */
    struct GlyphBitmap
    {
        int w = 0;
        int h = 0;
        int left = 0;                       // glyph bitmap left bearing
        int top = 0;                        // glyph bitmap top bearing
        int advance = 0;                    // pen advance (px)
        int pitch = 0;                      // bitmap pitch（每行字节数）
        std::vector<std::uint8_t> coverage; // pitch * h
    };

    /**
     * @brief 字形缓存 Key。
     * 以 codepoint + FontSpec 作为 key，确保不同字号/粗斜体不会互相污染。
     */
    struct GlyphKey
    {
        std::uint32_t cp = 0;
        int size_px = 0;
        bool bold = false;
        bool italic = false;

        bool operator==(const GlyphKey &o) const
        {
            return cp == o.cp && size_px == o.size_px && bold == o.bold && italic == o.italic;
        }
    };

    struct GlyphKeyHash
    {
        std::size_t operator()(const GlyphKey &k) const noexcept;
    };

    /**
     * @brief GlyphCache：负责从 FreeType 拉取字形并缓存。
     *
     * 性能定位：
     * - 开启缓存时：同样字符、同样样式、同样字号的字形只栅格化一次
     * - 大多数 UI 渲染体系都依赖 glyph cache 来避免每帧重复栅格化
     */
    class GlyphCache
    {
    public:
        explicit GlyphCache(FontFace &face);

        /**
         * @brief 获取指定 codepoint 的字形 bitmap（缓存命中则直接返回）。
         *
         * 注意：
         * - 本实现使用 FT_Render_Glyph 得到灰度 coverage
         * - bold 使用 FT_GlyphSlot_Embolden 进行模拟（不是加载真实 Bold face）
         * - italic 使用 FT_Set_Transform 做 shear 模拟（不是加载真实 Italic face）
         */
        const GlyphBitmap &get(std::uint32_t cp, const FontSpec &spec);

    private:
        FontFace &face_;
        std::unordered_map<GlyphKey, GlyphBitmap, GlyphKeyHash> cache_;
    };

} // namespace demo
