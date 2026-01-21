#pragma once
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace demo
{

    /**
     * @brief 字体 face（FT_Face）RAII 封装。
     *
     * 说明：
     * - 支持 .ttf/.otf/.ttc
     * - 对于 .ttc，可能需要指定 face_index（同一个文件中有多个字体 face）
     */
    class FontFace
    {
    public:
        FontFace(FT_Library lib, const std::string &font_path, int face_index = 0);
        ~FontFace();

        FontFace(const FontFace &) = delete;
        FontFace &operator=(const FontFace &) = delete;

        void setPixelSize(int px);
        FT_Face raw() const { return face_; }

    private:
        FT_Face face_ = nullptr;
    };

} // namespace demo
