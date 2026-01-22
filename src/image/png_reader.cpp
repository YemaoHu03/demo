#include "image/png_reader.h"

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <png.h>

namespace demo
{

    namespace
    {
        struct FileGuard
        {
            std::FILE *fp = nullptr;
            ~FileGuard()
            {
                if (fp)
                    std::fclose(fp);
            }
        };
    } // namespace

    bool PngReader::load(const std::string &path, RgbaSurface &out)
    {
        FileGuard file;
        file.fp = std::fopen(path.c_str(), "rb");
        if (!file.fp)
            return false;

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png_ptr)
            return false;

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
        {
            png_destroy_read_struct(&png_ptr, nullptr, nullptr);
            return false;
        }

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            return false;
        }

        png_init_io(png_ptr, file.fp);
        png_read_info(png_ptr, info_ptr);

        const png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
        const png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
        const int color_type = png_get_color_type(png_ptr, info_ptr);
        const int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        if (bit_depth == 16)
            png_set_strip_16(png_ptr);
        if (color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png_ptr);
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png_ptr);
        if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr);

        png_read_update_info(png_ptr, info_ptr);

        const png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
        std::vector<std::uint8_t> buffer(rowbytes * height);
        std::vector<png_bytep> row_pointers(height);
        for (png_uint_32 y = 0; y < height; ++y)
            row_pointers[y] = buffer.data() + y * rowbytes;

        png_read_image(png_ptr, row_pointers.data());
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

        if (width == 0 || height == 0)
            return false;

        out.reset(static_cast<int>(width), static_cast<int>(height));
        std::uint8_t *dst = out.data();
        const std::size_t size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
        std::memcpy(dst, buffer.data(), size);
        return true;
    }

} // namespace demo
