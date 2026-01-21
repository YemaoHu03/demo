#include "image/blit.h"
#include <cstddef>
#include <cstdint>

namespace demo
{

    void Blit::alphaBlend(RgbaSurface &dst, const RgbaSurface &src, int dst_x, int dst_y)
    {
        // 区域说明：
        // 1) 遍历 src 全部像素
        // 2) 计算贴到 dst 后的目标坐标
        // 3) 做 straight alpha blend

        for (int y = 0; y < src.height(); ++y)
        {
            int py = dst_y + y;
            if (py < 0 || py >= dst.height())
                continue;

            for (int x = 0; x < src.width(); ++x)
            {
                int px = dst_x + x;
                if (px < 0 || px >= dst.width())
                    continue;

                const std::uint8_t *sp = src.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(src.width()) + static_cast<std::size_t>(x)) * 4u;

                std::uint8_t sR = sp[0], sG = sp[1], sB = sp[2], sA = sp[3];
                if (sA == 0)
                    continue;

                std::uint8_t *dp = dst.data() + (static_cast<std::size_t>(py) * static_cast<std::size_t>(dst.width()) + static_cast<std::size_t>(px)) * 4u;

                std::uint8_t dR = dp[0], dG = dp[1], dB = dp[2], dA = dp[3];

                int invA = 255 - static_cast<int>(sA);

                // straight alpha blending：
                // out = src * a + dst * (1-a)
                dp[0] = static_cast<std::uint8_t>((static_cast<int>(sR) * sA + static_cast<int>(dR) * invA + 127) / 255);
                dp[1] = static_cast<std::uint8_t>((static_cast<int>(sG) * sA + static_cast<int>(dG) * invA + 127) / 255);
                dp[2] = static_cast<std::uint8_t>((static_cast<int>(sB) * sA + static_cast<int>(dB) * invA + 127) / 255);
                dp[3] = static_cast<std::uint8_t>((255 * static_cast<int>(sA) + static_cast<int>(dA) * invA + 127) / 255);
            }
        }
    }

} // namespace demo
