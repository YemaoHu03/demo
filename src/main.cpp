#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "text/font_library.h"
#include "text/font_face.h"
#include "text/glyph_cache.h"
#include "text/text_layout.h"
#include "text/text_rasterizer.h"

#include "elements/text_element.h"
#include "elements/element_renderer.h"
#include "renderer/scene_renderer.h"

#include "image/ppm_writer.h"

using namespace demo;

// 防止 text_only 模式下编译器把“渲染但不用结果”的逻辑优化掉
static volatile std::uint64_t g_sink = 0;

enum class BenchMode
{
    Full,     // render small + blit
    BlitOnly, // pre-render small once; per frame only blit
    TextOnly  // only render small surfaces, no blit
};

struct Options
{
    BenchMode mode = BenchMode::Full;
    int iters = 300;
    int warmup = 30;
    bool save = true;
};

static void printUsage(const char *prog)
{
    std::cerr
        << "Usage: " << prog << " /path/to/font.ttf_or_ttc [face_index] [--mode=full|blit_only|text_only] [--iters=N] [--warmup=N] [--no-save]\n"
        << "Example:\n"
        << "  " << prog << " /root/.local/share/fonts/simsun.ttc 0 --mode=full --iters=300\n"
        << "  " << prog << " /root/.local/share/fonts/simsun.ttc 0 --mode=blit_only\n"
        << "  " << prog << " /root/.local/share/fonts/simsun.ttc 0 --mode=text_only --no-save\n";
}

static bool startsWith(const std::string &s, const std::string &prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

static Options parseOptions(int argc, char **argv, int arg_start_index)
{
    Options opt;

    for (int i = arg_start_index; i < argc; ++i)
    {
        std::string a = argv[i];

        if (startsWith(a, "--mode="))
        {
            std::string v = a.substr(std::string("--mode=").size());
            if (v == "full")
                opt.mode = BenchMode::Full;
            else if (v == "blit_only")
                opt.mode = BenchMode::BlitOnly;
            else if (v == "text_only")
                opt.mode = BenchMode::TextOnly;
            else
            {
                std::cerr << "Unknown mode: " << v << "\n";
                printUsage(argv[0]);
                std::exit(1);
            }
        }
        else if (startsWith(a, "--iters="))
        {
            opt.iters = std::atoi(a.c_str() + std::string("--iters=").size());
        }
        else if (startsWith(a, "--warmup="))
        {
            opt.warmup = std::atoi(a.c_str() + std::string("--warmup=").size());
        }
        else if (a == "--no-save")
        {
            opt.save = false;
        }
        else
        {
            std::cerr << "Unknown argument: " << a << "\n";
            printUsage(argv[0]);
            std::exit(1);
        }
    }

    if (opt.iters <= 0)
        opt.iters = 1;
    if (opt.warmup < 0)
        opt.warmup = 0;
    return opt;
}

int main(int argc, char **argv)
{
    // =========================
    // 参数处理（字体路径必填）
    // =========================
    if (argc < 2)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string font_path = argv[1];

    // face_index：可选，如果 argv[2] 不是数字则默认 0，并把 argv[2] 当作 options 开始
    int face_index = 0;
    int opt_start = 2;

    if (argc >= 3)
    {
        // 简单判断 argv[2] 是否为纯数字（允许 0/1/2...）
        std::string s = argv[2];
        bool is_num = !s.empty();
        for (char c : s)
            is_num = is_num && (c >= '0' && c <= '9');

        if (is_num)
        {
            face_index = std::atoi(argv[2]);
            opt_start = 3;
        }
    }

    Options opt = parseOptions(argc, argv, opt_start);

    try
    {
        // =========================
        // 初始化：字体系统组件
        // =========================
        FontLibrary lib;
        FontFace face(lib.raw(), font_path, face_index);

        GlyphCache glyphCache(face);
        TextLayout layout(glyphCache);
        TextRasterizer rasterizer(glyphCache, layout);

        ElementRenderer elementRenderer(rasterizer);

        // =========================
        // 初始化：场景渲染器（大画布）
        // =========================
        SceneRenderer scene(1920, 1080);
        const Color clearBlack{0, 0, 0, 255};

        // =========================
        // 构造 demo 元素
        // =========================
        std::vector<TextElement> elements;

        {
            TextElement e;
            e.utf8_text = "中文渲染性能测试：数据表 DT + ASCII 123";
            e.position = {80, 120};
            e.style.font.size_px = 42;
            e.style.font.bold = true;
            e.style.font.italic = false;
            e.style.padding = 10;
            e.style.bg_color = Color{30, 30, 30, 255};
            e.style.fg_color = Color{255, 255, 255, 255};
            elements.push_back(e);
        }

        {
            TextElement e;
            e.utf8_text = "第二行：半透明底色 + 斜体模拟";
            e.position = {80, 220};
            e.style.font.size_px = 36;
            e.style.font.bold = false;
            e.style.font.italic = true;
            e.style.padding = 10;
            e.style.bg_color = Color{0, 80, 160, 180};
            e.style.fg_color = Color{255, 255, 255, 255};
            elements.push_back(e);
        }

        // =========================
        // blit_only 模式：预渲染所有小图一次
        // =========================
        std::vector<PreparedBlitItem> prepared;
        if (opt.mode == BenchMode::BlitOnly)
        {
            prepared.reserve(elements.size());
            for (const auto &e : elements)
            {
                PreparedBlitItem item;
                item.surface = elementRenderer.renderTextElement(e);
                item.position = e.position;
                // 为避免极端情况下编译器优化/或方便调试，写一点 sink
                g_sink += static_cast<std::uint64_t>(item.surface.width() + item.surface.height());
                prepared.push_back(std::move(item));
            }
        }

        // =========================
        // warm-up
        // =========================
        for (int i = 0; i < opt.warmup; ++i)
        {
            if (opt.mode == BenchMode::Full)
            {
                scene.renderFrame(elements, elementRenderer);
            }
            else if (opt.mode == BenchMode::BlitOnly)
            {
                scene.renderFramePrepared(prepared, clearBlack);
            }
            else
            { // TextOnly
                for (const auto &e : elements)
                {
                    auto s = elementRenderer.renderTextElement(e);
                    // touch a byte to make it "used"
                    g_sink += static_cast<std::uint64_t>(s.width() + s.height());
                    if (s.width() > 0 && s.height() > 0)
                    {
                        g_sink += s.data()[0];
                    }
                }
            }
        }

        // =========================
        // benchmark
        // =========================
        double total_ms = 0.0;

        for (int i = 0; i < opt.iters; ++i)
        {
            auto t0 = std::chrono::steady_clock::now();

            if (opt.mode == BenchMode::Full)
            {
                scene.renderFrame(elements, elementRenderer);
            }
            else if (opt.mode == BenchMode::BlitOnly)
            {
                scene.renderFramePrepared(prepared, clearBlack);
            }
            else
            { // TextOnly
                for (const auto &e : elements)
                {
                    auto s = elementRenderer.renderTextElement(e);
                    g_sink += static_cast<std::uint64_t>(s.width() + s.height());
                    if (s.width() > 0 && s.height() > 0)
                    {
                        g_sink += s.data()[0];
                    }
                }
            }

            auto t1 = std::chrono::steady_clock::now();
            total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        const double avg_ms = total_ms / opt.iters;

        // 输出模式信息
        const char *mode_str =
            (opt.mode == BenchMode::Full) ? "full" : (opt.mode == BenchMode::BlitOnly) ? "blit_only"
                                                                                       : "text_only";

        std::cout << "Mode: " << mode_str
                  << ", iters=" << opt.iters
                  << ", warmup=" << opt.warmup
                  << ", Avg frame: " << avg_ms << " ms\n";

        // =========================
        // 输出图像（仅 full / blit_only 有大图结果）
        // =========================
        if (opt.save && (opt.mode == BenchMode::Full || opt.mode == BenchMode::BlitOnly))
        {
            PpmWriter::saveP6("out.ppm", scene.canvas());
            std::cout << "Saved out.ppm\n";
        }

        // 防止 sink 被完全丢掉（调试用）
        if (g_sink == 0x12345678)
            std::cout << "sink=" << g_sink << "\n";

        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 2;
    }
}
