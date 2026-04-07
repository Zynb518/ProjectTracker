-- 项目进度跟踪系统演示数据
--
-- 说明：
-- 1. 本脚本假定已先执行 db/migrations/001_init_schema.sql
-- 2. 所有用户默认密码均为 123456
-- 3. 本脚本会先清空数据，再插入一组可用于联调和页面展示的假数据

BEGIN;

TRUNCATE TABLE
    project_template_node,
    sub_task_progress,
    sub_task,
    project_node,
    project_member,
    project_template,
    project,
    sys_user
RESTART IDENTITY CASCADE;

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
        2,
        '2026-03-05 11:00:00+08:00',
        '2026-03-15 12:00:00+08:00'
    );

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
        '官网重构',
        '对企业官网进行信息架构与视觉重构。',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        2,
        '2026-03-01',
        '2026-04-30',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-03-01 09:00:00+08:00',
        '2026-03-20 18:00:00+08:00'
    ),
    (
        '移动端打卡',
        '补充员工移动端打卡与异常申诉能力。',
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        1,
        '2026-04-10',
        '2026-05-20',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-03-05 09:00:00+08:00',
        '2026-03-18 16:30:00+08:00'
    ),
    (
        '财务报表平台',
        '统一财务报表指标口径并沉淀查询平台。',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        3,
        '2026-01-05',
        '2026-02-28',
        '2026-02-26 17:30:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-01-05 09:30:00+08:00',
        '2026-02-26 17:30:00+08:00'
    ),
    (
        '仓储扫码升级',
        '升级仓储扫码枪设备与扫码流程。',
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        4,
        '2026-02-01',
        '2026-03-15',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-02-01 10:00:00+08:00',
        '2026-03-18 10:00:00+08:00'
    ),
    (
        '人事流程优化',
        '梳理入转调离审批流程并推动线上化。',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        2,
        '2026-03-12',
        '2026-04-18',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-03-12 09:15:00+08:00',
        '2026-03-24 15:40:00+08:00'
    );

INSERT INTO project_member (
    project_id,
    user_id,
    added_by,
    joined_at
) VALUES
    (
        (SELECT id FROM project WHERE name = '官网重构'),
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-03-01 09:05:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '官网重构'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-03-02 10:00:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '移动端打卡'),
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-03-05 09:10:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '财务报表平台'),
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-01-05 09:40:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '仓储扫码升级'),
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        (SELECT id FROM sys_user WHERE username = 'admin'),
        '2026-02-01 10:10:00+08:00'
    );

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
    (
        (SELECT id FROM project WHERE name = '官网重构'),
        '需求分析',
        '梳理官网改版目标与页面信息架构。',
        1,
        3,
        '2026-03-01',
        '2026-03-07',
        '2026-03-06 18:00:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-03-01 09:20:00+08:00',
        '2026-03-06 18:00:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '移动端打卡'),
        '原型设计',
        '产出移动端打卡基础交互原型。',
        1,
        1,
        '2026-04-10',
        '2026-04-18',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-03-06 10:00:00+08:00',
        '2026-03-18 16:45:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '财务报表平台'),
        '数据建模',
        '完成财务报表主题域建模与字段确认。',
        1,
        3,
        '2026-01-08',
        '2026-01-31',
        '2026-01-29 17:00:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-01-08 09:30:00+08:00',
        '2026-01-29 17:00:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '仓储扫码升级'),
        '设备调研',
        '对扫码设备与接口协议进行调研。',
        1,
        4,
        '2026-02-01',
        '2026-02-20',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-02-01 10:30:00+08:00',
        '2026-03-18 10:30:00+08:00'
    ),
    (
        (SELECT id FROM project WHERE name = '人事流程优化'),
        '流程梳理',
        '梳理现有人事审批流程与痛点。',
        1,
        2,
        '2026-03-12',
        '2026-03-25',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-03-12 09:30:00+08:00',
        '2026-03-24 15:50:00+08:00'
    );

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
    (
        (
            SELECT pn.id
            FROM project_node pn
            JOIN project p ON p.id = pn.project_id
            WHERE p.name = '官网重构' AND pn.name = '需求分析'
        ),
        '页面清单整理',
        '输出官网现有页面清单与改版建议。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        3,
        100,
        2,
        '2026-03-01',
        '2026-03-04',
        '2026-03-03 17:30:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-03-01 09:40:00+08:00',
        '2026-03-03 17:30:00+08:00'
    ),
    (
        (
            SELECT pn.id
            FROM project_node pn
            JOIN project p ON p.id = pn.project_id
            WHERE p.name = '移动端打卡' AND pn.name = '原型设计'
        ),
        '打卡首页原型',
        '绘制移动端打卡首页与异常申诉入口原型。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        1,
        0,
        3,
        '2026-04-10',
        '2026-04-14',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-03-06 10:20:00+08:00',
        '2026-03-18 16:50:00+08:00'
    ),
    (
        (
            SELECT pn.id
            FROM project_node pn
            JOIN project p ON p.id = pn.project_id
            WHERE p.name = '财务报表平台' AND pn.name = '数据建模'
        ),
        '口径确认',
        '确认收入、成本、利润三大指标口径。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        3,
        100,
        3,
        '2026-01-10',
        '2026-01-20',
        '2026-01-18 16:00:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-01-10 10:00:00+08:00',
        '2026-01-18 16:00:00+08:00'
    ),
    (
        (
            SELECT pn.id
            FROM project_node pn
            JOIN project p ON p.id = pn.project_id
            WHERE p.name = '仓储扫码升级' AND pn.name = '设备调研'
        ),
        '扫码设备比选',
        '对两家设备供应商进行成本与稳定性对比。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        4,
        70,
        2,
        '2026-02-05',
        '2026-02-25',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'wang_jie'),
        '2026-02-05 10:40:00+08:00',
        '2026-03-18 10:45:00+08:00'
    ),
    (
        (
            SELECT pn.id
            FROM project_node pn
            JOIN project p ON p.id = pn.project_id
            WHERE p.name = '人事流程优化' AND pn.name = '流程梳理'
        ),
        '部门访谈',
        '访谈各部门审批参与人并整理诉求。',
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        2,
        40,
        2,
        '2026-03-12',
        '2026-03-22',
        NULL,
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-03-12 10:00:00+08:00',
        '2026-03-24 16:00:00+08:00'
    ),
    (
        (
            SELECT pn.id
            FROM project_node pn
            JOIN project p ON p.id = pn.project_id
            WHERE p.name = '人事流程优化' AND pn.name = '流程梳理'
        ),
        '审批节点回归确认',
        '用于验证撤销子任务完成时可恢复到最近一条未完成进度。',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        3,
        100,
        2,
        '2026-03-13',
        '2026-04-20',
        '2026-03-26 18:20:00+08:00',
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '2026-03-13 10:20:00+08:00',
        '2026-03-26 18:20:00+08:00'
    );

INSERT INTO sub_task_progress (
    sub_task_id,
    operator_user_id,
    progress_note,
    progress_percent,
    status,
    created_at
) VALUES
    (
        (SELECT id FROM sub_task WHERE name = '页面清单整理'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        '页面清单与改版建议已整理完成。',
        100,
        3,
        '2026-03-03 17:30:00+08:00'
    ),
    (
        (SELECT id FROM sub_task WHERE name = '打卡首页原型'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        '任务已创建，待进入原型绘制阶段。',
        0,
        1,
        '2026-03-18 16:50:00+08:00'
    ),
    (
        (SELECT id FROM sub_task WHERE name = '口径确认'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        '三大财务指标口径已确认。',
        100,
        3,
        '2026-01-18 16:00:00+08:00'
    ),
    (
        (SELECT id FROM sub_task WHERE name = '扫码设备比选'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        '已完成两家供应商比选与初步结论。',
        70,
        4,
        '2026-03-18 10:45:00+08:00'
    ),
    (
        (SELECT id FROM sub_task WHERE name = '部门访谈'),
        (SELECT id FROM sys_user WHERE username = 'chen_xu'),
        '已完成部分部门访谈，继续跟进审批节点。',
        40,
        2,
        '2026-03-24 16:00:00+08:00'
    ),
    (
        (SELECT id FROM sub_task WHERE name = '审批节点回归确认'),
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '已完成审批节点回归确认的大部分核对工作。',
        60,
        2,
        '2026-03-25 15:30:00+08:00'
    ),
    (
        (SELECT id FROM sub_task WHERE name = '审批节点回归确认'),
        (SELECT id FROM sys_user WHERE username = 'li_ming'),
        '审批节点回归确认已全部完成。',
        100,
        3,
        '2026-03-26 18:20:00+08:00'
    );

INSERT INTO project_template (
    name,
    description,
    status
) VALUES
    ('标准研发模板', '适用于常规软件研发项目。', 1),
    ('数据治理模板', '适用于数据治理与报表类项目。', 1),
    ('流程优化模板', '适用于内部流程优化类项目。', 1),
    ('设备升级模板', '适用于硬件设备升级与替换。', 1),
    ('历史停用模板', '历史模板，仅保留展示。', 2);

INSERT INTO project_template_node (
    template_id,
    name,
    description,
    sequence_no
) VALUES
    (
        (SELECT id FROM project_template WHERE name = '标准研发模板'),
        '需求确认',
        '完成需求分析与范围确认。',
        1
    ),
    (
        (SELECT id FROM project_template WHERE name = '数据治理模板'),
        '数据梳理',
        '完成数据源与指标口径梳理。',
        1
    ),
    (
        (SELECT id FROM project_template WHERE name = '流程优化模板'),
        '现状调研',
        '完成流程现状调研与问题收集。',
        1
    ),
    (
        (SELECT id FROM project_template WHERE name = '设备升级模板'),
        '设备调研',
        '完成设备选型与供应商对比。',
        1
    ),
    (
        (SELECT id FROM project_template WHERE name = '历史停用模板'),
        '历史阶段',
        '历史模板中的保留阶段。',
        1
    );

COMMIT;
