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

struct Options
{
    std::string out_path = "out.png";
    std::string chromium_path = "/usr/bin/chromium";
};

static void printUsage(const char *prog)
{
    std::cerr
        << "Usage: " << prog << " [face_index] [--out=PATH]\n"
        << "Example:\n"
        << "  " << prog << " 0\n"
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

        if (startsWith(a, "--out="))
        {
            opt.out_path = a.substr(std::string("--out=").size());
        }
        else
        {
            std::cerr << "Unknown argument: " << a << "\n";
            printUsage(argv[0]);
            std::exit(1);
        }
    }

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

        RichTextRenderer richTextRenderer(opt.chromium_path);
        if (!richTextRenderer.available())
        {
            rich_elements.clear();
        }
        if (rich_elements.empty())
        {
            scene.renderFrame(elements, elementRenderer);
        }
        else
        {
            scene.renderFrame(elements, rich_elements, elementRenderer, richTextRenderer);
        }

        if (PngWriter::save(opt.out_path, scene.canvas()))
            std::cout << "Saved " << opt.out_path << "\n";
        else
            std::cout << "Failed to save " << opt.out_path << "\n";

        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 2;
    }
}
