# Repository workflow for agents

本文件适用于整个仓库。目标是让源码、目录结构、接口说明和实际验收状态始终一致。

## 修改前

1. 运行 `git status --short`，把已有修改视为用户工作，不覆盖、不回滚。
2. 阅读根README、目标模块README及其实际构建入口：Hi3861检查 `BUILD.gn`，STM32检查 `USER/Template.uvprojx`。
3. 跨Hi3861/STM32改动必须同时核对接线、波特率、帧格式、超时和速度范围。

## README同步规则

当Agent新增、删除、重命名或修改功能时，必须在同一次变更中更新文档：

1. 更新最近的模块README：功能、可观察效果、入口文件、关键引脚、协议和配对模块。
2. 模块列表或目录发生变化时，更新上一级README的索引或结构树。
3. 顶层架构、主要链路或一级目录变化时，更新根README。
4. materials和notebooks只按大类维护索引，不为每个二进制、字模或底层库生成冗余说明。
5. README必须来自当前文件树和源码常量；禁止复制旧路径、旧速度上限、旧固件行为或未验证结论。

## Push前强制检查

用户要求Agent提交或push时，Agent必须把README同步纳入任务范围，不需要用户再次提醒。推送前至少执行：

```text
git status --short
git diff --check
git diff --cached --stat
```

并核对：

- 所有新增一级模块都有README。
- README中的相对链接、目录名、构建目标和接口常量确实存在。
- `src/harmony/BUILD.gn` 未同时启用会竞争相同GPIO、UART或硬件资源的应用。
- 未把OBJ、HEX、虚拟机镜像或第三方安装包误当作源码提交。
- 构建结果与硬件验收分开描述；未实测的行为必须标记为待验证。

只有用户明确要求时才执行commit或push。遇到认证失败、远端冲突或不确定的用户改动时停止外部写入并报告证据。

## 结构约定

- `src/harmony/`直接保存OpenHarmony `app/`下的文件和示例目录，不重新增加 `applications/sample/wifi-iot/app` 外层。
- `src/stm32/<project>/`保持独立Keil工程布局，入口为 `USER/Template.uvprojx`。
- `materials/`按用途分类，第三方二进制默认忽略，仅跟踪分类README。
- `docs/notebooks/`按知识主题分类；`docs/workflows/`保存可复用操作流程。
