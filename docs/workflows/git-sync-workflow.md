# Git同步与推送流程

## Linux/OpenHarmony侧

```bash
cd ~/harmony/code/projects/HarmonyOS-smart-car
git pull --ff-only origin main
```

把完整OpenHarmony源码树中实际使用的 `applications/sample/wifi-iot/app/` 内容同步到本仓库 `src/harmony/`，不要重新引入 `applications/sample/wifi-iot/app` 外层目录。检查差异后提交：

```bash
git status --short
git diff --check
git diff
git add src/harmony docs README.md AGENTS.md
git diff --cached --stat
git commit -m "<type>: <description>"
git push origin main
```

## Windows侧

```powershell
Set-Location D:\_GitHub\30_MyProjects\HarmonyOS-smart-car
git status --short
git pull --ff-only origin main
```

推送前必须检查：

1. 变更模块的README是否仍准确描述功能、接口、协议和构建入口。
2. 新增、删除或移动目录后，上级README中的结构表是否同步。
3. `src/harmony/BUILD.gn` 是否只启用了本次需要且互不冲突的Feature。
4. 构建成功、源码检查和实车验收是否被明确区分。
5. `git diff --check`、Markdown相对链接和 `git status --short` 是否正常。

Agent协作的强制规则见仓库根目录 `AGENTS.md`。
