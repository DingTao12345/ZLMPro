/*
 * Copyright (c) 2020 The ZLMediaKit project authors. All Rights Reserved.
 * Created by alex on 2021/4/6.
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_UDPPLAYER_H
#define ZLMEDIAKIT_UDPPLAYER_H

#include "Network/Socket.h"
#include "Poller/Timer.h"
#include "Player/PlayerBase.h"
#include "Rtsp/RtpReceiver.h"
#include "Extension/Factory.h"

namespace mediakit {

class UdpPlayer : public PlayerBase, public std::enable_shared_from_this<UdpPlayer>, public RtpMultiReceiver<1> {
public:
    UdpPlayer(const toolkit::EventPoller::Ptr &poller);

    /**
     * 开始播放
     */
    void play(const std::string &url) override;

    /**
     * 停止播放
     */
    void teardown() override;

protected:
    virtual void onTSPacket(const toolkit::Buffer::Ptr &buf, int64_t dts) = 0;
    virtual void onRtpSorted(RtpPacket::Ptr rtp, int index) override;

private:
    void teardown_l();
    void play_l(const std::string &url);
    void onData(const toolkit::Buffer::Ptr &buf);
    void onTSPacket_l(toolkit::Buffer::Ptr buf, int64_t dts, bool buffer_cache_able);
    bool onManager();

private:
    bool _wait_frame = true;
    toolkit::Ticker _play_ticker;
    toolkit::Ticker _frame_ticker;
    toolkit::Timer::Ptr _timer;
    toolkit::Socket::Ptr _sock;
    RtpCodec::Ptr _rtp_decoder;
    toolkit::EventPoller::Ptr _poller;
};

} // namespace mediakit
#endif // ZLMEDIAKIT_UDPPLAYER_H
