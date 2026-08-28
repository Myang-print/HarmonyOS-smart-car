# OpenHarmony Linux 开发 + GitHub 记录同步模板

## 1. 目标

采用以下工作模式：

```text
Linux 实际工程
~/harmony/code/code-1.0/
    │
    │ 开发 / 编译 / 烧录
    │
    │ rsync
    ▼
Linux Git 记录仓库
~/harmony/code/projects/HarmonyOS-smart-car/
    │
    │ commit + push
    ▼
GitHub
    │
    │ pull
    ▼
Windows Git 仓库
D:\_GitHub\30_MyProjects\HarmonyOS-smart-car
```

核心原则：

* Linux 实际工程负责开发、编译、烧录。
* Git 仓库不参与 OpenHarmony 编译，仅用于保存源码快照和 Git 历史。
* 只同步实际手动修改的目录：
  `applications/sample/wifi-iot/app/`
* 不同步完整 `code-1.0`、`out/`、toolchain、kernel、third_party 等平台文件。
* Windows 端作为长期仓库副本，通过 GitHub 拉取最新记录。

---

# 2. 固定目录约定

## Linux 实际 OpenHarmony 工程

```bash
/home/harmony/harmony/code/code-1.0
```

主要修改目录：

```bash
/home/harmony/harmony/code/code-1.0/applications/sample/wifi-iot/app
```

例如：

```text
applications/sample/wifi-iot/app/
├── BUILD.gn
├── 7.0_I2c_Ssd1306/
├── ...
```

## Linux Git 记录仓库

```bash
/home/harmony/harmony/code/projects/HarmonyOS-smart-car
```

Git 中保存对应结构：

```text
HarmonyOS-smart-car/
└── src/
    └── harmony/
        └── applications/
            └── sample/
                └── wifi-iot/
                    └── app/
```

因此路径映射为：

```text
实际工程：
code-1.0/applications/sample/wifi-iot/app/

Git 记录：
src/harmony/applications/sample/wifi-iot/app/
```

## Windows Git 仓库

```text
D:\_GitHub\30_MyProjects\HarmonyOS-smart-car
```

---

# 3. 日常开发流程

## Step 1：在 Linux 实际工程中开发

进入实际工程：

```bash
cd ~/harmony/code/code-1.0
```

修改：

```text
applications/sample/wifi-iot/app/
```

正常执行：

* 编写代码
* 修改 `BUILD.gn`
* 编译
* 调试
* 烧录
* 验证运行结果

例如：

```bash
python build.py wifiiot
```

Git 记录仓库不参与这一阶段。

---

# 4. 将最新代码同步到 Git 记录仓库

开发和验证完成后，进入 Git 仓库：

```bash
cd ~/harmony/code/projects/HarmonyOS-smart-car
```

先同步 GitHub 最新 `main`：

```bash
git pull --ff-only origin main
```

然后执行：

```bash
rsync -av --delete \
  /home/harmony/harmony/code/code-1.0/applications/sample/wifi-iot/app/ \
  /home/harmony/harmony/code/projects/HarmonyOS-smart-car/src/harmony/applications/sample/wifi-iot/app/
```

参数含义：

```text
-a
archive mode
递归复制并尽量保留文件属性

-v
verbose
显示同步过程

--delete
删除目标目录中已经不存在于源目录的文件
使 Git 记录目录成为实际工程目录的镜像
```

注意源目录最后的 `/`：

```bash
.../app/
```

表示同步 `app` 内部内容，而不是额外创建一层 `app/app`。

---

# 5. 检查同步结果

执行：

```bash
git status
```

快速查看：

```bash
git status --short
```

例如：

```text
 M src/harmony/applications/sample/wifi-iot/app/BUILD.gn
 M src/harmony/applications/sample/wifi-iot/app/7.0_I2c_Ssd1306/I2c_Ssd1306.c
?? src/harmony/applications/sample/wifi-iot/app/8.0_New_Project/
```

查看变化规模：

```bash
git diff --stat
```

查看具体修改：

```bash
git diff
```

提交前确认变更范围全部位于：

```text
src/harmony/applications/sample/wifi-iot/app/
```

---

# 6. Stage

只添加 Linux 负责记录的目录：

```bash
git add src/harmony/applications/sample/wifi-iot/app
```

避免：

```bash
git add .
```

这样可以防止误提交：

* README
* docs
* IDE 配置
* Windows 管理的其他文件
* 临时文件

检查 staged 内容：

```bash
git status
```

查看 staged diff：

```bash
git diff --cached --stat
```

或者：

```bash
git diff --cached
```

---

# 7. Commit

根据实际修改内容编写 commit：

```bash
git commit -m "feat: update SSD1306 experiment"
```

常见示例：

```bash
git commit -m "feat: add ultrasonic sensor experiment"
```

```bash
git commit -m "fix: correct SSD1306 driver path"
```

```bash
git commit -m "refactor: reorganize wifi-iot app code"
```

```bash
git commit -m "docs: update experiment comments"
```

---

# 8. Push

正常情况下：

```bash
git push origin main
```

如果提交期间 GitHub `main` 已经发生变化，先：

```bash
git pull --rebase origin main
```

然后：

```bash
git push origin main
```

推荐保持：

```text
main
A -- B -- C -- D
```

而不是产生无必要的 merge commit。

---

# 9. Windows 同步

进入 Windows 仓库：

```powershell
cd D:\_GitHub\30_MyProjects\HarmonyOS-smart-car
```

先检查：

```bash
git status
```

如果 working tree clean：

```bash
git pull --ff-only origin main
```

GitHub 中记录的 Linux 最新代码会同步到：

```text
D:\_GitHub\30_MyProjects\HarmonyOS-smart-car
\src\harmony\applications\sample\wifi-iot\app
```

---

# 10. 推荐：创建同步脚本

为了避免每次手动输入长 `rsync` 命令，创建：

```bash
nano ~/sync_harmony_app.sh
```

写入：

```bash
#!/bin/bash
set -e

SRC="/home/harmony/harmony/code/code-1.0/applications/sample/wifi-iot/app/"
DST="/home/harmony/harmony/code/projects/HarmonyOS-smart-car/src/harmony/applications/sample/wifi-iot/app/"
REPO="/home/harmony/harmony/code/projects/HarmonyOS-smart-car"

mkdir -p "$DST"

rsync -av --delete "$SRC" "$DST"

cd "$REPO"

echo
echo "=== Git status ==="
git status --short

echo
echo "=== Diff stat ==="
git diff --stat
```

赋予执行权限：

```bash
chmod +x ~/sync_harmony_app.sh
```

以后完成开发后只需：

```bash
~/sync_harmony_app.sh
```

然后：

```bash
cd ~/harmony/code/projects/HarmonyOS-smart-car

git diff

git add src/harmony/applications/sample/wifi-iot/app

git commit -m "feat: update OpenHarmony application"

git push origin main
```

---

# 11. 完整日常命令模板

## Linux：开发完成后

```bash
cd ~/harmony/code/projects/HarmonyOS-smart-car

git pull --ff-only origin main

~/sync_harmony_app.sh

git diff

git add src/harmony/applications/sample/wifi-iot/app

git diff --cached --stat

git commit -m "<type>: <description>"

git push origin main
```

## Windows：同步记录

```powershell
cd D:\_GitHub\30_MyProjects\HarmonyOS-smart-car

git status

git pull --ff-only origin main
```

---

# 12. 目录大小检查

查看单个目录总大小：

```bash
du -sh <directory>
```

例如：

```bash
du -sh ~/harmony/code/code-1.0/applications/
```

查看一级子目录分别占用多少：

```bash
du -h --max-depth=1 <directory> | sort -h
```

例如：

```bash
du -h --max-depth=1 \
  ~/harmony/code/code-1.0/applications/sample/wifi-iot/app \
  | sort -h
```

查看磁盘整体空间：

```bash
df -h
```

---

# 13. Git 仓库大小检查

查看 `.git` 大小：

```bash
du -sh .git
```

查看 Git object 情况：

```bash
git count-objects -vH
```

如果曾经误提交大量文件并重写历史，确认无误后可清理 unreachable objects：

```bash
git reflog expire --expire=now --all
git gc --prune=now
```

再次检查：

```bash
du -sh .git
```

---

# 14. 不应执行的操作

日常流程中避免：

```bash
git add .
```

优先：

```bash
git add src/harmony/applications/sample/wifi-iot/app
```

不要把完整：

```text
code-1.0/
```

复制进 Git 仓库。

不要提交：

```text
out/
kernel/
third_party/
toolchain/
完整 OpenHarmony platform source
```

除非确实需要对这些内容进行版本管理。

不要把 Git 记录仓库当作实际编译目录。

不要直接在：

```text
~/harmony/code/projects/HarmonyOS-smart-car/src/harmony/...
```

中进行正常开发。

正式开发始终发生在：

```text
~/harmony/code/code-1.0/
```

---

# 15. 最终工作模型

```text
┌──────────────────────────────────────────┐
│ Linux OpenHarmony Workspace              │
│ ~/harmony/code/code-1.0                  │
│                                          │
│ Source editing                           │
│ Build                                    │
│ Flash                                    │
│ Debug                                    │
└───────────────────┬──────────────────────┘
                    │
                    │ rsync
                    ▼
┌──────────────────────────────────────────┐
│ Linux Git Record Repository              │
│ ~/harmony/code/projects/                 │
│ HarmonyOS-smart-car                      │
│                                          │
│ src/harmony/.../wifi-iot/app             │
│                                          │
│ Git snapshot / history only              │
└───────────────────┬──────────────────────┘
                    │
                    │ git push
                    ▼
                 GitHub
                    │
                    │ git pull
                    ▼
┌──────────────────────────────────────────┐
│ Windows Git Repository                   │
│ D:\_GitHub\30_MyProjects\                │
│ HarmonyOS-smart-car                      │
│                                          │
│ Local archive / inspection / management  │
└──────────────────────────────────────────┘
```

核心流程可压缩为：

```text
Develop on Linux workspace
        ↓
Build / test / flash
        ↓
rsync modified application source
        ↓
git diff
        ↓
git add scoped directory
        ↓
git commit
        ↓
git push
        ↓
Windows git pull
```
