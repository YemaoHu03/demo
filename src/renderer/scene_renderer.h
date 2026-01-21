#pragma once
#include <vector>

#include "core/color.h"
#include "core/geometry.h"
#include "image/rgba_surface.h"
#include "elements/text_element.h"
#include "elements/element_renderer.h"

namespace demo
{
    /**
     * @brief 预渲染贴图项：用于 blit_only 模式。
     *
     * 说明：
     * - surface：预渲染的小图（例如文本元素渲染结果）
     * - position：该小图贴到大图的左上角坐标
     */
    struct PreparedBlitItem
    {
        RgbaSurface surface;
        Point position;
    };

    /**
     * @brief 场景渲染器：负责“大图”的渲染流程。
     *
     * 你的 demo 的核心流程就在这里：
     * 1) 大图清成黑底
     * 2) 遍历 elements：渲染成小图
     * 3) 将小图按 position 贴回大图
     *
     * 说明：
     * - 这里刻意保持“流程清晰”，把细节交给更底层模块
     */
    class SceneRenderer
    {
    public:
        SceneRenderer(int canvas_w, int canvas_h);

        /**
         * @brief 渲染一帧（输出写入内部 canvas）。
         */
        void renderFrame(const std::vector<TextElement> &elements, ElementRenderer &elementRenderer);

        /**
         * @brief 仅贴图模式：输入预渲染的小图列表，每帧只清屏 + 贴图。
         */
        void renderFramePrepared(const std::vector<PreparedBlitItem> &items, Color clear_color);

        /**
         * @brief 访问当前帧渲染结果。
         */
        const RgbaSurface &canvas() const { return canvas_; }

    private:
        RgbaSurface canvas_;
    };

} // namespace demo
