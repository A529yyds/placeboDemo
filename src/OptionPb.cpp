#include "OptionPb.h"
#include "defines.h"
// 此处直接引用libav.h会报错：C++不能启用PL_LIBAV_IMPLEMENTATION宏
// 解决方法：新增.c文件，将启用宏和引用头文件放到c文件里，详见test.c
#define PL_LIBAV_IMPLEMENTATION 0
#include <libplacebo/gpu.h>
#include <libplacebo/utils/libav.h>

#define CONDITION(a, b) \
    if (opt == a)       \
    {                   \
        b;              \
    }

OptionPb::OptionPb()
{
    _gpu = MyGL::getInstance()->getGpu();
    _render = MyGL::getInstance()->getRenderer();
    _outfile.open(OUTFILE, std::ios::binary);
    _outAvFrame = av_frame_alloc();
}

void OptionPb::pushAVframe(AVFrameWrap &frame)
{
    AVFrameWrap frame2;
    frame2.frame = av_frame_alloc();
    frame2 = frame;
    _queueFrame.push(std::move(frame2));
    // frame2.unref();
    // {
    //     std::lock_guard<std::mutex> lock(_mutex); // 加锁
    //     bool isEmpty = _myAVFrames.empty();
    //     _myAVFrames.push(frame);
    // }
}

bool OptionPb::handleOpt(MY_OPTION_PB opt)
{
    AVFrameWrap wFrame;
    bool ret = true;
    // wFrame.frame = av_frame_alloc();
    while (!_bStop)
    {
        // {
        //     std::lock_guard<std::mutex> lock(_mutex); // 加锁
        //     if (!_myAVFrames.empty())
        //     {
        //         AVFrame *tmpFrame = _myAVFrames.front();
        //         // _myAVFrames.front(); // 获取队首元素（不删除）:ml-citation{ref="6,14" data="citationList"}
        //         optPb(tmpFrame, opt);
        //         _myAVFrames.pop();
        //     }
        // }
        // if (ret = _queueFrame.try_pop(wFrame))
        // {
        //     ret = optPb(wFrame.frame, opt);
        // }
        if (ret = _queueFrame.try_pop(wFrame))
        {
            ret = optPb(wFrame.frame, opt);
        }
    }
    // wFrame.unref();
    _outfile.close();
    return ret;
}

void OptionPb::stopTask()
{
    std::lock_guard<std::mutex> lock(_mutex); // 加锁
    {
        _bStop = true;
    }
}
int count = 0;
bool OptionPb::optPb(AVFrame *inAvFrame, MY_OPTION_PB opt)
{
    bool ret = true;
    pl_frame inFrame = {.planes = {0}};

    // CONDITION(PB_OPT_CROP, crop(inAvFrame));
    // 从AVFrame格式转化为pl_frame格式，只能获取图像的信息，不能获取图像纹理
    pl_frame_from_avframe(&inFrame, inAvFrame);

    pl_tex inTex[4] = {NULL, NULL, NULL, NULL}; // 输入纹理初始化
    // 通过avframe的数据映射到pl_frame的纹理上，并将输入纹理上传gpu，理论上inTex可以为一个四维数组
    if (!pl_map_avframe(_gpu, &inFrame, inTex, inAvFrame)) // 不能放到子线程中运行
    {
        COUT("ERROR", "pl_map_avframe ret is ", "false");
        return false;
    }
    pl_tex outTex[4] = {NULL, NULL, NULL, NULL}; // 输出纹理初始化
    pl_frame outFrame = {.planes = {0}};
    // 创建输出帧：此处仅需要对输出帧w/h/format进行设置即可
    _outAvFrame->width = WIDTH * 0.5;
    _outAvFrame->height = HEIGHT * 0.5;
    _outAvFrame->format = inAvFrame->format;
    // 设置输出格式：
    // 输出格式的值可以随便设置，只需要对后续写入文件的数据平面处理好即可
    // 目前写入文件格式仅设置了YUV422P10LE、YUV420P、NV12
    // 初始化AVFrame的data数据[不初始化无法下载纹理]
    av_frame_get_buffer(_outAvFrame, 32);
    // 使用初始化后的out_frame[AVFrame]填充输出纹理和pl_frame
    ret = pl_frame_recreate_from_avframe(_gpu, &outFrame, outTex, _outAvFrame);
    // 设置渲染参数: 此处采用默认参数
    pl_render_params params = pl_render_default_params;
    // options(opt, params);
    // CONDITION(PB_OPT_SAR_NORMALIZE, SarNormalize(inFrame, outFrame, params, true, 0));

    auto start = std::chrono::steady_clock::now();
    if (!(ret = pl_render_image(_render, &inFrame, &outFrame, &params)))
    {
        COUT("ERROR", "pl_render_image ret is ", "false");
    }
    else
    {
        // 从gpu下载纹理并填充到输出帧
        if ((ret = pl_download_avframe(_gpu, &outFrame, _outAvFrame)))
        {
            writeFiles(_outAvFrame);
        }
    }
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    // 将时间差转换为 uint32_t
    uint32_t time_diff_ms = static_cast<uint32_t>(duration.count());
    // 输出时间差
    if (time_diff_ms > 0)
        std::cout
            << "frame " << count << ", pl_render_image time: " << time_diff_ms << " microseconds" << std::endl;
    if (time_diff_ms > 2000)
        count = 0;
    count++;
    // 释放纹理资源
    for (int i = 0; i < 4; i++)
    {
        pl_tex_destroy(_gpu, &inTex[i]);
        pl_tex_destroy(_gpu, &outTex[i]);
    }

    pl_unmap_avframe(_gpu, &inFrame);
    pl_unmap_avframe(_gpu, &outFrame);
    av_frame_unref(inAvFrame); // 根据所有权决定
    av_frame_unref(_outAvFrame);
    return ret;
}

int OptionPb::writeFiles(AVFrame *frame)
{
    int outW = frame->width;
    int outH = frame->height;
    // 获取输出帧数据并写入文件，根据图像格式将不同平面数据以及数据大小分别写入文件
    // YUV420P:Y[w * h], U[w / 2 * h / 2], V[w / 2 * h / 2]
    // YUV422P10LE:Y[w * h * 2], U[w * (h / 2) * 2], V[w * (h / 2) * 2]
    // NV12:Y[w * h], UV[(w / 2 * h / 2) * 2]
    switch (frame->format)
    {
    case AV_PIX_FMT_YUV420P:
    {
        _outfile.write(reinterpret_cast<const char *>(frame->data[0]), outW * outH);
        _outfile.write(reinterpret_cast<const char *>(frame->data[1]), outW * outH / 4);
        _outfile.write(reinterpret_cast<const char *>(frame->data[2]), outW * outH / 4);
        break;
    }
    case AV_PIX_FMT_YUV422P10LE:
    {
        _outfile.write(reinterpret_cast<const char *>(frame->data[0]), outW * outH * 2);
        _outfile.write(reinterpret_cast<const char *>(frame->data[1]), outW / 2 * outH * 2);
        _outfile.write(reinterpret_cast<const char *>(frame->data[2]), outW / 2 * outH * 2);
        break;
    }
    case AV_PIX_FMT_NV12:
    {
        _outfile.write(reinterpret_cast<const char *>(frame->data[0]), outW * outH);
        _outfile.write(reinterpret_cast<const char *>(frame->data[1]), outW * outH / 2);
        break;
    }

    default:
        break;
    }

    return 0;
}

void OptionPb::options(MY_OPTION_PB opt, pl_render_params &params)
{
    CONDITION(PB_OPT_TEST, test());
    CONDITION(PB_OPT_TRANSFER_FMT, transferFormat(AV_PIX_FMT_NV12));
    CONDITION(PB_OPT_FILL_BACKCOLOR, fillcolor(params));
    CONDITION(PB_OPT_CORNER_ROUND, colorOpt(PB_OPT_CORNER_ROUND, 0.5, params));
    CONDITION(PB_OPT_COLOR_SPACE, colorOpt(PB_OPT_COLOR_SPACE, AVCOL_SPC_SMPTE240M, params));
    CONDITION(PB_OPT_COLOR_RANGE, colorOpt(PB_OPT_COLOR_RANGE, AVCOL_RANGE_MPEG, params));
    CONDITION(PB_OPT_COLOR_PRIMARIES, colorOpt(PB_OPT_COLOR_PRIMARIES, AVCOL_PRI_RESERVED, params));
    CONDITION(PB_OPT_COLOR_TRC, colorOpt(PB_OPT_COLOR_TRC, AVCOL_PRI_RESERVED0, params));
    CONDITION(PB_OPT_UPSCARLER, filterOpt(1.5, params, SL_BICUBIC));
    CONDITION(PB_OPT_DOWNSCARLER, filterOpt(0.5, params, SL_MITCHELL, PB_OPT_DOWNSCARLER));
    CONDITION(PB_OPT_FRAME_MIXER, filterOpt(1, params, SL_BICUBIC, PB_OPT_FRAME_MIXER));
    CONDITION(PB_OPT_ANTIRING, opt1Params(PB_OPT_ANTIRING, params, 1));
    CONDITION(PB_OPT_SIGMOID, opt1Params(PB_OPT_SIGMOID, params, 1));
    CONDITION(PB_OPT_DEINTERLACE, deinterlace(PL_DEINTERLACE_BOB, true, params));
    float dVal[5] = {1, 1, 4, 16, 1024};
    CONDITION(PB_OPT_DEBAND, deband(params, dVal));
    float val[6] = {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    CONDITION(PB_OPT_COLOR_ADJ, colorAdj(params, val));
    float pdVal[8] = {1.0f, 20.0f, 1.0f, 3.0f, 100.0f, 1.0f, 1.0f, 1.0f};
    CONDITION(PB_OPT_PEAK_DETECT, peakDetect(params, pdVal));
    float cmVal[7] = {1.0f, 0.0f, 0.0f, 0.0f, 265.0f, 0.3f, 3.5f};
    CONDITION(PB_OPT_GAMUT_MODE, colorMap(params, cmVal));
    int idVal[4] = {0, 6, 0, 0};
    CONDITION(PB_OPT_ANTIRING, dither(params, idVal));
    float cVal[2] = {0, 0};
    CONDITION(PB_OPT_ANTIRING, cone(params, cVal));
    CONDITION(PB_OPT_ANTIRING, opt1Params(PB_OPT_ANTIRING, params, 0.5));
    CONDITION(PB_OPT_SIGMOID, opt1Params(PB_OPT_SIGMOID, params, 1));
    CONDITION(PB_OPT_SKIPAA, opt1Params(PB_OPT_SKIPAA, params, true));
    CONDITION(PB_OPT_DISABLE_LINEAR, opt1Params(PB_OPT_DISABLE_LINEAR, params, true));
    CONDITION(PB_OPT_DISABLE_BUILTIN, opt1Params(PB_OPT_DISABLE_BUILTIN, params, true));
    CONDITION(PB_OPT_FORCE_DITHER, opt1Params(PB_OPT_FORCE_DITHER, params, true));
    CONDITION(PB_OPT_DISABLE_FBOS, opt1Params(PB_OPT_DISABLE_FBOS, params, true));
}

void OptionPb::transferFormat(int outFmt)
{
    // 设置输出格式：
    _outAvFrame->format = (AVPixelFormat)outFmt;
}

void OptionPb::test()
{
    // _outAvFrame->width = 1920;
    // _outAvFrame->height = 1080;
    _outAvFrame->time_base = AVRational{1, 30};
}

void OptionPb::crop(AVFrame *inFrame)
{
    // 裁剪函数：需要提前对输入帧进行设置
    // inAvFrame->crop_top = 0;
    // inAvFrame->crop_left = 0;
    // inAvFrame->crop_right = 1920;
    // inAvFrame->crop_bottom = 1080;
    // int rt = av_frame_apply_cropping(inAvFrame, 0);
    // COUT("CROP", "ret = ", rt);
}

void OptionPb::filterOpt(int r, pl_render_params &params, MY_ALGORITHM_TYPE opt, MY_OPTION_PB ft)
{
    // 缩放倍数设为 r 倍
    _outAvFrame->width = WIDTH * r;
    _outAvFrame->height = HEIGHT * r;
    // 缩放算法
    const pl_filter_config *scaler;
    CONDITION(SL_SPLINE_16, scaler = &pl_filter_spline16)
    CONDITION(SL_SPLINE_36, scaler = &pl_filter_spline36)
    CONDITION(SL_SPLINE_64, scaler = &pl_filter_spline64)
    CONDITION(SL_NEAREST, scaler = &pl_filter_nearest)
    CONDITION(SL_BOX, scaler = &pl_filter_box)
    CONDITION(SL_BLINEAR, scaler = &pl_filter_bilinear)
    CONDITION(SL_GAUSSIAN, scaler = &pl_filter_gaussian)
    CONDITION(SL_SINC, scaler = &pl_filter_sinc)
    CONDITION(SL_LANCZOS, scaler = &pl_filter_lanczos)
    CONDITION(SL_GINSENG, scaler = &pl_filter_ginseng)
    CONDITION(SL_EWA_JINC, scaler = &pl_filter_ewa_jinc)
    CONDITION(SL_EWA_LANCZOS, scaler = &pl_filter_ewa_lanczos)
    CONDITION(SL_EWA_LANCZOS4SHARP, scaler = &pl_filter_ewa_lanczossharp)
    CONDITION(SL_EWA_LANCZOS4SHARPEST, scaler = &pl_filter_ewa_lanczos4sharpest)
    CONDITION(SL_EWA_GINSENG, scaler = &pl_filter_ewa_ginseng)
    CONDITION(SL_EWA_HANN, scaler = &pl_filter_ewa_hann)
    CONDITION(SL_BICUBIC, scaler = &pl_filter_bicubic)
    CONDITION(SL_HERMITE, scaler = &pl_filter_hermite)
    CONDITION(SL_CATMULL_ROM, scaler = &pl_filter_catmull_rom)
    CONDITION(SL_MITCHELL, scaler = &pl_filter_mitchell)
    CONDITION(SL_MITCHELL_CLAMP, scaler = &pl_filter_mitchell_clamp)
    CONDITION(SL_ROBIDOUX, scaler = &pl_filter_robidoux)
    CONDITION(SL_ROBIDOUXSHARP, scaler = &pl_filter_robidouxsharp)
    CONDITION(SL_EWA_ROBIDOUX, scaler = &pl_filter_ewa_robidoux)
    CONDITION(SL_EWA_ROBIDOUXSHARP, scaler = &pl_filter_ewa_robidouxsharp)
    CONDITION(SL_OVERSAMPLE, scaler = &pl_filter_oversample)
    switch (ft)
    {
    case PB_OPT_UPSCARLER:
        params.downscaler = scaler;
        break;
    case PB_OPT_DOWNSCARLER:
        params.upscaler = scaler;
        break;
    case PB_OPT_FRAME_MIXER:
        params.frame_mixer = scaler;
        break;
    default:
        break;
    }
}

void OptionPb::deinterlace(int val, bool bssc, pl_render_params &params)
{
    pl_deinterlace_params dParams = pl_deinterlace_default_params;
    dParams.algo = (pl_deinterlace_algorithm)val;
    dParams.skip_spatial_check = bssc;
    params.deinterlace_params = &dParams;
}

void OptionPb::deband(pl_render_params &params, float val[5])
{
    if (val[0])
    {
        pl_deband_params dParams = pl_deband_default_params;
        dParams.iterations = val[1];
        dParams.threshold = val[2];
        dParams.radius = val[3];
        dParams.grain = val[4];
        // 对于每个通道（从低到高索引）的“中性”噪声值
        // dParams.grain_neutral[0] = 1;
        // dParams.grain_neutral[1] = 1;
        // dParams.grain_neutral[2] = 1;
        params.deband_params = &dParams;
    }
}

void OptionPb::colorAdj(pl_render_params &params, float val[6])
{
    pl_color_adjustment caParams = pl_color_adjustment_neutral;
    caParams.brightness = val[0];
    caParams.contrast = val[1];
    caParams.saturation = val[2];
    caParams.hue = val[3];
    caParams.gamma = val[4];
    caParams.temperature = val[5];
    params.color_adjustment = &caParams;
}

void OptionPb::peakDetect(pl_render_params &params, float val[8])
{
    pl_peak_detect_params pdParams = pl_peak_detect_default_params;
    pdParams.smoothing_period = val[1];
    pdParams.scene_threshold_low = val[2];
    pdParams.scene_threshold_high = val[3];
    pdParams.percentile = val[4];
    pdParams.black_cutoff = val[5];
    pdParams.allow_delayed = val[6];
    pdParams.minimum_peak = val[7];
    params.peak_detect_params = val[0] ? &pdParams : NULL;
}

void OptionPb::colorMap(pl_render_params &params, float val[7])
{
    pl_color_map_params p = pl_color_map_default_params;
    switch ((MY_GAMUT)val[0])
    {
    case GAMUT_MAP_CLIP:
        p.gamut_mapping = &pl_gamut_map_clip;
        break;
    case GAMUT_MAP_PERCEPTUAL:
        p.gamut_mapping = &pl_gamut_map_perceptual;
        break;
    case GAMUT_MAP_RELATIVE:
        p.gamut_mapping = &pl_gamut_map_relative;
        break;
    case GAMUT_MAP_SATURATION:
        p.gamut_mapping = &pl_gamut_map_saturation;
        break;
    case GAMUT_MAP_ABSOLUTE:
        p.gamut_mapping = &pl_gamut_map_absolute;
        break;
    case GAMUT_MAP_DESATURATE:
        p.gamut_mapping = &pl_gamut_map_desaturate;
        break;
    case GAMUT_MAP_DARKEN:
        p.gamut_mapping = &pl_gamut_map_darken;
        break;
    case GAMUT_MAP_HIGHLIGHT:
        p.gamut_mapping = &pl_gamut_map_highlight;
        break;
    case GAMUT_MAP_LINEAR:
        p.gamut_mapping = &pl_gamut_map_linear;
        break;
    default:
        break;
    }
    switch ((MY_TONEMAPPING)val[1])
    {
    case TONE_MAP_AUTO:
        p.tone_mapping_function = &pl_tone_map_auto;
        break;
    case TONE_MAP_CLIP:
        p.tone_mapping_function = &pl_tone_map_clip;
        break;
#if PL_API_VER >= 246
    case TONE_MAP_ST2094_40:
        p.tone_mapping_function = &pl_tone_map_st2094_40;
        break;
    case TONE_MAP_ST2094_10:
        p.tone_mapping_function = &pl_tone_map_st2094_10;
        break;
#endif
    case TONE_MAP_BT2390:
        p.tone_mapping_function = &pl_tone_map_bt2390;
        break;
    case TONE_MAP_BT2446A:
        p.tone_mapping_function = &pl_tone_map_bt2446a;
        break;
    case TONE_MAP_SPLINE:
        p.tone_mapping_function = &pl_tone_map_spline;
        break;
    case TONE_MAP_REINHARD:
        p.tone_mapping_function = &pl_tone_map_reinhard;
        break;
    case TONE_MAP_MOBIUS:
        p.tone_mapping_function = &pl_tone_map_mobius;
        break;
    case TONE_MAP_HABLE:
        p.tone_mapping_function = &pl_tone_map_hable;
        break;
    case TONE_MAP_GAMMA:
        p.tone_mapping_function = &pl_tone_map_gamma;
        break;
    case TONE_MAP_LINEAR:
        p.tone_mapping_function = &pl_tone_map_linear;
        break;
    default:
        break;
    }
    p.tone_mapping_param = val[2],
    p.inverse_tone_mapping = val[3],
    p.lut_size = val[4],
    p.contrast_recovery = val[5],
    p.contrast_smoothness = val[6],
    params.color_map_params = &p;
}

void OptionPb::dither(pl_render_params &params, int val[4])
{
    pl_dither_params dParams = pl_dither_default_params;
    dParams.method = (pl_dither_method)val[0];
    dParams.lut_size = val[1];
    dParams.temporal = val[2];
    dParams.transfer = (pl_color_transfer)val[3];
    params.dither_params = &dParams;
}

void OptionPb::cone(pl_render_params &params, float val[2])
{
    pl_cone_params cParams = pl_vision_normal;
    cParams.cones = (pl_cone)val[0];
    cParams.strength = val[1];
    params.cone_params = &cParams;
}

void OptionPb::SarNormalize(pl_frame inFrame, pl_frame &outFrame, pl_render_params params, bool bN, int padCropRatio)
{
    float aspect = pl_rect2df_aspect(&inFrame.crop);
    // aspect *= av_q2d(in->link->sample_aspect_ratio); // 默认*1
    pl_rect2df_aspect_set(&outFrame.crop, aspect, padCropRatio);
    // pl_frame_mix mix = (struct pl_frame_mix){
    //     .num_frames = 1,
    //     .frames = (const struct pl_frame *[]){&inFrame},
    //     .signatures = (uint64_t[]){0xFFF1},
    //     .timestamps = (float[]){-100, 100},
    //     .vsync_duration = 1.6,
    // };
    // pl_render_image_mix(_render, &mix, &outFrame, &params);
}

void OptionPb::fillcolor(pl_render_params &params)
{
    params.background_color[0] = 0; // R【max = 1.0】
    params.background_color[1] = 0; // G
    params.background_color[2] = 0; // B
}

template <typename T>
inline void OptionPb::colorOpt(MY_OPTION_PB opt, T val, pl_render_params &params)
{
    CONDITION(PB_OPT_CORNER_ROUND, params.corner_rounding = val)
    CONDITION(PB_OPT_COLOR_SPACE, _outAvFrame->colorspace = (AVColorSpace)val)
    CONDITION(PB_OPT_COLOR_RANGE, _outAvFrame->color_range = (AVColorRange)val)
    CONDITION(PB_OPT_COLOR_PRIMARIES, _outAvFrame->color_primaries = (AVColorPrimaries)val)
    CONDITION(PB_OPT_COLOR_TRC, _outAvFrame->color_trc = (AVColorTransferCharacteristic)val)
}

template <typename T>
void OptionPb::opt1Params(MY_OPTION_PB opt, pl_render_params &params, T val)
{
    // 应用圆角效果，将图像的角落进行圆形化处理，模拟一个带有透明度蒙版的效果。
    // 该值表示圆角半径相对于图像边长的比例。
    // 例如： 1.0 表示最大圆角（几乎是一个圆形）; 0.0 表示没有圆角。
    CONDITION(PB_OPT_CORNER_ROUND, params.corner_rounding = val);
    // 控制滤镜的抗环绕强度
    CONDITION(PB_OPT_ANTIRING, params.antiringing_strength = val);
    // 配置在图像缩放之前应用 Sigmoid 函数的参数，通常用于对图像进行非线性调整（如增加对比度或锐化）
    // Sigmoid 函数使图像的亮度或对比度进行 S 形曲线调整，增加亮度较低或较高区域的对比度
    CONDITION(PB_OPT_SIGMOID, params.sigmoid_params = val ? &pl_sigmoid_default_params : NULL);
    // 禁用下采样时的抗锯齿处理
    // 抗锯齿可以平滑图像中的锯齿状边缘，而禁用它可能会导致图像出现严重的摩尔纹（moiré）效应和锯齿。
    // 此设置在高缩放比的下采样（如缩小很多倍）时，可能会加速渲染过程
    CONDITION(PB_OPT_SKIPAA, params.skip_anti_aliasing = val);
    // 禁用图像缩放前的线性化/ Sigmoid化
    // 这通常用于排除一些异常图像伪影或避免过度的环绕效应，尤其是在需要调试或修复图像问题时
    CONDITION(PB_OPT_DISABLE_LINEAR, params.disable_linear_scaling = val);
    // 强制禁用内建的专用缩放算法，即使是常用的高效算法（如 pl_filter_bicubic 等）也会被禁用，
    // 使用通用的（较慢的）缩放实现
    CONDITION(PB_OPT_DISABLE_BUILTIN, params.disable_builtin_scalers = val);
    // 强制启用抖动（dither），即使在渲染到 16 位浮动帧缓冲对象（FBO）时也会启用抖动。
    // 16 位 FBO 的色深已经足够高，通常无需额外的抖动处理，但此选项可以用于调试抖动算法
    CONDITION(PB_OPT_FORCE_DITHER, params.force_dither = val);
    // 完全禁用使用 FBO（帧缓冲对象），相当于假设没有可渲染的纹理格式。
    // 这将禁用大多数使用帧缓冲对象的功能，并且通常会减少性能，特别是在需要渲染到离屏缓冲的场景中
    CONDITION(PB_OPT_DISABLE_FBOS, params.disable_fbos = val);
}

inline void AVFrameWrap::unref()
{
    if (frame != nullptr)
    {
        av_frame_unref(frame);
    }
}

AVFrameWrap::AVFrameWrap(AVFrameWrap &&temp)
{
    operator=(std::move(temp));
}

AVFrameWrap::AVFrameWrap(AVFrameWrap &temp)
{
    operator=(temp);
}

AVFrameWrap::~AVFrameWrap()
{
    if (frame != nullptr)
    {
        av_frame_free(&frame);
        frame = nullptr;
    }
}

inline void AVFrameWrap::operator=(AVFrameWrap &temp)
{
    if (temp.frame == nullptr)
    {
        return;
    }

    if (frame != nullptr)
    {
        unref();
        av_frame_ref(frame, temp.frame);
    }
    else
    {
        frame = av_frame_clone(temp.frame);
    }
}

inline void AVFrameWrap::operator=(AVFrameWrap &&temp)
{
    if (temp.frame == nullptr)
    {
        return;
    }

    if (frame != nullptr)
    {
        unref();
        av_frame_move_ref(frame, temp.frame);
    }
    else
    {
        frame = av_frame_alloc();
        av_frame_move_ref(frame, temp.frame);
    }
}
