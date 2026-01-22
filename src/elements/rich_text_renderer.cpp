#include "elements/rich_text_renderer.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "image/png_reader.h"

namespace demo
{
    namespace
    {
        std::string escapeForShell(const std::string &input)
        {
            std::ostringstream out;
            out << "'";
            for (char c : input)
            {
                if (c == '\'')
                    out << "'\"'\"'";
                else
                    out << c;
            }
            out << "'";
            return out.str();
        }

        std::string buildHtmlDocument(const RichTextElement &element)
        {
            std::ostringstream html;
            html << "<!doctype html><html><head><meta charset=\"utf-8\">"
                 << "<style>html,body{margin:0;padding:0;background:transparent;}</style>"
                 << "</head><body>"
                 << "<div style=\"width:" << element.width << "px;height:" << element.height << "px;\">"
                 << element.html
                 << "</div></body></html>";
            return html.str();
        }
    } // namespace

    RichTextRenderer::RichTextRenderer(std::string chromium_path, std::string work_dir)
        : chromium_path_(std::move(chromium_path)), work_dir_(std::move(work_dir))
    {
        available_ = ::access(chromium_path_.c_str(), X_OK) == 0;
    }

    RgbaSurface RichTextRenderer::renderRichTextElement(const RichTextElement &element) const
    {
        RgbaSurface surface;
        if (!available_)
        {
            if (!warned_missing_)
            {
                std::cerr << "Chromium not found at " << chromium_path_
                          << ", skip rich text rendering.\n";
                warned_missing_ = true;
            }
            return surface;
        }
        if (element.width <= 0 || element.height <= 0 || element.html.empty())
            return surface;

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const std::string html_path = work_dir_ + "/rich_text_" + std::to_string(stamp) + ".html";
        const std::string png_path = work_dir_ + "/rich_text_" + std::to_string(stamp) + ".png";

        {
            std::ofstream out(html_path);
            out << buildHtmlDocument(element);
        }

        const std::string cmd =
            escapeForShell(chromium_path_) +
            " --headless --disable-gpu --hide-scrollbars --default-background-color=00000000"
            " --window-size=" + std::to_string(element.width) + "," + std::to_string(element.height) +
            " --screenshot=" + escapeForShell(png_path) + " " + escapeForShell("file://" + html_path);

        const int rc = std::system(cmd.c_str());
        if (rc != 0)
            return surface;

        if (!PngReader::load(png_path, surface))
            return RgbaSurface();

        return surface;
    }

} // namespace demo
