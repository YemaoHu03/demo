#include "text/glyph_cache.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H // FT_GlyphSlot_Embolden

#include <stdexcept>
#include <utility>

namespace demo
{

    std::size_t GlyphKeyHash::operator()(const GlyphKey &k) const noexcept
    {
        // 一个简单且足够用的 hash 混合
        std::size_t h = static_cast<std::size_t>(k.cp) * 1315423911u;
        h ^= static_cast<std::size_t>(k.size_px) * 2654435761u;
        h ^= (k.bold ? 0x9E3779B97F4A7C15ull : 0ull);
        h ^= (k.italic ? 0xBF58476D1CE4E5B9ull : 0ull);
        return h;
    }

    GlyphCache::GlyphCache(FontFace &face) : face_(face) {}

    const GlyphBitmap &GlyphCache::get(std::uint32_t cp, const FontSpec &spec)
    {
        GlyphKey key{cp, spec.size_px, spec.bold, spec.italic};
        auto it = cache_.find(key);
        if (it != cache_.end())
            return it->second;

        // 区域说明：准备 FreeType（设置字号、选择 glyph index）
        face_.setPixelSize(spec.size_px);
        FT_Face f = face_.raw();
        FT_UInt glyphIndex = FT_Get_Char_Index(f, cp);

        // 区域说明：可选 italic（shear 变换）
        // 注意：FT_Set_Transform 会影响 face 的后续加载，因此要保证在函数内进行设置与复位。
        if (spec.italic)
        {
            FT_Matrix m;
            m.xx = 1 << 16;
            m.xy = static_cast<FT_Fixed>(0.25 * (1 << 16)); // shear 系数可调
            m.yx = 0;
            m.yy = 1 << 16;
            FT_Set_Transform(f, &m, nullptr);
        }
        else
        {
            FT_Set_Transform(f, nullptr, nullptr);
        }

        // 区域说明：加载并渲染 glyph -> bitmap（coverage）
        if (FT_Load_Glyph(f, glyphIndex, FT_LOAD_DEFAULT) != 0)
        {
            // 失败也要缓存一个空 glyph，避免重复失败开销
            auto ins = cache_.emplace(key, GlyphBitmap{});
            return ins.first->second;
        }

        if (FT_Render_Glyph(f->glyph, FT_RENDER_MODE_NORMAL) != 0)
        {
            auto ins = cache_.emplace(key, GlyphBitmap{});
            return ins.first->second;
        }

        // 区域说明：可选 bold（embolden）
        // 注意：这里是“模拟加粗”，不等价于真实 Bold 字体，但对性能 demo 足够。
        if (spec.bold)
        {
            FT_GlyphSlot_Embolden(f->glyph);
        }

        // 区域说明：拷贝 bitmap 到我们自己的缓存结构中（避免 FreeType 内部缓存生命周期问题）
        FT_GlyphSlot g = f->glyph;
        FT_Bitmap &bm = g->bitmap;

        GlyphBitmap gb;
        gb.w = static_cast<int>(bm.width);
        gb.h = static_cast<int>(bm.rows);
        gb.left = g->bitmap_left;
        gb.top = g->bitmap_top;
        gb.advance = static_cast<int>(g->advance.x >> 6);
        gb.pitch = static_cast<int>(bm.pitch);

        if (bm.buffer && gb.w > 0 && gb.h > 0 && gb.pitch != 0)
        {
            gb.coverage.assign(bm.buffer, bm.buffer + static_cast<std::size_t>(gb.pitch) * static_cast<std::size_t>(gb.h));
        }

        // 复位 transform，避免影响后续调用（安全习惯）
        FT_Set_Transform(f, nullptr, nullptr);

        auto ins = cache_.emplace(key, std::move(gb));
        return ins.first->second;
    }

} // namespace demo
