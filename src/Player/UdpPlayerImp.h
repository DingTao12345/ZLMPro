/*
 * Copyright (c) 2020 The ZLMediaKit project authors. All Rights Reserved.
 * Created by alex on 2021/4/6.
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_UDPPLAYERIMP_H
#define ZLMEDIAKIT_UDPPLAYERIMP_H

#include "UdpPlayer.h"

namespace mediakit {

class UdpPlayerImp : public PlayerImp<UdpPlayer, PlayerBase> {
public:
    using Ptr = std::shared_ptr<UdpPlayerImp>;

    UdpPlayerImp(const toolkit::EventPoller::Ptr &poller = nullptr);

private:
    //// PlayerBase override////
    void onPlayResult(const toolkit::SockException &ex) override;
    std::vector<Track::Ptr> getTracks(bool ready = true) const override;

    //// UdpPlayer override////
    void onTSPacket(const toolkit::Buffer::Ptr &buf, int64_t dts) override;

private:
    Track::Ptr _track;
};

} // namespace mediakit
#endif // ZLMEDIAKIT_UDPPLAYERIMP_H