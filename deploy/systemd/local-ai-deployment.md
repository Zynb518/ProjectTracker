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

### 3.1 如果你是手动上传文件

如果你是通过 `WinSCP`、`SFTP` 或共享目录手动传文件，不一定要把整个 `tools/ai_project_manager` 都传过去。

实测更稳妥的“最小上传集”是：

- `tools/ai_project_manager/pyproject.toml`
- `tools/ai_project_manager/src`
- `tools/ai_project_manager/scripts`
- `tools/ai_project_manager/models/mt5-small-offline`

目标目录要保持为：

```text
/opt/project-tracker/tools/ai_project_manager
```

目录名必须是 `ai_project_manager`，不要传成 `ai_project_manager (副本)` 这类名字，否则 `config.prod.json` 里的绝对路径会直接失效。

手动上传时，下面这些内容通常不需要传：

- `tools/ai_project_manager/.conda`
- `tools/ai_project_manager/.pytest_cache`
- `tools/ai_project_manager/tests`
- `tools/ai_project_manager/data`
- `tools/ai_project_manager/models/mt5-small-offline/checkpoints`

尤其是 `.conda`。  
这类本地 Python 环境里会包含 `torch`、CUDA、cuDNN、`triton` 等大库，体积会膨胀到几个 GB，但在目标机器上本来就应该重新创建环境，不适合手动拷贝。

## 4. 准备 Python 环境

当前仓库里的 AI 工具说明文档使用 `conda`，这里也直接沿用这个方案，减少额外变量。

### 4.1 如果虚拟机还没装 conda

如果目标机器还没有 `conda`，建议直接安装 `Miniconda`。

如果机器里没有 `zsh`，完全没关系，直接用 `bash` 即可。

示例：

```bash
mkdir -p ~/miniconda3
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O /tmp/miniconda.sh
bash /tmp/miniconda.sh -b -u -p ~/miniconda3
~/miniconda3/bin/conda init bash
source ~/.bashrc
```

首次使用 `repo.anaconda.com` 默认 channel 时，可能会被 `Terms of Service` 卡住。  
如果你看到 `CondaToSNonInteractiveError`，先执行：

```bash
~/miniconda3/bin/conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/main
~/miniconda3/bin/conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/r
```

然后再继续创建环境。

在部署机上执行：

```bash
~/miniconda3/bin/conda create -y -p /opt/project-tracker/tools/ai_project_manager/.conda python=3.11
~/miniconda3/bin/conda run -p /opt/project-tracker/tools/ai_project_manager/.conda python -m pip install -U pip setuptools wheel
~/miniconda3/bin/conda run -p /opt/project-tracker/tools/ai_project_manager/.conda python -m pip install -e '/opt/project-tracker/tools/ai_project_manager[train]'
```

说明：

- 运行 worker 需要基础依赖，也需要 `transformers`、`torch`、`sentencepiece`、`accelerate`
- 这些推理相关包当前在 `train` extra 里，因此部署时直接安装 `...[train]` 最省事
- 如果你只是做虚拟机实验，不追求极致性能，先能跑起来最重要

如果安装依赖时看起来“卡住不动”，先不要急着判断为失败。  
`torch`、`transformers` 这类包第一次安装会比较慢，而且 `conda run` 默认会缓冲输出。

可以改成下面这种方式，实时查看安装进度：

```bash
~/miniconda3/bin/conda run --no-capture-output -p /opt/project-tracker/tools/ai_project_manager/.conda \
  python -m pip install -v -e "/opt/project-tracker/tools/ai_project_manager[train]"
```

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

如果前端或后端报：

- `local_ai.model_dir 指向的路径不存在`

优先怀疑的不是配置写错，而是模型目录根本没有随部署一起传过去。  
这类问题在“代码传过去了，但模型目录没传过去”时非常常见。

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
~/miniconda3/bin/conda run -p /opt/project-tracker/tools/ai_project_manager/.conda \
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

### 8.3 如果你怀疑是代理超时

如果前端提示：

- `Request failed with status code 504`

不要先怀疑前端页面。  
这个错误在当前部署里更常见的含义是：

- `nginx` 已经把请求转发到了后端
- 但后端第一次拉起 worker 并加载本地模型太慢
- `nginx` 在等待上游响应时先超时了

这种情况在纯 CPU 虚拟机上尤其常见。

可以先直接在机器本地测一次模型脚本耗时：

```bash
cat >/tmp/project-ai-request.json <<'EOF'
{
  "prompt": "做一个官网改版项目，包含需求梳理、设计、开发、测试上线四个阶段"
}
EOF

time /opt/project-tracker/tools/ai_project_manager/.conda/bin/python \
  /opt/project-tracker/tools/ai_project_manager/scripts/generate_once.py \
  --model-dir /opt/project-tracker/tools/ai_project_manager/models/mt5-small-offline \
  --input-file /tmp/project-ai-request.json
```

这里重点看 `real` 时间，也就是你真实等了多久。

如果 `real` 已经接近或超过 60 秒，而前端请求又是经过 `nginx` 转发，那么出现 `504` 很正常。  
这时应该同时检查：

```bash
sudo journalctl -u project-tracker.service -f -n 100
sudo tail -f /var/log/nginx/error.log
```

如果 `nginx` 错误日志里出现：

- `upstream timed out`

就说明根因已经确认，是代理超时而不是接口参数问题。

## 9. 正常现象

下面这些情况通常是正常的，不必先入为主判断为故障：

- 第一次 AI 生成明显更慢
- 后续第二次、第三次生成明显更快
- 启用 GPU 后日志里出现类似 `Device set to use cuda:0` 的提示

如果是第一次请求耗时 10 秒左右甚至更长，在本地模型加载阶段并不奇怪。  
真正需要关注的是：后续请求是否仍然每次都同样慢，或者是否频繁重新拉起 worker。

补充一个实操经验：

- `generate_once.py` 每次执行都会重新加载模型
- 而线上接口走的是常驻 `generate_worker.py`

所以：

- 手工执行 `generate_once.py` 的耗时，通常会比 worker 复用后的后续接口请求更慢
- 但它依然很适合用来测“第一次加载模型大概要多久”
- 在纯 CPU 虚拟机上，第一次加载和推理花到几分钟并不罕见

## 10. 常见故障排查

- `local_ai.python_bin 指向的路径不存在`
  说明配置里的 Python 路径不对，或者 `.conda` 环境根本没创建出来。

- `CondaToSNonInteractiveError`
  说明你第一次使用 `repo.anaconda.com` 的默认 channel，还没有接受 Anaconda 的 ToS。先执行文档上面的两条 `conda tos accept ...`，再重新创建环境。

- `EnvironmentLocationNotFound`
  说明你已经开始执行 `conda run -p ...`，但前面的 `conda create -p ...` 实际还没有成功，或者路径写错了。

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

- 前端提示 `Request failed with status code 504`
  很可能不是前端本身超时，而是 `nginx` 作为反向代理等待后端太久。
  处理：
  ```bash
  sudo journalctl -u project-tracker.service -n 100
  sudo tail -n 100 /var/log/nginx/error.log
  ```
  如果确认是 `upstream timed out`，请按 `nginx` 部署文档中的 AI 超时配置，调大 `/api/` 的代理超时。

## 11. 一个最小可行部署结论

如果你只是想先在虚拟机里把本地 AI 跑通，最小可行方案就是：

1. 已部署 `Project_Tracker` 主服务
2. 把 `pyproject.toml`、`src`、`scripts`、`models/mt5-small-offline` 上传到 `/opt/project-tracker/tools/ai_project_manager/`
3. 在虚拟机上安装 `Miniconda`
4. 接受默认 channel 的 ToS
5. 用 `conda` 在该目录下创建 Python 3.11 环境
6. 安装 `'/opt/project-tracker/tools/ai_project_manager[train]'`
7. 在 `config.prod.json` 里填好三个绝对路径
8. 如果前端入口走 `nginx`，确认 `/api/` 代理超时已经按 AI 场景放宽
9. 重启 `project-tracker.service`
10. 通过前端的 AI 创建项目入口验证

按这个路径走，变量最少，最适合先做一次虚拟机实验。
