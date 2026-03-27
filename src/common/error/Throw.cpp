#include "common/error/Throw.h"

namespace project_tracker::common::error {

void throwBusinessError(
    drogon::HttpStatusCode status,
    ErrorCode code,
    const std::string &message) {
    throw BusinessException(status, code, message);
}

void throwBadRequest(
    ErrorCode code,
    const std::string &message) {
    throwBusinessError(drogon::k400BadRequest, code, message);
}

void throwUnauthorized(
    ErrorCode code,
    const std::string &message) {
    throwBusinessError(drogon::k401Unauthorized, code, message);
}

void throwForbidden(
    ErrorCode code,
    const std::string &message) {
    throwBusinessError(drogon::k403Forbidden, code, message);
}

void throwNotFound(
    ErrorCode code,
    const std::string &message) {
    throwBusinessError(drogon::k404NotFound, code, message);
}

void throwConflict(
    ErrorCode code,
    const std::string &message) {
    throwBusinessError(drogon::k409Conflict, code, message);
}

void throwInternalError(
    ErrorCode code,
    const std::string &message) {
    throwBusinessError(drogon::k500InternalServerError, code, message);
}

} // namespace project_tracker::common::error
