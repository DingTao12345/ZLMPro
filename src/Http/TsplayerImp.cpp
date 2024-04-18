/*
 * Copyright (c) 2020 The ZLMediaKit project authors. All Rights Reserved.
 * Created by alex on 2021/4/6.
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "TsPlayerImp.h"
#include "HlsPlayer.h"
#include "Common/config.h"
#include "Extension/Factory.h"

using namespace std;
using namespace toolkit;

namespace mediakit {

TsPlayerImp::TsPlayerImp(const EventPoller::Ptr &poller) : PlayerImp<TsPlayer, PlayerBase>(poller) {}

void TsPlayerImp::onResponseBody(const char *data, size_t len) {
    TsPlayer::onResponseBody(data, len);
    auto ts = getCurrentMillisecond();
   _track->inputFrame(Factory::getFrameFromPtr(CodecTS, data, len, ts, ts));
}

void TsPlayerImp::onPlayResult(const SockException &ex) {
    _track = Factory::getTrackByCodecId(CodecTS);
    PlayerImp<TsPlayer, PlayerBase>::onPlayResult(ex);
}

vector<Track::Ptr> TsPlayerImp::getTracks(bool ready) const {
    return { _track };
}

}//namespace mediakit