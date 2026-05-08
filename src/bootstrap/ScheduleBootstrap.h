#pragma once

namespace project_tracker::bootstrap {
    /**
     * @brief 注册每日状态更新定时器
     * 
     * 该函数会计算距离下一个 00:00 的时间，并设置一个定时器。
     * 当定时器触发时，会自动将“进行中”且“已过截止日期”的实体状态更新为“已延期”。
     */
    void registerDailyStatusUpdater();
}
