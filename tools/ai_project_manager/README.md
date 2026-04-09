# AI Project Manager（Offline）

本目录提供一个与当前 Drogon 后端解耦的本地离线工具，用来把中文需求文本生成成后续可接入项目创建接口的 JSON 草案。

当前阶段目标只有两件事：

1. 先产出可消费的结构化数据。
2. 跑通你自己的数据构建、模型训练、本地推理链路。

## 当前能力

- `heuristic` 生成器已经可用，能稳定输出 `project + nodes + subtasks` 的完整 JSON。
- `model` 生成器已经打通本地训练和推理，训练目标会先学习 `PROJECT/NODE/TASK/END` 中间结构，再恢复为最终 JSON。
- `model` 分支现在会执行“模型生成 -> 容错解析 -> 归一化/校验 -> 低质量回退修复”，因此最终输出可以稳定落到当前 API 所需结构。
- 种子样本已经覆盖短流程项目和长流程项目，同时包含通用软件项目与硬件/产品交付类项目。
- 当前种子样本已经扩展为一组分层数据集，同时覆盖短流程项目和长流程项目，适合继续迭代本地模型。

## 输出契约

最终 JSON 只保留当前 API 真正需要的字段：

```json
{
  "project": {
    "name": "内部进度平台",
    "description": "用于内部项目进度跟踪",
    "planned_start_date": "2026-03-20",
    "planned_end_date": "2026-06-30"
  },
  "nodes": [
    {
      "name": "开发实现",
      "description": "完成核心功能开发",
      "planned_start_date": "2026-03-26",
      "planned_end_date": "2026-04-20",
      "subtasks": [
        {
          "name": "完成登录接口开发",
          "description": "完成账号密码登录功能",
          "priority": 2,
          "planned_start_date": "2026-03-27",
          "planned_end_date": "2026-03-31"
        }
      ]
    }
  ]
}
```

约束如下：

- `project` 只包含 `name`、`description`、`planned_start_date`、`planned_end_date`
- `node` 只包含 `name`、`description`、`planned_start_date`、`planned_end_date`、`subtasks`
- `subtask` 只包含 `name`、`description`、`responsible_user_id`、`priority`、`planned_start_date`、`planned_end_date`
- `responsible_user_id` 允许缺省
- `priority` 只允许 `1 / 2 / 3`
- 日期格式固定为 `YYYY-MM-DD`
- 项目日期必须覆盖全部阶段，阶段日期必须覆盖其下全部子任务

## 环境准备

在仓库根目录执行：

```bash
conda create -y -p tools/ai_project_manager/.conda python=3.11
conda run -p tools/ai_project_manager/.conda python -m pip install -U pip setuptools wheel
conda run -p tools/ai_project_manager/.conda python -m pip install -e 'tools/ai_project_manager[dev]'
```

如果你要训练本地模型，再额外安装训练依赖：

```bash
conda run -p tools/ai_project_manager/.conda python -m pip install -e 'tools/ai_project_manager[train]'
```

## 直接生成项目计划

### 启发式生成器

这是当前最实用的入口，适合直接拿来产出完整项目计划：

```bash
conda run -p tools/ai_project_manager/.conda ai-project-manager generate \
  --generator heuristic \
  --requirement "做一个公司官网改版项目，包含设计、开发、测试上线三个阶段"
```

### 本地模型生成器

先完成训练后，再执行：

```bash
conda run -p tools/ai_project_manager/.conda ai-project-manager generate \
  --generator model \
  --model-dir tools/ai_project_manager/models/mt5-small-offline \
  --requirement "做一个公司官网改版项目，包含设计、开发、测试上线三个阶段"
```

当前 `model` 分支的执行策略是：

- 先让本地 `mt5-small` 输出中间计划文本
- 解析为 `project + nodes + subtasks`
- 做日期归一化和 schema 校验
- 如果模型结果明显偏低质，则自动回退到基于需求显式阶段的本地修复

因此它现在的定位不是“纯模型端到端演示”，而是“优先使用本地模型、保证最终结果可用”的离线生成链路。

## 数据集构建

原始种子样本位于：

- `tools/ai_project_manager/data/raw/seed_samples.jsonl`

当前这份种子集采用“短流程 / 中流程 / 长流程”混合策略：

- 短流程样本用于训练 2 到 3 个阶段的简单项目
- 中流程样本用于训练 4 到 5 个阶段的常规项目
- 长流程样本用于训练 6 到 7 个阶段的复杂项目

长流程结构借用了参考资料中的阶段推进节奏，但已经抽象成通用项目管理语义，不依赖医疗或硬件专有术语。

构建训练集：

```bash
conda run -p tools/ai_project_manager/.conda python tools/ai_project_manager/scripts/build_dataset.py
```

生成结果会写入：

- `tools/ai_project_manager/data/processed/train.jsonl`

这个脚本会自动把 `tools/ai_project_manager/src` 加到 `sys.path`，因此即使没有先做 editable install，也可以从仓库根目录直接运行。

## 本地训练

训练脚本会读取 `data/processed/train.jsonl`，默认基于 `google/mt5-small` 训练，并把产物写到：

- `tools/ai_project_manager/models/mt5-small-offline`

训练目标不是直接 JSON，而是更短、更稳定的中间文本格式：

```text
PROJECT|项目名|项目描述|开始日期|结束日期
NODE|阶段名|阶段描述|开始日期|结束日期
TASK|任务名|任务描述|优先级|开始日期|结束日期
END
```

执行命令：

```bash
conda run -p tools/ai_project_manager/.conda python tools/ai_project_manager/scripts/train_model.py
```

## 推理脚本

除了 CLI，也可以直接运行推理脚本：

```bash
conda run -p tools/ai_project_manager/.conda python tools/ai_project_manager/scripts/run_inference.py \
  --model-dir tools/ai_project_manager/models/mt5-small-offline \
  --requirement "做一个公司官网改版项目，包含设计、开发、测试上线三个阶段"
```

推理脚本会自动补上训练时使用的 instruction prefix，所以 `--requirement` 只需要传原始中文需求。

## 验证命令

建议至少执行下面这几条：

```bash
conda run -p tools/ai_project_manager/.conda pytest tools/ai_project_manager/tests -q
conda run -p tools/ai_project_manager/.conda python tools/ai_project_manager/scripts/build_dataset.py
conda run -p tools/ai_project_manager/.conda ai-project-manager generate --generator heuristic --requirement "做一个公司官网改版项目，包含设计、开发、测试上线三个阶段"
conda run -p tools/ai_project_manager/.conda ai-project-manager generate --generator model --model-dir tools/ai_project_manager/models/mt5-small-offline --requirement "做一个公司官网改版项目，包含设计、开发、测试上线三个阶段"
```

## 当前限制

- 当前种子样本仍然是手工整理的小规模数据集，泛化能力仍然有限。
- 原始 `mt5-small` 输出仍可能出现行挤压、字段错位或描述重复，当前可用性依赖后处理修复和质量门槛。
- `model` 分支现在已经能稳定返回合法 JSON，但还不是“纯模型直接输出高质量计划”的最终形态。
- 如果你接下来想提升模型原生质量，优先继续扩充中长流程样本，而不是先更换推理接口。
