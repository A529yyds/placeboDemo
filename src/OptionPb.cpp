#include "OptionPb.h"
#include "defines.h"

#define PL_LIBAV_IMPLEMENTATION 0
#include <libplacebo/gpu.h>
#include <libplacebo/utils/libav.h>

OptionPb::OptionPb()
{
    _gpu = MyGL::getInstance()->getGpu();
    _render = MyGL::getInstance()->getRenderer();
    _outfile.open(OUTFILE, std::ios::binary);
}

void OptionPb::endOpt()
{
    _outfile.close();
}

bool OptionPb::optPb(AVFrame *inAvFrame, MY_OPTION_PB opt)
{
    bool ret = true;
    pl_frame inFrame = {.planes = {0}};
    // 从AVFrame格式转化为pl_frame格式，只能获取图像的信息，不能获取图像纹理
    pl_frame_from_avframe(&inFrame, inAvFrame);

    pl_tex inTex[3] = {NULL, NULL, NULL}; // 输入纹理初始化
    // 通过avframe的数据映射到pl_frame的纹理上，并将输入纹理上传gpu，理论上inTex可以为一个四维数组
    if (!pl_map_avframe(_gpu, &inFrame, inTex, inAvFrame))
    {
        COUT("ERROR", "pl_map_avframe ret is ", "false");
        return false;
    }
    pl_tex outTex[4] = {NULL, NULL, NULL, NULL}; // 输出纹理初始化
    pl_frame outFrame = {.planes = {0}};
    // 创建输出帧：此处仅需要对输出帧w/h/format进行设置即可
    AVFrame *outAvFrame = av_frame_alloc();
    outAvFrame->width = WIDTH;
    outAvFrame->height = HEIGHT;
    outAvFrame->format = inAvFrame->format;
    // 设置输出格式：
    // 输出格式的值可以随便设置，只需要对后续写入文件的数据平面处理好即可
    // 目前写入文件格式仅设置了YUV422P10LE、YUV420P、NV12
    // 设置渲染参数: 此处采用默认参数
    pl_render_params params = pl_render_default_params;
    switch (opt)
    {
    case PB_OPT_TRANSFER_FMT:
    {
        transferFormat(outAvFrame, AV_PIX_FMT_NV12);
        break;
    }
    case PB_OPT_SCARLER:
    {
        scaler(outAvFrame, 1.5, params, SCALER_BICUBIC);
        break;
    }

    default:
        break;
    }
    // 使用初始化后的out_frame[AVFrame]填充输出纹理和pl_frame
    pl_frame_recreate_from_avframe(_gpu, &outFrame, outTex, outAvFrame);
    if (!pl_render_image(_render, &inFrame, &outFrame, &params))
    {
        COUT("ERROR", "pl_render_image ret is ", "false");
    }
    else
    {
        // 初始化AVFrame的data数据[不初始化无法下载纹理]
        av_frame_get_buffer(outAvFrame, 32);
        // 从gpu下载纹理并填充到输出帧
        if (pl_download_avframe(_gpu, &outFrame, outAvFrame))
        {
            writeFiles(outAvFrame);
        }
        // 释放out_frame的资源
        av_frame_free(&outAvFrame);
    }
    // 释放纹理资源
    pl_tex_destroy(_gpu, &inTex[0]);
    pl_tex_destroy(_gpu, &inTex[1]);
    pl_tex_destroy(_gpu, &inTex[2]);
    pl_tex_destroy(_gpu, &outTex[0]);
    pl_tex_destroy(_gpu, &outTex[1]);
    pl_tex_destroy(_gpu, &outTex[2]);
    pl_tex_destroy(_gpu, &outTex[3]);
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

void OptionPb::transferFormat(AVFrame *outAvFrame, int outFmt)
{
    // 设置输出格式：
    outAvFrame->format = (AVPixelFormat)outFmt;
}

void OptionPb::scaler(AVFrame *outFrame, int r, pl_render_params &params, MY_SCALER_TYPE type)
{
    // 放大倍数设为 r 倍
    outFrame->width = WIDTH * r;
    outFrame->height = HEIGHT * r;
    // 放大算法
    params.upscaler = &pl_filter_bicubic;
    // params.upscaler = &pl_filter_nearest;
}
