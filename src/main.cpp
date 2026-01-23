#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>

#include "text/font_library.h"
#include "text/font_face.h"
#include "text/glyph_cache.h"
#include "text/text_layout.h"
#include "text/text_rasterizer.h"

#include "elements/text_element.h"
#include "elements/element_renderer.h"
#include "elements/rich_text_element.h"
#include "elements/rich_text_renderer.h"
#include "renderer/scene_renderer.h"

#include "image/png_writer.h"

using namespace demo;

// 防止 text_only 模式下编译器把“渲染但不用结果”的逻辑优化掉
static volatile std::uint64_t g_sink = 0;

enum class BenchMode
{
    Full,     // render small + blit
    BlitOnly, // pre-render small once; per frame only blit
    TextOnly, // only render small surfaces, no blit
    KernelOnly, // alias of TextOnly, explicitly used for kernel generation speed
    RichTextOnly // only render rich text elements, no blit
};

struct Options
{
    BenchMode mode = BenchMode::Full;
    int iters = 300;
    int warmup = 30;
    bool save = true;
    std::string out_path = "out.png";
    std::string chromium_path = "/usr/bin/chromium";
};

static void printUsage(const char *prog)
{
    std::cerr
        << "Usage: " << prog << " [face_index] [--mode=full|blit_only|text_only|kernel_only|rich_text_only] [--iters=N] [--warmup=N] [--out=PATH] [--no-save]\n"
        << "Example:\n"
        << "  " << prog << " 0 --mode=full --iters=300\n"
        << "  " << prog << " 0 --mode=blit_only\n"
        << "  " << prog << " 0 --mode=text_only --no-save\n"
        << "  " << prog << " 0 --out=out.png\n";
}

static bool startsWith(const std::string &s, const std::string &prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

static std::string decodeHtmlEntitiesOnce(const std::string &input)
{
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (input.compare(i, 4, "&lt;") == 0)
        {
            out.push_back('<');
            i += 3;
        }
        else if (input.compare(i, 4, "&gt;") == 0)
        {
            out.push_back('>');
            i += 3;
        }
        else if (input.compare(i, 6, "&quot;") == 0)
        {
            out.push_back('"');
            i += 5;
        }
        else if (input.compare(i, 5, "&amp;") == 0)
        {
            out.push_back('&');
            i += 4;
        }
        else
        {
            out.push_back(input[i]);
        }
    }
    return out;
}

static std::string decodeHtmlEntities(const std::string &input)
{
    std::string current = input;
    for (int i = 0; i < 4; ++i)
    {
        std::string next = decodeHtmlEntitiesOnce(current);
        if (next == current)
            break;
        current = std::move(next);
    }
    return current;
}

static std::string pickChromiumPath()
{
    const char *candidates[] = {
        "/snap/bin/chromium",
        "/usr/bin/chromium",
        "/usr/bin/chromium-browser",
        "/usr/local/bin/chromium",
    };
    for (const char *path : candidates)
    {
        if (::access(path, X_OK) == 0)
            return path;
    }
    return "/usr/bin/chromium";
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
            else if (v == "kernel_only")
                opt.mode = BenchMode::KernelOnly;
            else if (v == "rich_text_only")
                opt.mode = BenchMode::RichTextOnly;
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
        else if (startsWith(a, "--out="))
        {
            opt.out_path = a.substr(std::string("--out=").size());
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
    // 参数处理（字体路径固定）
    // =========================
    if (argc < 1)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string font_path = "/root/.local/share/fonts/simsun.ttc";

    // face_index：可选，如果 argv[2] 不是数字则默认 0，并把 argv[2] 当作 options 开始
    int face_index = 0;
    int opt_start = 1;

    if (argc >= 2)
    {
        // 简单判断 argv[2] 是否为纯数字（允许 0/1/2...）
        std::string s = argv[1];
        bool is_num = !s.empty();
        for (char c : s)
            is_num = is_num && (c >= '0' && c <= '9');

        if (is_num)
        {
            face_index = std::atoi(argv[1]);
            opt_start = 2;
        }
    }

    Options opt = parseOptions(argc, argv, opt_start);
    opt.chromium_path = pickChromiumPath();

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
        std::vector<RichTextElement> rich_elements;

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

        const std::string rich_html_encoded =
            "&lt;p style=&quot;color:red;font-size:16px&quot;&gt;你难道看不见那黄河之水从天上而来，波涛滚滚奔向东海,永不回头。难道看不见那从高大的厅堂的明镜中照见了白发，早晨还是满头的黑发，傍晚便白得如雪。人生有兴致时，要尽情地寻欢作乐，别让酒杯无酒，空对着天上的明月。天地造就我的才干,必有它的用处，即使千金耗尽，还会重新再来。烹羊宰牛，且图眼前的欢乐，应当痛痛快快一口气喝上三百杯。岑勋先生呵，丹丘先生呵，请快快喝酒吧，（举起）酒杯不要停。让我为你们高歌一曲，请你们为我侧耳仔细听。那些富贵生活，有什么值得苦苦追求的呢，我只愿长醉享乐，不愿醒来。自古以来圣贤常被世人冷落，唯有那些寄情美酒之人芳名永驻。陈王曹植从前在平乐观举行宴会，喝着名贵的酒，尽情地欢乐戏谑。元丹丘呵，为什么说钱不够，毫不犹豫地买下美酒来让我们一起痛饮。这一匹名贵的五花马、这一件珍贵的皮衣，叫侍僮拿去换美酒吧，我与你们一起排遣万古长愁。&lt;/p&gt;&lt;p style=&quot;color:red;font-size:16px&quot;&gt;&lt;br&gt;&lt;/p&gt;";
        RichTextElement rich;
        rich.html = decodeHtmlEntities(rich_html_encoded);
        rich.position = {80, 320};
        rich.width = 900;
        rich.height = 360;
        rich_elements.push_back(rich);

        // =========================
        // blit_only 模式：预渲染所有小图一次
        // =========================
        std::vector<PreparedBlitItem> prepared;
        RichTextRenderer richTextRenderer(opt.chromium_path);
        if (!richTextRenderer.available())
        {
            rich_elements.clear();
        }
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

            for (const auto &e : rich_elements)
            {
                PreparedBlitItem item;
                item.surface = richTextRenderer.renderRichTextElement(e);
                item.position = e.position;
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
                if (rich_elements.empty())
                {
                    scene.renderFrame(elements, elementRenderer);
                }
                else
                {
                    scene.renderFrame(elements, rich_elements, elementRenderer, richTextRenderer);
                }
            }
            else if (opt.mode == BenchMode::BlitOnly)
            {
                scene.renderFramePrepared(prepared, clearBlack);
            }
            else if (opt.mode == BenchMode::TextOnly || opt.mode == BenchMode::KernelOnly)
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
                for (const auto &e : rich_elements)
                {
                    auto s = richTextRenderer.renderRichTextElement(e);
                    g_sink += static_cast<std::uint64_t>(s.width() + s.height());
                    if (s.width() > 0 && s.height() > 0)
                    {
                        g_sink += s.data()[0];
                    }
                }
            }
            else
            { // RichTextOnly
                for (const auto &e : rich_elements)
                {
                    auto s = richTextRenderer.renderRichTextElement(e);
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
                if (rich_elements.empty())
                {
                    scene.renderFrame(elements, elementRenderer);
                }
                else
                {
                    scene.renderFrame(elements, rich_elements, elementRenderer, richTextRenderer);
                }
            }
            else if (opt.mode == BenchMode::BlitOnly)
            {
                scene.renderFramePrepared(prepared, clearBlack);
            }
            else if (opt.mode == BenchMode::TextOnly || opt.mode == BenchMode::KernelOnly)
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
                for (const auto &e : rich_elements)
                {
                    auto s = richTextRenderer.renderRichTextElement(e);
                    g_sink += static_cast<std::uint64_t>(s.width() + s.height());
                    if (s.width() > 0 && s.height() > 0)
                    {
                        g_sink += s.data()[0];
                    }
                }
            }
            else
            { // RichTextOnly
                for (const auto &e : rich_elements)
                {
                    auto s = richTextRenderer.renderRichTextElement(e);
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
            (opt.mode == BenchMode::Full) ? "full"
            : (opt.mode == BenchMode::BlitOnly) ? "blit_only"
            : (opt.mode == BenchMode::KernelOnly) ? "kernel_only"
            : (opt.mode == BenchMode::RichTextOnly) ? "rich_text_only"
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
            if (PngWriter::save(opt.out_path, scene.canvas()))
                std::cout << "Saved " << opt.out_path << "\n";
            else
                std::cout << "Failed to save " << opt.out_path << "\n";
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
