
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>

class BuildInfo {
public:
    enum class Channel {
        Nightly, Beta, RC, Release
    };

    static const        Channel     channel_       = Channel::Beta;
    inline static const std::string channel_iter_  = "2";

    inline static const std::string version_major_ = "2026";
    inline static const std::string version_minor_ = "0";
    inline static const std::string version_patch_ = "0";

    static std::string ChannelPrint() {
        switch (channel_) {
            case Channel::Nightly:  return std::format("Nightly ({})", __DATE__);
            case Channel::Beta:     return "Beta " + channel_iter_;
            case Channel::RC:       return "RC " + channel_iter_;
            case Channel::Release:  return "";
        }
        return "";
    }
    static std::string Print() {
        return std::format("Xero {}.{}.{} {}", 
            version_major_, version_minor_, version_patch_, ChannelPrint()
        );
    }
};
