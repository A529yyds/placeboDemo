/******************************
 * 文件名：OptionPb.h
 * 功能：一些libplacebo的操作实现
 ******************************/

#pragma once
#include <fstream>
#include "MyGL.h"
class AVFrame;

enum MY_OPTION_PB
{
    PB_OPT_TEST = -1,
    PB_OPT_TRANSFER_FMT = 0,
    PB_OPT_SAR_NORMALIZE = 1,
    PB_OPT_CROP = 2,
    PB_OPT_FILL_BACKCOLOR,
    PB_OPT_CORNER_ROUND,
    PB_OPT_COLOR_SPACE,
    PB_OPT_COLOR_RANGE,
    PB_OPT_COLOR_PRIMARIES,
    PB_OPT_COLOR_TRC,
    PB_OPT_UPSCARLER,
    PB_OPT_DOWNSCARLER,
    PB_OPT_FRAME_MIXER,
    PB_OPT_ANTIRING,
    PB_OPT_SIGMOID,
    PB_OPT_DEINTERLACE,
    PB_OPT_DEBAND,
    PB_OPT_COLOR_ADJ,
    PB_OPT_PEAK_DETECT,
    PB_OPT_GAMUT_MODE,
    PB_OPT_TONE,
    PB_OPT_DITHER,
    PB_OPT_CONE,
    PB_OPT_SKIPAA,
    PB_OPT_DISABLE_LINEAR,
    PB_OPT_DISABLE_BUILTIN,
    PB_OPT_FORCE_DITHER,
    PB_OPT_DISABLE_FBOS,
};
enum MY_ALGORITHM_TYPE
{
    // 基本插值算法
    // spline插值：这些滤镜通常用于高质量的图像缩放，尤其适用于要求高保真度的场景
    SL_SPLINE_16 = 0, // 16阶样条插值
    SL_SPLINE_36 = 1,
    SL_SPLINE_64,
    SL_NEAREST,  // 最近邻插值:常用于低延迟要求的应用，或者对于图像质量要求不高的场景
    SL_BOX,      // 方框滤波: 用于图像缩小时，减少锯齿的效果，适用于快速和高效的图像缩放
    SL_BLINEAR,  // 双线性插值: 在需要平滑过渡的情况下使用，适用于一般的图像缩放操作
    SL_GAUSSIAN, // 高斯插值: 用于高质量的图像缩放，特别是在减少锯齿和噪点时非常有效

    // Sinc 系列插值
    SL_SINC,        // 无窗 Sinc 插值，理论上提供最好的重建质量。
    SL_LANCZOS,     // 由两个 Sinc 函数组合而成，是常用的高质量插值方法，尤其在放大图像时表现出色
    SL_GINSENG,     // 类似于 Lanczos，但结合了不同的窗函数，旨在提供更好的视觉效果
    SL_EWA_JINC,    // 使用 Jinc 函数作为窗函数，优化了 Sinc 插值的频率响应，适用于高质量放大
    SL_EWA_LANCZOS, // 结合了 Lanczos 和 Jinc 窗函数，进一步优化了图像质量
    SL_EWA_LANCZOS4SHARP,
    SL_EWA_LANCZOS4SHARPEST,
    SL_EWA_GINSENG,
    SL_EWA_HANN,

    // Bicubic 和 Hermite 系列插值【经典的样条插值方法】
    SL_BICUBIC,        // 经典的三次插值，提供平滑的过渡
    SL_HERMITE,        // 具有更平滑过渡的样条插值，适用于自然图像。
    SL_CATMULL_ROM,    // 适用于图像细节较多的情况，提供较强的锐度
    SL_MITCHELL,       // 常用的插值方法，兼顾平滑和锐度
    SL_MITCHELL_CLAMP, // 通过限制插值函数的响应范围来避免过度模糊。

    // Robidoux 系列插值:用于 EWA（Elliptical Weighted Averaging）技术的插值方法
    // 这些滤镜结合了 EWA 技术，用于高质量的图像重采样，特别是在图像放大的同时保留细节和锐度。
    // robidouxsharp 则进一步加强了锐度
    SL_ROBIDOUX,
    SL_ROBIDOUXSHARP,
    SL_EWA_ROBIDOUX,
    SL_EWA_ROBIDOUXSHARP,
    SL_OVERSAMPLE,
};
enum MY_GAMUT
{
    GAMUT_MAP_CLIP,
    GAMUT_MAP_PERCEPTUAL,
    GAMUT_MAP_RELATIVE,
    GAMUT_MAP_SATURATION,
    GAMUT_MAP_ABSOLUTE,
    GAMUT_MAP_DESATURATE,
    GAMUT_MAP_DARKEN,
    GAMUT_MAP_HIGHLIGHT,
    GAMUT_MAP_LINEAR,
    GAMUT_MAP_COUNT,
};
enum MY_TONEMAPPING
{
    TONE_MAP_AUTO,
    TONE_MAP_CLIP,
    TONE_MAP_ST2094_40,
    TONE_MAP_ST2094_10,
    TONE_MAP_BT2390,
    TONE_MAP_BT2446A,
    TONE_MAP_SPLINE,
    TONE_MAP_REINHARD,
    TONE_MAP_MOBIUS,
    TONE_MAP_HABLE,
    TONE_MAP_GAMMA,
    TONE_MAP_LINEAR,
    TONE_MAP_COUNT,
};

class OptionPb
{
public:
    OptionPb();
    bool optPb(AVFrame *inAVFrame, MY_OPTION_PB opt = PB_OPT_TEST);
    void endOpt();

private:
    int writeFiles(AVFrame *outAvFrame);
    void options(MY_OPTION_PB opt, pl_render_params &params);
    void test();
    void crop(AVFrame *inFrame);
    void transferFormat(int outFmt);
    void SarNormalize(pl_frame inFrame, pl_frame &outFrame, pl_render_params params, bool bN, int padCropRatio);
    void fillcolor(pl_render_params &params);
    // 色彩操作，包括：视频圆角【0-1】、colorspace、color_range、color_primaries、color_trc
    template <typename T>
    void colorOpt(MY_OPTION_PB opt, T val, pl_render_params &params);
    /*
    功能：图像缩放、混合操作，以及不同算法的使用
    参数说明：
            r: 缩放倍数，混合设为1即可
            params:
            algo:算法类型
            ft：过滤器
    */
    void filterOpt(int r, pl_render_params &params, MY_ALGORITHM_TYPE algo, MY_OPTION_PB ft = PB_OPT_UPSCARLER);
    /*
    功能：配置去交错算法和一些优化选项
    参数说明：
            algo:选择去交错算法, 默认PL_DEINTERLACE_YADIF
            bssc:在使用 PL_DEINTERLACE_YADIF 算法时，是否跳过空间交错检查
    */
    void deinterlace(int algo, bool bssc, pl_render_params &params);
    /*
    功能：处理图像中的 色带问题
    参数说明：
            val[0]:启用色带功能deband
            val[1]:deband_iterations【每个样本的去带步骤数】
            val[2]:deband_threshold【去带滤波器的阈值，控制滤波器的强度】
            val[3]:deband_radius【去带滤波器的初始半径】
            val[4]:deband_grain【向图像中添加额外的噪声，有助于掩盖剩余的量化伪影】
    */
    void deband(pl_render_params &params, float val[5]);
    /*
    功能：用于在色彩空间转换过程中应用额外的艺术性调整。
            这些调整影响图像的亮度、对比度、饱和度、色相、伽马值和色温等属性，
            从而在不改变色彩空间本身的情况下对图像的视觉效果进行优化或修改
    参数说明：
            val[0]:brightness【亮度调整】
            val[1]:contrast【对比度调整】
            val[2]:saturation【饱和度调整】
            val[3]:hue【色相偏移】
            val[4]:gamma【伽马调整】
            val[5]:temperature【色温调整】
    */
    void colorAdj(pl_render_params &params, float val[6]);
    /*
    功能：图像峰值检测
    参数说明：
            val[0]:peak_detect 0/1
            val[1]:smoothing_period【决定了图像亮度平滑的频率响应】
            val[2]:scene_threshold_low【窗口的低阈值】
            val[3]:scene_threshold_high【窗口的高阈值】
            val[4]:percentile【确定哪个百分位的亮度值应该被视为场景的真实峰值】
            val[5]:black_cutoff【控制在非常暗的区域中是否进行 黑色裁切】
            val[6]:allow_delayed【允许峰值检测结果延迟最多一帧】
            val[7]:minimum_peak【已弃用】
    */
    void peakDetect(pl_render_params &params, float val[8]);
    /*
    功能：色域、色调映射，主要用于 高动态范围（HDR）图像处理 和 色彩映射
    参数说明：
            val[0]:GAMUT方法【色域映射】
            val[1]:TONE方法【色调映射】
            val[2]:tone_mapping_param
            val[3]:inverse_tone_mapping【如果为 true，并且支持逆色调映射的色调映射函数，libplacebo 将执行 逆色调映射，即扩展信号的动态范围。这样可以将 SDR 图像的亮度信息扩展到 HDR 范围】
            val[4]:lut_size【指定色调映射 LUT（查找表）的大小，通常默认为 256】
            val[5]:contrast_recovery【控制 HDR 对比度恢复强度。设置为大于 0.0 的值时，源图像会被分成高频和低频分量，并将部分高频图像加回到色调映射后的输出图像中】
            val[6]:contrast_smoothness【设置 对比度恢复低通内核大小】
    */
    void colorMap(pl_render_params &params, float val[7]);
    /*
    功能：配置 抖动（dithering） 相关的参数
    参数说明：
            val[0]:method【指定抖动噪声的源，控制使用的抖动方法】
            val[1]:lut_size【对于需要使用查找表（LUT）的抖动方法，控制 LUT 的大小（以 2 为基数）】
            val[2]:temporal【启用 时域抖动】
            val[3]:transfer【指定用于 色调映射（gamma correction）的 Gamma 函数】
    */
    void dither(pl_render_params &params, int val[4]);
    /*
    功能：模拟色盲效果
    参数说明：
            val[0]:指定受色盲影响的锥体细胞类型，常见选项为：PL_CONE_L: 影响 L 锥体（通常会影响红色感知）。
                    PL_CONE_M: 影响 M 锥体（通常会影响绿色感知）。
                    PL_CONE_S: 影响 S 锥体（通常会影响蓝色感知）。
                    PL_CONE_LM: 同时影响 L 和 M 锥体，模拟红绿色盲。
                    PL_CONE_LS: 同时影响 L 和 S 锥体，模拟红蓝色盲。
                    PL_CONE_MS: 同时影响 M 和 S 锥体，模拟绿色蓝色盲。
                    PL_CONE_LMS: 同时影响所有三种锥体，模拟全色盲。
            val[1]:控制色盲的强度系数，影响色盲的程度。值范围从 0.0（完全色盲）到1.0（完全正常）
    */
    void cone(pl_render_params &params, float val[2]);
    // 配置一个参数实现
    template <typename T>
    void opt1Params(MY_OPTION_PB opt, pl_render_params &params, T val);

private:
    char *_inFileName;
    char *_outFileName;
    bool _bCLoseFile = false;
    std::ofstream _outfile;
    pl_gpu _gpu;
    pl_renderer _render;
    AVFrame *_outAvFrame = nullptr;
};
