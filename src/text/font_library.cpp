#include "text/font_library.h"
#include <stdexcept>

namespace demo
{

    FontLibrary::FontLibrary()
    {
        if (FT_Init_FreeType(&lib_) != 0)
        {
            throw std::runtime_error("FT_Init_FreeType failed");
        }
    }

    FontLibrary::~FontLibrary()
    {
        if (lib_)
        {
            FT_Done_FreeType(lib_);
            lib_ = nullptr;
        }
    }

} // namespace demo
