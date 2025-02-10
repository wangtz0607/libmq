// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <format>

namespace mq {

enum class RPCError : uint8_t {
    kOk = 0,
    kMethodNotFound = 1,
    kBadRequest = 2,
    kBadReply = 3,
    kCancelled = 4,
};

} // namespace mq

template <>
struct std::formatter<mq::RPCError> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(mq::RPCError error, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", name(error));
    }

private:
    static constexpr const char *name(mq::RPCError error) {
        using enum mq::RPCError;
        switch (error) {
            case kOk: return "Ok";
            case kMethodNotFound: return "MethodNotFound";
            case kBadRequest: return "BadRequest";
            case kBadReply: return "BadReply";
            case kCancelled: return "Cancelled";
            default: return nullptr;
        }
    }
};
