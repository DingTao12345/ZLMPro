/*
 * Copyright (c) 2020 The ZLMediaKit project authors. All Rights Reserved.
 * Created by alex on 2021/4/6.
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "UdpPlayer.h"
#include "Common/config.h"
#include "Util/uv_errno.h"

using namespace std;
using namespace toolkit;

namespace mediakit {

enum class UrlType : int {
    Unknow = -1,
    Rtp = 0,
    Udp = 1,
};

static UrlType parseUrl(const string &media_url, string &ip, uint16_t &port) {
    MediaInfo info(media_url);
    ip = info.host;
    if (ip.front() == '@') {
        ip.erase(0, 1);
    }
    port = info.port;
    if (strcasecmp("rtp", info.schema.data()) == 0) {
        return UrlType::Rtp;
    }
    if (strcasecmp("udp", info.schema.data()) == 0) {
        return UrlType::Udp;
    }
    return UrlType::Unknow;
}

static bool isMulticastAddr(const string &ip) {
    uint32_t addressInNetworkOrder = htonl(inet_addr(ip.data()));
    return addressInNetworkOrder > 0xE00000FF && addressInNetworkOrder <= 0xEFFFFFFF;
}

UdpPlayer::UdpPlayer(const EventPoller::Ptr &poller) {
    _poller = poller ? poller : EventPollerPool::Instance().getPoller();
}

void UdpPlayer::play(const string &url) {
    try {
        teardown_l();
        play_l(url);
        _play_ticker.resetTime();
        _frame_ticker.resetTime();
        std::weak_ptr<UdpPlayer> weak_self = shared_from_this();
        _timer = std::make_shared<Timer>(2,[weak_self]() {
            if (auto strong_self = weak_self.lock()) {
                return strong_self->onManager();
            }
            return false;
        },_poller);
    } catch (SockException &ex) {
        onPlayResult(ex);
    }
}

void UdpPlayer::play_l(const string &url) {
    string ip;
    uint16_t port;
    auto type = parseUrl(url, ip, port);
    if (type == UrlType::Unknow) {
        throw std::invalid_argument("invalid url: " + url);
    }
    if (type == UrlType::Rtp) {
        _rtp_decoder = Factory::getRtpDecoderByCodecId(CodecTS);
        CHECK(_rtp_decoder);
        _rtp_decoder->addDelegate([this](const Frame::Ptr &frame) {
            onTSPacket_l(frame, frame->pts(), frame->cacheAble());
            return true;
        });
    }
    DebugL << "start play: " << url;
    _sock = Socket::createSocket(_poller, false);
    std::weak_ptr<UdpPlayer> weak_self = shared_from_this();
    _sock->setOnMultiRead([weak_self](Buffer::Ptr *buf, struct sockaddr_storage *, size_t count) {
        if (auto strong_self = weak_self.lock()) {
            for (auto i = 0u; i < count; ++i) {
                auto &ptr = *(buf + i);
                strong_self->onData(ptr);
                // 声明已经被转义拷贝走
                ptr = nullptr;
            }
        }
    });

    auto local_ip = (*this)[Client::kNetAdapter];
    if (local_ip.empty()) {
        local_ip = "0.0.0.0";
    }

    if (!isMulticastAddr(ip)) {
        WarnL << "Not a multicast ip: " << ip;
        if (!_sock->bindUdpSock(port, local_ip)) {
            throw SockException(Err_other, StrPrinter << "bind upd socket[" << local_ip << ":" << port << "] failed: " << toolkit::get_uv_errmsg());
        }
        SockUtil::setRecvBuf(_sock->rawFD(), 1024 * 1024);
        return;
    }

    if (!_sock->bindUdpSock(port, ip)) {
        throw SockException(Err_other, StrPrinter << "bind upd socket[" << ip << ":" << port << "] failed: " << toolkit::get_uv_errmsg());
    }
    SockUtil::setRecvBuf(_sock->rawFD(), 1024 * 1024);
    if (-1 == SockUtil::joinMultiAddr(_sock->rawFD(), ip.data(), local_ip.data())) {
        throw SockException(Err_other, StrPrinter << "join multicast address[" << ip << "/" << local_ip << "] failed: " << toolkit::get_uv_errmsg());
    }

    if (local_ip != "0.0.0.0" && -1 == SockUtil::setMultiIF(_sock->rawFD(), local_ip.data())) {
        throw SockException(Err_other, StrPrinter << "set multicast interface [" << local_ip << "] failed: " << toolkit::get_uv_errmsg());
    }
}

void UdpPlayer::teardown() {
    teardown_l();
}

void UdpPlayer::teardown_l() {
    _wait_frame = true;
    _timer = nullptr;
    _sock = nullptr;
    _rtp_decoder = nullptr;
}

bool UdpPlayer::onManager() {
    auto media_timeout = (*this)[Client::kMediaTimeoutMS].as<uint64_t>();
    auto play_timeout = (*this)[Client::kTimeoutMS].as<uint64_t>();
    if (_wait_frame) {
        if (_play_ticker.elapsedTime() > play_timeout) {
            teardown_l();
            onPlayResult(SockException(Err_timeout, "play timeout"));
        }
    } else if (_frame_ticker.elapsedTime() > media_timeout) {
        teardown_l();
        onShutdown(SockException(Err_timeout, "wait data timeout"));
    }
    return true;
}

void UdpPlayer::onData(const toolkit::Buffer::Ptr &buf) {
    if (_rtp_decoder) {
        handleOneRtp(0, TrackVideo, 90000, (uint8_t *)buf->data(), buf->size());
    } else {
        onTSPacket_l(buf, 0, true);
    }
}

void UdpPlayer::onTSPacket_l(toolkit::Buffer::Ptr buf, int64_t dts, bool buffer_cache_able) {
    if (_wait_frame) {
        _wait_frame = false;
        onPlayResult(SockException());
    }
    _frame_ticker.resetTime();
    if (!buffer_cache_able) {
        auto copy_able = BufferRaw::create();
        copy_able->assign(buf->data(), buf->size());
        buf = copy_able;
    }
    onTSPacket(buf, dts);
}

void UdpPlayer::onRtpSorted(RtpPacket::Ptr rtp, int index) {
    _rtp_decoder->inputRtp(rtp, false);
}

} // namespace mediakit