#pragma once

#include <drogon/HttpFilter.h>

namespace project_tracker::filters {
    class AdminRequiredFilter : public drogon::HttpFilter<AdminRequiredFilter> {
    public:
        void doFilter(const drogon::HttpRequestPtr &request,
                      drogon::FilterCallback &&failureCallback,
                      drogon::FilterChainCallback &&successCallback) override;
    };
} // namespace project_tracker::filters
