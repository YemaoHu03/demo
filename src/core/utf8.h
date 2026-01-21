#pragma once
#include <cstdint>

/**
 * 本文件提供最小可用的 UTF-8 解码器：
 * - 支持 1/2/3/4 字节 UTF-8
 * - 遇到非法字节序列，会返回 U+FFFD
 *
 * 设计目标：
 * - 纯性能 demo：避免引入额外依赖（如 ICU）
 * - 足够支持中文 + ASCII 混排场景
 */

namespace demo
{

    /**
     * @brief 读取下一个 Unicode codepoint。
     *
     * @param p   指向当前 UTF-8 位置的指针（函数内部会推进）
     * @param end 指向字符串末尾
     * @param cp  输出：解析得到的 codepoint
     * @return true 表示成功读取；false 表示已到末尾
     */
    inline bool utf8_next(const char *&p, const char *end, std::uint32_t &cp)
    {
        if (p >= end)
            return false;

        unsigned char c = static_cast<unsigned char>(*p++);
        if (c < 0x80)
        {
            cp = c;
            return true;
        }

        // 2 bytes
        if ((c >> 5) == 0x6 && p < end)
        {
            cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
            return true;
        }

        // 3 bytes
        if ((c >> 4) == 0xE && (p + 1) < end)
        {
            cp = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(*p++) & 0x3F) << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
            return true;
        }

        // 4 bytes
        if ((c >> 3) == 0x1E && (p + 2) < end)
        {
            cp = ((c & 0x07) << 18) | ((static_cast<unsigned char>(*p++) & 0x3F) << 12) | ((static_cast<unsigned char>(*p++) & 0x3F) << 6) | (static_cast<unsigned char>(*p++) & 0x3F);
            return true;
        }

        // 非法情况：回退为 replacement character
        cp = 0xFFFD;
        return true;
    }

} // namespace demo
