#pragma once

#include <drogon/HttpFilter.h>

namespace project_tracker::filters {
    class LoginRequiredFilter : public drogon::HttpFilter<LoginRequiredFilter> {
    public:
        void doFilter(const drogon::HttpRequestPtr &request,
                      drogon::FilterCallback &&failureCallback,
                      drogon::FilterChainCallback &&successCallback) override;
    };
} // namespace project_tracker::filters
