/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "Extension/CommonRtp.h"
#include "Extension/Factory.h"
#include "Rtp/TSDecoder.h"

using namespace std;
using namespace toolkit;

namespace mediakit {
namespace {

Frame::Ptr getFrameFromPtr(const char *data, size_t bytes, uint64_t dts, uint64_t pts);

class TSSdp : public Sdp {
public:
    TSSdp(CodecId codecId, int payload_type, int sample_rate, int bitrate) : Sdp(sample_rate, payload_type) {
        _printer << "m=video 0 RTP/AVP " << payload_type << "\r\n";
        if (bitrate) {
            _printer << "b=AS:" << bitrate << "\r\n";
        }
        _printer << "a=rtpmap:" << payload_type << " " << getCodecName(codecId) << "/" << sample_rate << "\r\n";
    }

    string getSdp() const override { return _printer; }

private:
    _StrPrinter _printer;
};

class TSTrack : public VideoTrackImp {
public:
    TSTrack(): VideoTrackImp(CodecTS, 0, 0, 0), _segment(188 * 5) {
        setup();
    }

    TSTrack(const TSTrack &that): VideoTrackImp(that), _segment(188 * 5) {
        setup();
    }

    Track::Ptr clone() const override { return std::make_shared<TSTrack>(*this); }

    Sdp::Ptr getSdp(uint8_t) const override {
        if (!ready()) {
            WarnL << getCodecName() << " Track未准备好";
            return nullptr;
        }
        return std::make_shared<TSSdp>(CodecTS, Rtsp::PT_MP2T, 9000, getBitRate() / 1024);
    }

#if 1
    bool inputFrame(const Frame::Ptr &frame) override {
        _ts = frame->dts();
        _segment.input(frame->data(), frame->size());
        return true;
    }
#endif

private:
    void setup() {
        _segment.setOnSegment([this](const char *data, size_t len) {
            auto frame = getFrameFromPtr(data, len, _ts, _ts);
            VideoTrackImp::inputFrame(frame);
        });
    }

private:
    uint64_t _ts;
    TSSegment _segment;
};

CodecId getCodec() {
    return CodecTS;
}

Track::Ptr getTrackByCodecId(int, int, int) {
    return std::make_shared<TSTrack>();
}

Track::Ptr getTrackBySdp(const SdpTrack::Ptr &) {
    return getTrackByCodecId(0, 0, 0);
}

RtpCodec::Ptr getRtpEncoderByCodecId(uint8_t) {
    return std::make_shared<CommonRtpEncoder>();
}

RtpCodec::Ptr getRtpDecoderByCodecId() {
    return std::make_shared<CommonRtpDecoder>(CodecTS);
}

RtmpCodec::Ptr getRtmpEncoderByTrack(const Track::Ptr &) {
    return nullptr;
}

RtmpCodec::Ptr getRtmpDecoderByTrack(const Track::Ptr &) {
    return nullptr;
}

Frame::Ptr getFrameFromPtr(const char *data, size_t bytes, uint64_t dts, uint64_t pts) {
    return std::make_shared<FrameFromPtr>(CodecTS, (char *)data, bytes, dts, pts, 0, true);
}

} // namespace

CodecPlugin mpegts_plugin = {
                            getCodec,
                            getTrackByCodecId,
                            getTrackBySdp,
                            getRtpEncoderByCodecId,
                            getRtpDecoderByCodecId,
                            getRtmpEncoderByTrack,
                            getRtmpDecoderByTrack,
                            getFrameFromPtr };

} // namespace mediakit
