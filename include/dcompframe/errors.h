#pragma once

#include <string>

namespace dcompframe {

enum class ErrorCode {
    Ok = 0,
    InvalidArgument,
    NotInitialized,
    NotFound,
    DeviceFailure,
    IOError,
    ParseError,
    Unsupported,
    Unknown
};

struct Status {
    ErrorCode code = ErrorCode::Ok;
    std::string message;

    [[nodiscard]] bool ok() const {
        return code == ErrorCode::Ok;
    }

    static Status success() {
        return Status {};
    }

    static Status failure(ErrorCode c, std::string msg) {
        return Status {.code = c, .message = std::move(msg)};
    }
};

}  // namespace dcompframe
