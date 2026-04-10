-- 项目进度跟踪系统生产用户初始化种子
--
-- 说明：
-- 1. 本脚本假定已先执行 db/migrations/001_init_schema.sql
-- 2. 本脚本只初始化 sys_user，不插入项目、节点、任务等业务数据
-- 3. 所有用户默认初始密码均为 123456
-- 4. 如果 username 已存在，则跳过该账号，避免覆盖线上已有用户

BEGIN;

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
        'zhou_yang',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '周阳',
        3,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    ),
    (
        'huang_yonghe',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '黄永和',
        3,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    ),
    (
        'zou_yihui',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '邹亿辉',
        3,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    ),
    (
        'teng_hongjun',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '滕洪军',
        3,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    ),
    (
        'huang_hai',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '黄海',
        3,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    ),
    (
        'yang_wei',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '杨伟',
        3,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    ),
    (
        'huang_zhongxi',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '黄忠喜',
        3,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    ),
    (
        'li_zan',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '李赞',
        1,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    ),
    (
        'hong_bingyun',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '洪冰赟',
        3,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    ),
    (
        'li_zhuoyang',
        '$argon2id$v=19$m=65536,t=2,p=1$cHJvamVjdC10cmFja2VyLQ$199JoMMgsdke/WTA+8aYWQtdjzisn4t3x5s09F2tYck',
        '李卓阳',
        3,
        '',
        '',
        1,
        '2026-04-10 09:00:00+08:00',
        '2026-04-10 09:00:00+08:00'
    )
ON CONFLICT (username) DO NOTHING;

COMMIT;
