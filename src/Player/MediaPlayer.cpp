/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include <algorithm>
#include "MediaPlayer.h"
#include "Common/config.h"
#include "Http/HlsPlayer.h"

using namespace std;
using namespace toolkit;

namespace mediakit {

MediaPlayer::MediaPlayer(const EventPoller::Ptr &poller) {
    _poller = poller ? poller : EventPollerPool::Instance().getPoller();
}

static void setOnCreateSocket_l(const std::shared_ptr<PlayerBase> &delegate, const Socket::onCreateSocket &cb){
    auto helper = dynamic_pointer_cast<SocketHelper>(delegate);
    if (helper) {
        if (cb) {
            helper->setOnCreateSocket(cb);
        } else {
            // 客户端，确保开启互斥锁  [AUTO-TRANSLATED:a75e6e36]
            // Client, ensure mutual exclusion lock is enabled
            helper->setOnCreateSocket([](const EventPoller::Ptr &poller) {
                return Socket::createSocket(poller, true);
            });
        }
    }
}

void MediaPlayer::onShutdown(const toolkit::SockException &ex) {
    while (_demuxer) {
        try {
            //shared_from_this()可能抛异常
            std::weak_ptr<MediaPlayer> weak_self = shared_from_this();
            if (_decoder) {
                _decoder->flush();
            }
            //等待所有frame flush输出后，再触发onShutdown事件
            static_pointer_cast<HlsDemuxer>(_demuxer)->pushTask([weak_self, ex]() {
                if (auto strong_self = weak_self.lock()) {
                    strong_self->_demuxer = nullptr;
                    strong_self->onShutdown(ex);
                }
            });
            return;
        } catch (...) {
            break;
        }
    }

    if (_on_shutdown) {
        _on_shutdown(ex);
    }
}

void MediaPlayer::onFrame(const Frame::Ptr &frame) {
    if (!_decoder && _demuxer) {
        _decoder = DecoderImp::createDecoder(CodecTS, _demuxer.get());
    }
    if (_decoder && _demuxer) {
        _decoder->input((uint8_t *)frame->data(), frame->size());
    }
}

void MediaPlayer::onPlayResult(const toolkit::SockException &ex) {
    _demux_frame = (*this)[Client::kDemuxFrame];
    auto tracks = PlayerImp<PlayerBase, PlayerBase>::getTracks(false);
    auto track = tracks.empty() ? nullptr : tracks.front();
    if (_demux_frame && track && track->getCodecId() == CodecTS) {
        auto benchmark_mode = (*this)[Client::kBenchmarkMode].as<int>();
        if (ex || benchmark_mode) {
            if (_on_play_result) {
                _on_play_result(ex);
            }
        } else {
            auto demuxer = std::make_shared<HlsDemuxer>();
            demuxer->start(getPoller(), this);
            _demuxer = std::move(demuxer);

            std::weak_ptr<MediaPlayer> weak_self = shared_from_this();
            track->addDelegate([weak_self](const Frame::Ptr &frame) {
                if (auto strong_self = weak_self.lock()) {
                    strong_self->onFrame(frame);
                }
                return true;
            });
        }
    } else {
        if (_on_play_result) {
            _on_play_result(ex);
        }
    }
}

void MediaPlayer::addTrackCompleted() {
    if (_on_play_result) {
        _on_play_result(SockException(Err_success, "play success"));
    }
}

void MediaPlayer::play(const string &url) {
    _delegate = PlayerBase::createPlayer(_poller, url);
    assert(_delegate);
    setOnCreateSocket_l(_delegate, _on_create_socket);

    std::weak_ptr<MediaPlayer> weak_self = shared_from_this();
    _delegate->setOnShutdown([weak_self](const toolkit::SockException &ex) {
        if (auto strong_self = weak_self.lock()) {
            strong_self->onShutdown(ex);
        }
    });
    _delegate->setOnPlayResult([weak_self](const toolkit::SockException &ex) {
        if (auto strong_self = weak_self.lock()) {
            strong_self->onPlayResult(ex);
        }
    });
    _delegate->setOnResume(_on_resume);
    _delegate->setMediaSource(_media_src);
    for (auto &pr : *this) {
        (*_delegate)[pr.first] = pr.second;
    }
    _delegate->play(url);
}

vector<Track::Ptr> MediaPlayer::getTracks(bool ready) const {
    if (!_demux_frame) {
        return PlayerImp<PlayerBase, PlayerBase>::getTracks(ready);
    }
    if (!_demuxer) {
        return vector<Track::Ptr>();
    }
    return static_pointer_cast<HlsDemuxer>(_demuxer)->getTracks(ready);
}

EventPoller::Ptr MediaPlayer::getPoller(){
    return _poller;
}

void MediaPlayer::setOnCreateSocket(Socket::onCreateSocket cb){
    setOnCreateSocket_l(_delegate, cb);
    _on_create_socket = std::move(cb);
}

} /* namespace mediakit */
