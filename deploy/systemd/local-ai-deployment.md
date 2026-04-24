# Local AI Deployment

本文档说明如何在 Linux 虚拟机或服务器上，为当前项目部署“本地 AI 项目草稿生成”能力。

本文档基于仓库当前代码的真实行为整理，重点是“能直接跑起来”：

- 后端主服务仍然只使用一个 `systemd` 服务：`project-tracker.service`
- 本地 AI 不是单独的 HTTP 服务
- 后端首次收到 AI 请求时，才会拉起一个常驻 Python worker
- Python worker 启动后只加载一次本地模型，后续请求复用同一个 worker

当前约定：

- 部署用户：`ubzy`
- 部署根目录：`/opt/project-tracker`
- 后端二进制路径：`/opt/project-tracker/bin/Project_Tracker`
- 生产配置路径：`/opt/project-tracker/config/config.prod.json`
- AI 工具目录：`/opt/project-tracker/tools/ai_project_manager`
- `systemd` 服务文件样例：`deploy/systemd/project-tracker.service`

## 1. 当前部署方式

推荐使用“同机部署”：

- `Project_Tracker` 主服务、Python 运行环境、模型目录放在同一台机器
- 只让 `systemd` 托管 `project-tracker.service`
- 不需要额外再写一个 `local-ai.service` 或 `generate_worker.service`

原因很直接：

- 当前后端会在第一次 AI 请求时自动 `fork + exec` 拉起 Python worker
- worker 是单例常驻进程，不是每次请求都重新启动
- 如果你再人为拆成第二个服务，运维复杂度会上升，但当前代码并没有直接受益

需要注意：

- 首次 AI 请求会明显更慢，因为这一步会启动 worker 并加载模型
- 后续请求通常会快很多，因为模型已经常驻内存或显存
- 如果你启动多个后端实例，每个实例都会各自拉起一份 worker，也就会各自占一份模型内存或显存

## 2. 前置条件

建议先满足下面这些前提：

- Linux 系统，使用 `systemd`
- 已按 [systemd-deployment.md](/home/ubzy/CLionProjects/Project-Tracker/deploy/systemd/systemd-deployment.md) 部署好后端主服务
- 机器上可用 Python 3.11
- 磁盘空间足够存放模型目录
- 内存足够容纳后端进程和模型

如果你准备使用 GPU：

- 宿主机驱动、CUDA、PyTorch 版本必须匹配
- `project-tracker.service` 运行用户必须能访问 GPU
- 如果虚拟机没有正确透传 GPU，先用 CPU 跑通全链路，再考虑 GPU 加速

## 3. 需要部署哪些文件

后端二进制本身不够。

要让本地 AI 真正可用，至少还要有下面这些内容：

- `tools/ai_project_manager/scripts`
- `tools/ai_project_manager/src`
- `tools/ai_project_manager/models`
- Python 环境里的运行依赖

最省事的做法是直接把整个 `tools/ai_project_manager` 目录部署过去。

先创建目录：

```bash
sudo mkdir -p /opt/project-tracker/tools
```

再把仓库里的 AI 工具目录整体复制到部署目录：

```bash
cp -a tools/ai_project_manager /opt/project-tracker/tools/
```

修正目录所有者：

```bash
sudo chown -R ubzy:ubzy /opt/project-tracker/tools/ai_project_manager
```

如果你本来就是整仓库部署到 `/opt/project-tracker`，这一步可以跳过，只要确认 `tools/ai_project_manager` 已经在部署目录里即可。

## 4. 准备 Python 环境

当前仓库里的 AI 工具说明文档使用 `conda`，这里也直接沿用这个方案，减少额外变量。

在部署机上执行：

```bash
conda create -y -p /opt/project-tracker/tools/ai_project_manager/.conda python=3.11
conda run -p /opt/project-tracker/tools/ai_project_manager/.conda python -m pip install -U pip setuptools wheel
conda run -p /opt/project-tracker/tools/ai_project_manager/.conda python -m pip install -e '/opt/project-tracker/tools/ai_project_manager[train]'
```

说明：

- 运行 worker 需要基础依赖，也需要 `transformers`、`torch`、`sentencepiece`、`accelerate`
- 这些推理相关包当前在 `train` extra 里，因此部署时直接安装 `...[train]` 最省事
- 如果你只是做虚拟机实验，不追求极致性能，先能跑起来最重要

如果你的机器使用 GPU，`torch` 需要换成和当前 CUDA 匹配的版本。  
这一步不一定非要在这里改，但你至少要保证最终环境里的 `torch` 能真正识别 GPU。

## 5. 准备模型目录

当前 worker 会直接调用 Hugging Face 的本地加载方式：

- `AutoTokenizer.from_pretrained(model_dir)`
- `AutoModelForSeq2SeqLM.from_pretrained(model_dir)`

所以 `model_dir` 必须是一个完整的本地模型目录，而不是一个空目录，也不是只放一两个权重文件。

推荐放到：

```text
/opt/project-tracker/tools/ai_project_manager/models/mt5-small-offline
```

如果你的模型已经在其他机器上训练好，可以直接复制整个目录过来，例如：

```bash
sudo mkdir -p /opt/project-tracker/tools/ai_project_manager/models
cp -a /path/to/mt5-small-offline /opt/project-tracker/tools/ai_project_manager/models/
sudo chown -R ubzy:ubzy /opt/project-tracker/tools/ai_project_manager/models
```

一个可用的模型目录里，通常至少应该能看到这类文件中的大部分：

- `config.json`
- `generation_config.json`
- `tokenizer_config.json`
- `spiece.model`
- `model.safetensors` 或 `pytorch_model.bin`

## 6. 修改生产配置

编辑：

```text
/opt/project-tracker/config/config.prod.json
```

建议直接使用绝对路径，不要在生产配置里继续依赖相对路径。

可参考下面这个配置片段：

```json
{
  "custom_config": {
    "threads_num": 0,
    "local_ai": {
      "python_bin": "/opt/project-tracker/tools/ai_project_manager/.conda/bin/python",
      "script_path": "/opt/project-tracker/tools/ai_project_manager/scripts/generate_once.py",
      "model_dir": "/opt/project-tracker/tools/ai_project_manager/models/mt5-small-offline"
    }
  }
}
```

这三个字段的要求是：

- `python_bin` 必须是存在的 Python 可执行文件
- `script_path` 必须是存在的文件
- `model_dir` 必须是存在的目录

有一个很关键的实现细节：

- 当前后端虽然会校验 `script_path`
- 但它真正拉起的是 `script_path` 同目录下的 `generate_worker.py`

因此你要确保：

- `script_path` 指向一个真实存在的文件，通常就填 `generate_once.py`
- 同一个 `scripts` 目录下同时存在 `generate_worker.py`

不要只复制 `generate_once.py`，也不要只复制 `generate_worker.py`。

## 7. 重启后端服务

配置和文件都就位以后，重启主服务：

```bash
sudo systemctl restart project-tracker.service
```

确认状态：

```bash
sudo systemctl status project-tracker.service
```

持续看日志：

```bash
sudo journalctl -u project-tracker.service -f
```

如果你修改过 `project-tracker.service`，不要忘记先执行：

```bash
sudo systemctl daemon-reload
sudo systemctl restart project-tracker.service
```

## 8. 建议的验收顺序

建议按下面的顺序验证，不要一上来就只看前端页面。

### 8.1 先验证模型本身能跑

先准备一个输入文件：

```bash
cat >/tmp/project-ai-request.json <<'EOF'
{
  "prompt": "做一个官网改版项目，包含需求梳理、设计、开发、测试上线四个阶段"
}
EOF
```

再直接调用一次脚本：

```bash
conda run -p /opt/project-tracker/tools/ai_project_manager/.conda \
  python /opt/project-tracker/tools/ai_project_manager/scripts/generate_once.py \
  --model-dir /opt/project-tracker/tools/ai_project_manager/models/mt5-small-offline \
  --input-file /tmp/project-ai-request.json
```

如果这一步都失败，先不要怀疑前端和后端，问题通常就在 Python 环境、模型目录或者推理依赖。

### 8.2 再通过后端接口验证

接口路径是：

```text
POST /api/ai/project-draft/generate
```

这个接口要求登录态，直接裸调会返回 `401`。  
最简单的验收方法是：

- 先登录前端
- 从“AI 创建项目”的页面发起生成
- 同时观察 `journalctl -u project-tracker.service -f`

如果你确实要用 `curl` 验证，也要带上你自己的已登录 Cookie，例如：

```bash
curl -X POST 'http://127.0.0.1:8080/api/ai/project-draft/generate' \
  -H 'Content-Type: application/json' \
  -H 'Cookie: 你的登录态Cookie' \
  -d '{"prompt":"做一个官网改版项目，包含需求梳理、设计、开发、测试上线四个阶段"}'
```

## 9. 正常现象

下面这些情况通常是正常的，不必先入为主判断为故障：

- 第一次 AI 生成明显更慢
- 后续第二次、第三次生成明显更快
- 启用 GPU 后日志里出现类似 `Device set to use cuda:0` 的提示

如果是第一次请求耗时 10 秒左右甚至更长，在本地模型加载阶段并不奇怪。  
真正需要关注的是：后续请求是否仍然每次都同样慢，或者是否频繁重新拉起 worker。

## 10. 常见故障排查

- `local_ai.python_bin 指向的路径不存在`
  说明配置里的 Python 路径不对，或者 `.conda` 环境根本没创建出来。

- `local_ai.script_path 指向的路径不存在`
  说明脚本没有随部署一起复制，或者配置里写成了相对路径但当前工作目录与你预期不一致。

- `local_ai.model_dir 指向的路径不存在`
  说明模型目录还没放好，或者目录名写错。

- `local_ai.script_path 所在目录缺少 generate_worker.py`
  说明你只复制了单个脚本，没有把整个 `scripts` 目录一起部署。

- Python 直接报 `ModuleNotFoundError`
  说明 `tools/ai_project_manager/src` 不完整，或者 editable install 没成功。

- 首次请求成功，后续请求偶发失败
  先看 `journalctl`，确认 worker 是否异常退出。如果 worker 退出，后端下一次请求会重新拉起它。

- GPU 明明存在，但推理没有走 GPU
  大概率是驱动、CUDA、`torch` 三者版本不匹配，或者当前 `systemd` 运行用户没有 GPU 访问权限。

- 前端只看到“生成失败”，但不知道为什么
  先不要猜，直接执行：
  ```bash
  sudo systemctl status project-tracker.service
  sudo journalctl -u project-tracker.service -n 100
  ```

## 11. 一个最小可行部署结论

如果你只是想先在虚拟机里把本地 AI 跑通，最小可行方案就是：

1. 已部署 `Project_Tracker` 主服务
2. 把 `tools/ai_project_manager` 整个目录复制到 `/opt/project-tracker/tools/`
3. 用 `conda` 在该目录下创建 Python 3.11 环境
4. 安装 `'/opt/project-tracker/tools/ai_project_manager[train]'`
5. 把完整模型目录放到 `/opt/project-tracker/tools/ai_project_manager/models/mt5-small-offline`
6. 在 `config.prod.json` 里填好三个绝对路径
7. 重启 `project-tracker.service`
8. 通过前端的 AI 创建项目入口验证

按这个路径走，变量最少，最适合先做一次虚拟机实验。
