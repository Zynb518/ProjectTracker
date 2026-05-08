#include "ScheduleBootstrap.h"
#include <drogon/drogon.h>
#include <trantor/utils/Date.h>
#include <chrono>

namespace project_tracker::bootstrap {

    static trantor::Date getNextMidnight() {
        using namespace std::chrono;
        
        // 获取当前系统时间点
        auto now = system_clock::now();
        
        // 转换为天数（自 epoch 以来），然后加1天得到明天
        auto days_since_epoch = floor<days>(now);
        auto next_midnight_tp = days_since_epoch + days(1);
        
        // 转换为微秒，以符合 trantor::Date 的构造函数要求
        auto micros = duration_cast<microseconds>(next_midnight_tp.time_since_epoch()).count();
        
        return trantor::Date(static_cast<int64_t>(micros));
    }

    void registerDailyStatusUpdater() {
        auto nextMidnight = getNextMidnight();
        
        LOG_INFO << "每日状态巡检已调度 | 下次触发时间: " << nextMidnight.toFormattedString(false);

        drogon::app().getLoop()->runAt(nextMidnight, []() {
            auto dbClient = drogon::app().getDbClient();
            
            LOG_INFO << "开始每日状态巡检：检查进行中的 任务/阶段/项目 是否已延期...";

            // 1. 子任务：进行中 -> 已延期 (4)
            dbClient->execSqlAsync(
                "UPDATE sub_task SET status = 4 WHERE status = 2 AND planned_end_date < CURRENT_DATE",
                [](const drogon::orm::Result &r) {
                    if (r.affectedRows() > 0) {
                        LOG_INFO << "每日巡检：已更新 " << r.affectedRows() << " 个延期子任务";
                    }
                },
                [](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "每日巡检更新子任务失败: " << e.base().what();
                }
            );

            // 2. 阶段节点：进行中 -> 已延期 (4)
            dbClient->execSqlAsync(
                "UPDATE project_node SET status = 4 WHERE status = 2 AND planned_end_date < CURRENT_DATE",
                [](const drogon::orm::Result &r) {
                    if (r.affectedRows() > 0) {
                        LOG_INFO << "每日巡检：已更新 " << r.affectedRows() << " 个延期阶段节点";
                    }
                },
                [](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "每日巡检更新阶段节点失败: " << e.base().what();
                }
            );

            // 3. 项目：进行中 -> 已延期 (4)
            dbClient->execSqlAsync(
                "UPDATE project SET status = 4 WHERE status = 2 AND planned_end_date < CURRENT_DATE",
                [](const drogon::orm::Result &r) {
                    if (r.affectedRows() > 0) {
                        LOG_INFO << "每日巡检：已更新 " << r.affectedRows() << " 个延期项目";
                    }
                },
                [](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "每日巡检更新项目失败: " << e.base().what();
                }
            );

            // 重新注册明天的定时器
            registerDailyStatusUpdater();
        });
    }
}
