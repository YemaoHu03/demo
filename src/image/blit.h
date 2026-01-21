#pragma once
#include "image/rgba_surface.h"

namespace demo
{

    /**
     * @brief 贴图合成（src -> dst），执行 alpha blending。
     *
     * src 与 dst 均为 RGBA：
     * - src.a=0 的像素跳过
     * - 使用 straight alpha 混合
     *
     * 这是你 demo 中“贴图”阶段的核心计时点之一。
     */
    class Blit
    {
    public:
        /**
         * @brief 将 src 贴到 dst 的 (dst_x, dst_y) 左上角位置。
         */
        static void alphaBlend(RgbaSurface &dst, const RgbaSurface &src, int dst_x, int dst_y);
    };

} // namespace demo
