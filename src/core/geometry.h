#pragma once

namespace demo
{

    /**
     * @brief 二维点（像素坐标）。
     * 通常用于元素贴图位置、绘制起点等。
     */
    struct Point
    {
        int x = 0;
        int y = 0;
    };

    /**
     * @brief 矩形区域（像素坐标 + 尺寸）。
     * 常用于填充底色区域、裁剪、布局测量等。
     */
    struct Rect
    {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
    };

} // namespace demo
