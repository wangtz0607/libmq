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
            case kOk: return "OK";
            case kMethodNotFound: return "METHOD_NOT_FOUND";
            case kBadRequest: return "BAD_REQUEST";
            case kBadReply: return "BAD_REPLY";
            case kCancelled: return "CANCELLED";
            default: return nullptr;
        }
    }
};
