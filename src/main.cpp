#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#define PL_LIBAV_IMPLEMENTATION 0
#include <libplacebo/gpu.h>
#include <libplacebo/utils/libav.h>

#include "defines.h"
#include "OptionPb.h"
#include <chrono>
#include <thread>

AVFormatContext *_fmtCtx = nullptr;
AVCodecContext *_codecCtx = nullptr;
int _videoStreamIndex = -1;

int InitFFmpeg(const char *filename)
{
    avformat_network_init();
    if (avformat_open_input(&_fmtCtx, filename, nullptr, nullptr) < 0)
    {
        CERR("ERROR", "avformat_open_input failed!");
        return -1;
    }
    if (avformat_find_stream_info(_fmtCtx, nullptr) < 0)
    {
        CERR("ERROR", "avformat_find_stream_info failed!");
        return -1;
    }
    for (unsigned i = 0; i < _fmtCtx->nb_streams; ++i)
    {
        if (_fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            _videoStreamIndex = i;
            break;
        }
    }
    if (_videoStreamIndex < 0)
    {
        CERR("ERROR", "_videoStreamIndex < 0");
        return -1;
    }
    AVCodecParameters *codecpar = _fmtCtx->streams[_videoStreamIndex]->codecpar;
    const AVCodec *decoder = avcodec_find_decoder(codecpar->codec_id);
    if (!decoder)
    {
        CERR("ERROR", "vavcodec_find_decoder failed");
        avformat_close_input(&_fmtCtx);
        return -1;
    }
    _codecCtx = avcodec_alloc_context3(decoder);
    if (!_codecCtx)
    {
        CERR("ERROR", "avcodec_alloc_context3 failed");
        avcodec_free_context(&_codecCtx);
        avformat_close_input(&_fmtCtx);
        return -1;
    }
    if (avcodec_parameters_to_context(_codecCtx, codecpar) < 0)
    {
        CERR("ERROR", "avcodec_parameters_to_context failed");
        avcodec_free_context(&_codecCtx);
        avformat_close_input(&_fmtCtx);
        return -1;
    }
    if (avcodec_open2(_codecCtx, decoder, nullptr) < 0)
    {
        CERR("ERROR", "avcodec_open2 failed");
        avcodec_free_context(&_codecCtx);
        avformat_close_input(&_fmtCtx);
        return -1;
    }
}

void codecTask(OptionPb *opt)
{
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    bool stopFlag = false;
    AVFrameWrap wFrame;
    wFrame.frame = av_frame_alloc();
    while (av_read_frame(_fmtCtx, pkt) >= 0)
    {
        if (pkt->stream_index == _videoStreamIndex)
        {
            if (avcodec_send_packet(_codecCtx, pkt) == 0)
            {
                while (avcodec_receive_frame(_codecCtx, frame) == 0)
                {
                    frame->pts = pkt->pts;
                    wFrame.frame = frame;
                    opt->pushAVframe(wFrame);
                    // if (opt->optPb(frame))
                    // {
                    //     stopFlag = true;
                    //     break;
                    // }
                }
                // if (stopFlag)
                //     break;
            }
        }
        av_packet_unref(pkt);
    }
    // av_frame_free(&frame);
    // frame = nullptr;
    av_packet_free(&pkt);
    // wFrame.unref();
    opt->stopTask();
}

void placenoOptTask(OptionPb *opt)
{
    // bool bret = opt->handleOpt(PB_OPT_DOWNSCARLER);
    auto start = std::chrono::steady_clock::now();
    opt->handleOpt(PB_OPT_DOWNSCARLER);
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    // 将时间差转换为 uint32_t
    uint32_t time_diff_ms = static_cast<uint32_t>(duration.count());
    // 输出时间差
    std::cout << "Time difference: " << time_diff_ms << " seconds" << std::endl;
    avcodec_free_context(&_codecCtx);
    avformat_close_input(&_fmtCtx);
    DELETE(opt);
}

int main(int argc, char *argv[])
{
    OptionPb *opt = new OptionPb();
    InitFFmpeg(VIDEOFILE);

    // auto start = std::chrono::steady_clock::now();
    // std::thread workerThread(codecTask, opt);
    std::thread workerThread1(codecTask, opt);
    // workerThread.join();
    // codecTask(opt);
    placenoOptTask(opt);
    workerThread1.join();

    // if (opt->bEndTask())
    // {
    // }
    // //
    // avcodec_free_context(&_codecCtx);
    // avformat_close_input(&_fmtCtx);
    // DELETE(opt);
    // auto start = std::chrono::steady_clock::now();
    // auto end = std::chrono::steady_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    // // 将时间差转换为 uint32_t
    // uint32_t time_diff_ms = static_cast<uint32_t>(duration.count());
    // // 输出时间差
    // std::cout << "Time difference: " << time_diff_ms << " seconds" << std::endl;

    return 0;
}
