
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <string>

class BuildInfo {
public:
    enum class Channel {
        Nightly, Beta, RC, Release
    };

    static const        Channel     channel       = Channel::Nightly;
    inline static const std::string channel_iter  = "0";

    inline static const std::string version_major = "2026";
    inline static const std::string version_minor = "0";

    static std::string ChannelPrint() {
        switch (channel) {
            case Channel::Nightly:
                return std::format("Nightly ({})", __DATE__);
            case Channel::Beta:
                return "Beta " + channel_iter;
            case Channel::RC:
                return "RC " + channel_iter;
            case Channel::Release:
                return "";
        }
        return "";
    }
    static std::string Print() {
        return std::format("Xero {}.{} {}", 
            version_major, version_minor, ChannelPrint()
        );
    }
};
