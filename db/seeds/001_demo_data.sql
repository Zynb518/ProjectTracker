-- 运维工单管理系统 演示数据种子脚本
--
-- 说明：
-- 1. 本脚本会先清空数据，再插入一组专门定制的运维工单假数据。
-- 2. 所有用户默认密码均为 123456。
-- 3. 本脚本建立在 001_init_schema.sql 结构之上。

BEGIN;

TRUNCATE TABLE
    device_usage_log,
    device,
    project_template_node,
    sub_task_progress,
    sub_task,
    project_node,
    project_member,
    project_template,
    project,
    sys_user
RESTART IDENTITY CASCADE;

-- 1. 插入用户数据
INSERT INTO sys_user (
    username,
    password_hash,
    real_name,
    system_role,
    email,
    phone,
    status,
    created_at,
    updated_at
) VALUES
    (
        'admin',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '系统管理员',
        1,
        'admin@example.com',
        '13800000001',
        1,
        '2026-03-01 09:00:00+08:00',
        '2026-03-01 09:00:00+08:00'
    ),
    (
        'li_ming',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '李明',
        2,
        'li_ming@example.com',
        '13800000002',
        1,
        '2026-03-02 09:30:00+08:00',
        '2026-03-12 10:00:00+08:00'
    ),
    (
        'wang_jie',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '王杰',
        2,
        'wang_jie@example.com',
        '13800000003',
        1,
        '2026-03-03 10:00:00+08:00',
        '2026-03-13 11:00:00+08:00'
    ),
    (
        'chen_xu',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '陈旭',
        3,
        'chen_xu@example.com',
        '13800000004',
        1,
        '2026-03-04 10:30:00+08:00',
        '2026-03-14 11:30:00+08:00'
    ),
    (
        'zhao_yue',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '赵月',
        3,
        'zhao_yue@example.com',
        '13800000005',
        1,
        '2026-03-05 11:00:00+08:00',
        '2026-03-15 12:00:00+08:00'
    );

-- 2. 插入工单数据 (原 project)
INSERT INTO project (
    name,
    description,
    owner_user_id,
    status,
    planned_start_date,
    planned_end_date,
    completed_at,
    created_by,
    created_at,
    updated_at
) VALUES
    (
        '【故障】省级政务专网骨干干线光缆中断告警',
        '省级政务外网核心节点发生双向光衰超限告警，影响政务连通性，疑似干线光缆被野蛮施工挖断。',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        2, -- 进行中
        '2026-06-01',
        '2026-06-30',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-06-01 09:00:00+08:00',
        '2026-06-20 18:00:00+08:00'
    ),
    (
        '【需求】市公安局第二数据中心新增防火墙策略',
        '因视频监控网络扩容，需在第二数据中心边界防火墙上新增3条安全访问规则，开通特定端口。',
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        1, -- 未开始 (待审批)
        '2026-06-15',
        '2026-07-15',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-06-05 09:00:00+08:00',
        '2026-06-20 16:30:00+08:00'
    ),
    (
        '【配置】核心网络交换机季度例行固件与补丁升级',
        '对全市76台核心网络节点交换机执行季度例行漏洞修复与SSH安全策略加固。',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        3, -- 已完成
        '2026-05-01',
        '2026-05-30',
        '2026-05-28 17:30:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-05-01 09:30:00+08:00',
        '2026-05-28 17:30:00+08:00'
    ),
    (
        '【故障】政务大楼B座接入层交换机成批掉线告警',
        '政务B座12-15层发生办公网大面积中断，多台交换机心跳丢失，指示灯红色，疑似供电模块故障。',
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        4, -- 已延期 (超时预警，2026-06-20 此时点已过 planned_end_date)
        '2026-06-01',
        '2026-06-15',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-06-01 10:00:00+08:00',
        '2026-06-20 10:00:00+08:00'
    );

-- 3. 插入工单成员 (原 project_member)
INSERT INTO project_member (project_id, user_id, added_by, joined_at)
SELECT p.id, u.id, (SELECT id FROM sys_user WHERE username = 'admin'), CURRENT_TIMESTAMP
FROM project p, sys_user u
WHERE u.username IN ('li_ming', 'wang_jie', 'chen_xu', 'zhao_yue');

-- 4. 插入工单流转节点 (原 project_node)
-- 为每个工单默认创建 3 个流程节点，严格按 sequence_no 流转
INSERT INTO project_node (
    project_id,
    name,
    description,
    sequence_no,
    status,
    planned_start_date,
    planned_end_date,
    completed_at,
    created_by,
    created_at,
    updated_at
) VALUES
    -- 工单 1 的流转阶段
    (
        (SELECT id FROM project WHERE name = '【故障】省级政务专网骨干干线光缆中断告警'),
        '部门审批阶段',
        '审批工单合理性，由省级主管对资源及方案进行审核。',
        1,
        3, -- 已完成
        '2026-06-01',
        '2026-06-10',
        '2026-06-08 11:00:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-06-01 09:00:00+08:00',
        '2026-06-08 11:00:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '【故障】省级政务专网骨干干线光缆中断告警'),
        '派单执行阶段',
        '审批通过，指派外线运维班组赶赴现场测试并抢修。',
        2,
        2, -- 进行中
        '2026-06-11',
        '2026-06-25',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-06-01 09:00:00+08:00',
        '2026-06-11 09:00:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '【故障】省级政务专网骨干干线光缆中断告警'),
        '现场回单阶段',
        '熔接完成，提报光测试报告，申请工单归档结单。',
        3,
        1, -- 未开始
        '2026-06-26',
        '2026-06-30',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-06-01 09:00:00+08:00',
        '2026-06-01 09:00:00+08:00'
    ),

    -- 工单 2 的流转阶段 (均未开始)
    (
        (SELECT id FROM project WHERE name = '【需求】市公安局第二数据中心新增防火墙策略'),
        '部门审批阶段',
        '主管初审策略合理性与合规性要求。',
        1,
        1,
        '2026-06-15',
        '2026-06-25',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-05 09:00:00+08:00',
        '2026-06-05 09:00:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '【需求】市公安局第二数据中心新增防火墙策略'),
        '派单执行阶段',
        '安全组配置导入与策略发布。',
        2,
        1,
        '2026-06-26',
        '2026-07-05',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-05 09:00:00+08:00',
        '2026-06-05 09:00:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '【需求】市公安局第二数据中心新增防火墙策略'),
        '现场回单阶段',
        '策略测试连通性验证，工单审核结单。',
        3,
        1,
        '2026-07-06',
        '2026-07-15',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-05 09:00:00+08:00',
        '2026-06-05 09:00:00+08:00'
    ),

    -- 工单 3 的流转阶段 (全部完成)
    (
        (SELECT id FROM project WHERE name = '【配置】核心网络交换机季度例行固件与补丁升级'),
        '部门审批阶段',
        '审核固件兼容性与灰度升级实施步骤。',
        1,
        3,
        '2026-05-01',
        '2026-05-10',
        '2026-05-08 10:00:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-05-01 09:30:00+08:00',
        '2026-05-08 10:00:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '【配置】核心网络交换机季度例行固件与补丁升级'),
        '派单执行阶段',
        '运维班组夜间窗口期执行固件升级与策略配置。',
        2,
        3,
        '2026-05-11',
        '2026-05-25',
        '2026-05-24 04:30:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-05-01 09:30:00+08:00',
        '2026-05-24 04:30:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '【配置】核心网络交换机季度例行固件与补丁升级'),
        '现场回单阶段',
        '漏洞扫描对比，工单完工结单。',
        3,
        3,
        '2026-05-26',
        '2026-05-30',
        '2026-05-28 17:30:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-05-01 09:30:00+08:00',
        '2026-05-28 17:30:00+08:00'
    ),

    -- 工单 4 的流转阶段 (已超时延期)
    (
        (SELECT id FROM project WHERE name = '【故障】政务大楼B座接入层交换机成批掉线告警'),
        '部门审批阶段',
        '初审告警真实性并审核紧急响应机制。',
        1,
        3,
        '2026-06-01',
        '2026-06-05',
        '2026-06-04 11:20:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-01 10:00:00+08:00',
        '2026-06-04 11:20:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '【故障】政务大楼B座接入层交换机成批掉线告警'),
        '派单执行阶段',
        '抢修班组排查PoE供电及设备硬件指示状态。',
        2,
        2, -- 进行中，但因 2026-06-10 截止日期已过且未完成，故表现为超时状态
        '2026-06-06',
        '2026-06-10',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-01 10:00:00+08:00',
        '2026-06-06 09:00:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '【故障】政务大楼B座接入层交换机成批掉线告警'),
        '现场回单阶段',
        '提交更换模块运行日志，完成回单。',
        3,
        1,
        '2026-06-11',
        '2026-06-15',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-01 10:00:00+08:00',
        '2026-06-01 10:00:00+08:00'
    );

-- 5. 插入派单任务 (原 sub_task)
INSERT INTO sub_task (
    node_id,
    name,
    description,
    responsible_user_id,
    status,
    progress_percent,
    priority,
    planned_start_date,
    planned_end_date,
    completed_at,
    created_by,
    created_at,
    updated_at
) VALUES
    -- 工单 1 任务
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【故障】省级政务专网骨干干线光缆中断告警') AND name = '部门审批阶段'),
        '【审批】工单初审与合规复核',
        '复核告警监控点以及施工申请范围。',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        3, -- 已完成
        100,
        3,
        '2026-06-01',
        '2026-06-08',
        '2026-06-08 11:00:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-06-01 09:00:00+08:00',
        '2026-06-08 11:00:00+08:00'
    ),
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【故障】省级政务专网骨干干线光缆中断告警') AND name = '派单执行阶段'),
        '【排障】前往光交箱用 OTDR 测距故障点',
        '用光时域反射仪测量故障点公里数，精准定位断纤位置。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        3, -- 已完成
        100,
        3,
        '2026-06-11',
        '2026-06-15',
        '2026-06-14 15:30:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-06-11 09:00:00+08:00',
        '2026-06-14 15:30:00+08:00'
    ),
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【故障】省级政务专网骨干干线光缆中断告警') AND name = '派单执行阶段'),
        '【抢修】阻水光缆熔接与跳线重新测试',
        '现场清理断点，拉设临时光缆并重新进行多纤芯熔接。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        2, -- 进行中
        50,
        3,
        '2026-06-16',
        '2026-06-25',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-06-11 09:00:00+08:00',
        '2026-06-20 18:00:00+08:00'
    ),
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【故障】省级政务专网骨干干线光缆中断告警') AND name = '现场回单阶段'),
        '【回单】提交双向光衰测试报告并申请结单',
        '上传测试波形与熔接芯数报告，向主管提请归档。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        1, -- 未开始
        0,
        3,
        '2026-06-26',
        '2026-06-30',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-06-01 09:00:00+08:00',
        '2026-06-20 18:00:00+08:00'
    ),

    -- 工单 2 任务
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【需求】市公安局第二数据中心新增防火墙策略') AND name = '部门审批阶段'),
        '【审批】评估端口暴露风险与防火墙策略合规性',
        '分析新增开通端口是否符合安全审计规范，防止恶意端口外露。',
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        1, -- 未开始
        0,
        2,
        '2026-06-15',
        '2026-06-25',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-05 09:00:00+08:00',
        '2026-06-05 09:00:00+08:00'
    ),
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【需求】市公安局第二数据中心新增防火墙策略') AND name = '派单执行阶段'),
        '【配置】登录边界防火墙下发开通端口策略',
        '在安全网关上执行路由和IP段绑定，开通新增端口。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        1, -- 未开始
        0,
        2,
        '2026-06-26',
        '2026-07-05',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-05 09:00:00+08:00',
        '2026-06-05 09:00:00+08:00'
    ),
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【需求】市公安局第二数据中心新增防火墙策略') AND name = '现场回单阶段'),
        '【回单】配合市局技术人员进行连通性测试',
        '远程telnet确认端口开通无误，核对工单并提交回单。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        1, -- 未开始
        0,
        2,
        '2026-07-06',
        '2026-07-15',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-05 09:00:00+08:00',
        '2026-06-05 09:00:00+08:00'
    ),

    -- 工单 3 任务 (全完成)
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【配置】核心网络交换机季度例行固件与补丁升级') AND name = '部门审批阶段'),
        '【审批】审计升级方案可行性与安全回滚步骤',
        '检查升级脚本及应急预案，规避全网交换机断网风险。',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        3,
        100,
        2,
        '2026-05-01',
        '2026-05-08',
        '2026-05-08 10:00:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-05-01 09:30:00+08:00',
        '2026-05-08 10:00:00+08:00'
    ),
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【配置】核心网络交换机季度例行固件与补丁升级') AND name = '派单执行阶段'),
        '【执行】下载固件，窗口期执行升级配置',
        '凌晨 02:00 - 05:00 执行固件写入与热重启。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        3,
        100,
        2,
        '2026-05-11',
        '2026-05-24',
        '2026-05-24 04:30:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-05-11 09:00:00+08:00',
        '2026-05-24 04:30:00+08:00'
    ),
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【配置】核心网络交换机季度例行固件与补丁升级') AND name = '现场回单阶段'),
        '【回单】提交对比日志并终审结单',
        '上传配置比对文件，扫描修复后版本，申请工单结单。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        3,
        100,
        2,
        '2026-05-26',
        '2026-05-28',
        '2026-05-28 17:30:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-05-26 09:00:00+08:00',
        '2026-05-28 17:30:00+08:00'
    ),

    -- 工单 4 任务 (已超时)
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【故障】政务大楼B座接入层交换机成批掉线告警') AND name = '部门审批阶段'),
        '【审批】对大面积断网事件进行应急审核',
        '确认故障波及范围，审核并启动二级别网络故障应急预案。',
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        3, -- 已完成
        100,
        3,
        '2026-06-01',
        '2026-06-05',
        '2026-06-04 11:20:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-01 10:00:00+08:00',
        '2026-06-04 11:20:00+08:00'
    ),
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【故障】政务大楼B座接入层交换机成批掉线告警') AND name = '派单执行阶段'),
        '【执行】到现场测试各楼层机房供电与配电柜',
        '定位是否因为机房空气开关跳闸或PoE电源模板击穿导致大面积红光。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        2, -- 进行中，且已超时
        30,
        3,
        '2026-06-06',
        '2026-06-10',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-06 09:00:00+08:00',
        '2026-06-20 10:00:00+08:00'
    ),
    (
        (SELECT id FROM project_node WHERE project_id = (SELECT id FROM project WHERE name = '【故障】政务大楼B座接入层交换机成批掉线告警') AND name = '现场回单阶段'),
        '【回单】提交PoE供电模块备件更换报告',
        '核对替换设备的序列号，上传测试日志，申请工单归档。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        1, -- 未开始
        0,
        3,
        '2026-06-11',
        '2026-06-15',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-06-01 10:00:00+08:00',
        '2026-06-20 10:00:00+08:00'
    );

-- 6. 插入操作留痕日志 (原 sub_task_progress)
INSERT INTO sub_task_progress (
    sub_task_id,
    operator_user_id,
    progress_note,
    progress_percent,
    status,
    created_at
) VALUES
    -- 工单 1 抢修历史
    (
        (SELECT id FROM sub_task WHERE name = '【排障】前往光交箱用 OTDR 测距故障点'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        '携带 OTDR 仪器已抵达现场，开始进行单端红光测试与测距。',
        50,
        2,
        '2026-06-12 10:00:00+08:00'
    ),
    (
        (SELECT id FROM sub_task WHERE name = '【排障】前往光交箱用 OTDR 测距故障点'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        '测距波形显示在 4.2 公里处有严重反射断点，已确定是环城南路地铁施工挖断，排障完毕。',
        100,
        3,
        '2026-06-14 15:30:00+08:00'
    ),
    (
        (SELECT id FROM sub_task WHERE name = '【抢修】阻水光缆熔接与跳线重新测试'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        '由于地铁工地施工空间狭窄，已调集多纤熔接备件。临时拉设了 50 米光纤，熔接工作完成 12 芯。',
        50,
        2,
        '2026-06-18 16:00:00+08:00'
    ),

    -- 工单 4 抢修历史 (超时告警中)
    (
        (SELECT id FROM sub_task WHERE name = '【执行】到现场测试各楼层机房供电与配电柜'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        '排查了 12-13 层的机房，发现配电箱正常，正等待大楼物业电工协助排查总闸。',
        30,
        2,
        '2026-06-08 14:00:00+08:00'
    );

-- 7. 插入运维资产设备 (原 device)
INSERT INTO device (name, description, created_at, updated_at) VALUES
('高精度光时域反射仪 (OTDR)', '用于精密检测光纤断点及光衰情况。', '2026-03-01 09:00:00+08:00', '2026-03-01 09:00:00+08:00'),
('全自动光纤熔接机 (带切刀)', '用于通信抢修中两端光缆的对准与热熔接。', '2026-03-01 09:00:00+08:00', '2026-03-01 09:00:00+08:00'),
('数字万用表 / 测线仪', '用于基础弱电供电测量及网线水晶头连通测试。', '2026-03-01 09:00:00+08:00', '2026-03-01 09:00:00+08:00');

-- 8. 插入资产领用流水日志 (原 device_usage_log)
INSERT INTO device_usage_log (device_id, user_id, purpose, created_at) VALUES
(
    (SELECT id FROM device WHERE name = '高精度光时域反射仪 (OTDR)'),
    (SELECT id FROM sys_user WHERE username = 'chen_xu'),
    '领用用于省级专网光纤中断故障点测距与抢修。',
    '2026-06-11 09:15:00+08:00'
),
(
    (SELECT id FROM device WHERE name = '全自动光纤熔接机 (带切刀)'),
    (SELECT id FROM sys_user WHERE username = 'chen_xu'),
    '配合 OTDR 领用，执行环城南路断点熔接施工。',
    '2026-06-16 08:30:00+08:00'
),
(
    (SELECT id FROM device WHERE name = '数字万用表 / 测线仪'),
    (SELECT id FROM sys_user WHERE username = 'chen_xu'),
    '排查政务大楼B座网络交换机掉线是否为供电跳闸。',
    '2026-06-06 09:45:00+08:00'
);

COMMIT;
