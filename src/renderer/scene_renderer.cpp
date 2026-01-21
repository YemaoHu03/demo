#include "renderer/scene_renderer.h"
#include "image/blit.h"

namespace demo
{

    SceneRenderer::SceneRenderer(int canvas_w, int canvas_h)
        : canvas_(canvas_w, canvas_h) {}

    void SceneRenderer::renderFrame(const std::vector<TextElement> &elements, ElementRenderer &elementRenderer)
    {
        // 区域说明：1) 清屏（纯黑底）
        canvas_.clear(Color{0, 0, 0, 255});

        // 区域说明：2) 渲染并贴图每个元素
        for (const auto &e : elements)
        {
            RgbaSurface small = elementRenderer.renderTextElement(e);
            Blit::alphaBlend(canvas_, small, e.position.x, e.position.y);
        }
    }

    void SceneRenderer::renderFramePrepared(const std::vector<PreparedBlitItem> &items, Color clear_color)
    {
        canvas_.clear(clear_color);

        for (const auto &it : items)
        {
            Blit::alphaBlend(canvas_, it.surface, it.position.x, it.position.y);
        }
    }

} // namespace demo
