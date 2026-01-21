#include "image/rgba_surface.h"
#include <algorithm>
#include <cstddef>

namespace demo
{

    RgbaSurface::RgbaSurface(int w, int h)
    {
        reset(w, h);
    }

    void RgbaSurface::reset(int w, int h)
    {
        w_ = w;
        h_ = h;
        px_.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u, 0);
    }

    void RgbaSurface::clear(Color c)
    {
        // 将 RGBA 按内存顺序打包（R,G,B,A 对应低到高字节；小端机器上常见）
        const std::uint32_t v =
            (std::uint32_t)c.r |
            ((std::uint32_t)c.g << 8) |
            ((std::uint32_t)c.b << 16) |
            ((std::uint32_t)c.a << 24);

        auto *p = reinterpret_cast<std::uint32_t *>(px_.data());
        const std::size_t n = (std::size_t)w_ * (std::size_t)h_;
        std::fill(p, p + n, v);
    }

    void RgbaSurface::fillRect(const Rect &r, Color c)
    {
        // 区域说明：边界裁剪，防止越界写
        int x0 = std::max(0, r.x);
        int y0 = std::max(0, r.y);
        int x1 = std::min(w_, r.x + r.w);
        int y1 = std::min(h_, r.y + r.h);
        // 将 RGBA 按内存顺序打包（R,G,B,A 对应低到高字节；小端机器上常见）
        const std::uint32_t v =
            (std::uint32_t)c.r |
            ((std::uint32_t)c.g << 8) |
            ((std::uint32_t)c.b << 16) |
            ((std::uint32_t)c.a << 24);

        auto *p = reinterpret_cast<std::uint32_t *>(px_.data());
        const std::size_t n = (std::size_t)x1 * (std::size_t)y1;

    }

    inline void RgbaSurface::setPixel(int x, int y, Color c)
    {
        if (x < 0 || y < 0 || x >= w_ || y >= h_)
            return;
        std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w_) + static_cast<std::size_t>(x)) * 4u;
        px_[i + 0] = c.r;
        px_[i + 1] = c.g;
        px_[i + 2] = c.b;
        px_[i + 3] = c.a;
    }

} // namespace demo
