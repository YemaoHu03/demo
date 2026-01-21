#include "image/ppm_writer.h"
#include <cstdio>
#include <cstddef>

namespace demo
{

    void PpmWriter::saveP6(const std::string &path, const RgbaSurface &img)
    {
        FILE *f = std::fopen(path.c_str(), "wb");
        if (!f)
            return;

        // P6 header
        std::fprintf(f, "P6\n%d %d\n255\n", img.width(), img.height());

        // 写入 RGB（忽略 alpha）
        for (int y = 0; y < img.height(); ++y)
        {
            for (int x = 0; x < img.width(); ++x)
            {
                const std::uint8_t *p = img.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(img.width()) + static_cast<std::size_t>(x)) * 4u;
                std::fputc(p[0], f);
                std::fputc(p[1], f);
                std::fputc(p[2], f);
            }
        }

        std::fclose(f);
    }

} // namespace demo
