#pragma once
#include <memory>

extern "C"
{
#include <libavcodec/avcodec.h>
}

struct AVPacketDeleter
{
    void operator()(AVPacket *packet) const
    {
        av_packet_free(&packet);
    }
};

using unique_avpacket = std::unique_ptr<AVPacket, AVPacketDeleter>;

struct AVFrameDeleter
{
    void operator()(AVFrame *frame) const
    {
        av_frame_free(&frame);
    }
};

using unique_avframe = std::unique_ptr<AVFrame, AVFrameDeleter>;

struct AVCodecContextDeleter
{
    void operator()(AVCodecContext *context) const
    {
        avcodec_free_context(&context);
    }
};

using unique_avcodec_context = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
