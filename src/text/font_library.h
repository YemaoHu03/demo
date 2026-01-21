#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H

namespace demo
{

    /**
     * @brief FreeType 库句柄 RAII 封装。
     *
     * 该对象在程序生命周期内通常只需要 1 个实例，
     * 用于创建 FontFace（FT_Face）。
     */
    class FontLibrary
    {
    public:
        FontLibrary();
        ~FontLibrary();

        FontLibrary(const FontLibrary &) = delete;
        FontLibrary &operator=(const FontLibrary &) = delete;

        FT_Library raw() const { return lib_; }

    private:
        FT_Library lib_ = nullptr;
    };

} // namespace demo
