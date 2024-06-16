/*
 * Copyright (c) 2020 The ZLMediaKit project authors. All Rights Reserved.
 * Created by alex on 2021/4/6.
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "UdpPlayerImp.h"
#include "Common/config.h"
#include "Extension/Factory.h"

using namespace std;
using namespace toolkit;

namespace mediakit {

UdpPlayerImp::UdpPlayerImp(const EventPoller::Ptr &poller)
    : PlayerImp<UdpPlayer, PlayerBase>(poller) {}

void UdpPlayerImp::onTSPacket(const toolkit::Buffer::Ptr &buf, int64_t dts) {
    auto ts = dts ? dts : getCurrentMillisecond();
    _track->inputFrame(Factory::getFrameFromBuffer(CodecTS, buf, ts, ts));
}

void UdpPlayerImp::onPlayResult(const SockException &ex) {
    _track = Factory::getTrackByCodecId(CodecTS);
    PlayerImp<UdpPlayer, PlayerBase>::onPlayResult(ex);
}

vector<Track::Ptr> UdpPlayerImp::getTracks(bool ready) const {
    return { _track };
}

} // namespace mediakit