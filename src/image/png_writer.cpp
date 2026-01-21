#include "image/png_writer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

namespace demo
{

    bool PngWriter::save(const std::string &path, const RgbaSurface &img)
    {
        if (img.width() <= 0 || img.height() <= 0)
            return false;

        const int stride = img.width() * 4;
        return stbi_write_png(path.c_str(), img.width(), img.height(), 4, img.data(), stride) != 0;
    }

} // namespace demo
