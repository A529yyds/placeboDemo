// 定义一些通用的枚举、宏、结构体等
#pragma once

#include <iostream>
#define WIDTH 3840
#define HEIGHT 2160
#define VIDEOFILE "/root/lincy/projects/placeboDemo/VideoFiles/in/4K1.mkv"
#define OUTFILE "/root/lincy/projects/placeboDemo/VideoFiles/out/target.yuv"
// #define OUTFILE "/root/lincy/projects/placeboDemo/yuv_scaler.yuv"
// #define VIDEOFILE "/root/lincy/projects/placeboDemo/yuv420p.mkv"
#define CERR(title, ctx) \
    std::cerr << "[" << title << "] " << ctx << std::endl;
#define COUT(title, ctx, params) \
    std::cout << "[" << title << "] " << ctx << params << std::endl;
#define DELETE(a)    \
    if (a)           \
    {                \
        delete a;    \
        a = nullptr; \
    }