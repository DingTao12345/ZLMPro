/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef SRC_PLAYER_MEDIAPLAYER_H_
#define SRC_PLAYER_MEDIAPLAYER_H_

#include <memory>
#include <string>
#include "PlayerBase.h"
#include "Rtp/Decoder.h"

namespace mediakit {

class MediaPlayer : public PlayerImp<PlayerBase, PlayerBase>,  private TrackListener, public std::enable_shared_from_this<MediaPlayer>{
public:
    using Ptr = std::shared_ptr<MediaPlayer>;

    MediaPlayer(const toolkit::EventPoller::Ptr &poller = nullptr);

    void play(const std::string &url) override;
    toolkit::EventPoller::Ptr getPoller();
    void setOnCreateSocket(toolkit::Socket::onCreateSocket cb);

    //// PlayerBase override////
    std::vector<Track::Ptr> getTracks(bool ready = true) const override;
    void onShutdown(const toolkit::SockException &ex) override;
    void onPlayResult(const toolkit::SockException &ex) override;

private:
    //// TrackListener override////
    bool addTrack(const Track::Ptr &track) override { return true; };
    void addTrackCompleted() override;

    void onFrame(const Frame::Ptr &frame);

private:
    bool _demux_frame = true;
    DecoderImp::Ptr _decoder;
    MediaSinkInterface::Ptr _demuxer;
    toolkit::EventPoller::Ptr _poller;
    toolkit::Socket::onCreateSocket _on_create_socket;
};

} /* namespace mediakit */

#endif /* SRC_PLAYER_MEDIAPLAYER_H_ */
