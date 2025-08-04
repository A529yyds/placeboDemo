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
    PB_OPT_TRANSFER_FMT = 0,
    PB_OPT_SCARLER = 1,
};

enum MY_SCALER_TYPE
{
    SCALER_NEAREST = 0,
    SCALER_BICUBIC = 1,
};

class OptionPb
{
public:
    OptionPb();
    bool optPb(AVFrame *inAVFrame, MY_OPTION_PB opt = PB_OPT_TRANSFER_FMT);
    void endOpt();

private:
    int writeFiles(AVFrame *outAvFrame);
    void transferFormat(AVFrame *outFrame, int outFmt);
    void scaler(AVFrame *outFrame, int r, pl_render_params &params, MY_SCALER_TYPE type = SCALER_NEAREST);

private:
    char *_inFileName;
    char *_outFileName;
    bool _bCLoseFile = false;
    std::ofstream _outfile;
    pl_gpu _gpu;
    pl_renderer _render;
};