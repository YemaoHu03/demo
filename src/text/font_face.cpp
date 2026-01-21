#include "text/font_face.h"
#include <stdexcept>

namespace demo
{

    FontFace::FontFace(FT_Library lib, const std::string &font_path, int face_index)
    {
        if (FT_New_Face(lib, font_path.c_str(), face_index, &face_) != 0)
        {
            throw std::runtime_error("FT_New_Face failed (font not found or invalid face_index)");
        }
    }

    FontFace::~FontFace()
    {
        if (face_)
        {
            FT_Done_Face(face_);
            face_ = nullptr;
        }
    }

    void FontFace::setPixelSize(int px)
    {
        FT_Set_Pixel_Sizes(face_, 0, px);
    }

} // namespace demo
