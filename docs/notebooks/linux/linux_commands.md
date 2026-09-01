# Linux 核心命令教程

本文面向 Linux 初学者，覆盖文件系统导航、文件操作、文本查看、搜索、权限、进程、网络和软件包管理等常用命令。命令中的路径和文件名区分大小写。

## 1. 命令的基本结构

```bash
command [options] [arguments]
```

例如：

```bash
ls -la /home
```

- `ls`：命令。
- `-la`：选项，`-l` 表示详细列表，`-a` 表示显示隐藏文件。
- `/home`：操作对象。

可使用 `man` 或 `--help` 查看帮助：

```bash
man ls
ls --help
```

## 2. 目录导航

### `pwd`：显示当前目录

```bash
pwd
```

### `ls`：列出目录内容

```bash
ls
ls -lah
ls /etc
```

### `cd`：切换目录

```bash
cd /var/log     # 进入绝对路径
cd ..           # 返回上一级
cd ~            # 回到当前用户主目录
cd -            # 回到上一次所在目录
```

Linux 路径以 `/` 为根目录。以 `/` 开头的是绝对路径；相对于当前目录书写的是相对路径。

## 3. 创建、复制、移动和删除

### 创建目录与文件

```bash
mkdir project
mkdir -p project/src/utils
touch project/README.md
```

`mkdir -p` 会同时创建缺失的父目录。`touch` 在文件不存在时创建空文件，在文件存在时更新时间戳。

### 复制与移动

```bash
cp README.md README.backup.md
cp -r project project_backup
mv README.backup.md docs.md
mv docs.md project/
```

`mv` 既可移动文件，也可重命名文件。

### 删除

```bash
rm old.txt
rmdir empty_directory
rm -r old_project
```

`rm` 默认不可恢复。执行递归删除前，应先用 `pwd` 和 `ls` 确认目标；不要随意使用 `rm -rf`。

## 4. 查看和编辑文本

```bash
cat README.md             # 一次输出整个文件
less /var/log/syslog      # 分页查看，按 q 退出
head -n 10 data.txt       # 查看前 10 行
tail -n 20 data.txt       # 查看后 20 行
tail -f app.log           # 持续跟踪新增日志
```

常见终端编辑器：

```bash
nano notes.md
vim notes.md
```

## 5. 搜索文件和文本

### `find`：按文件属性搜索

```bash
find . -name "*.py"
find /var/log -type f -size +10M
```

### `grep`：在文本中搜索

```bash
grep "ERROR" app.log
grep -n -i "warning" app.log
grep -r "TODO" src/
```

- `-n`：显示行号。
- `-i`：忽略大小写。
- `-r`：递归搜索目录。

管道 `|` 会把前一个命令的标准输出交给后一个命令：

```bash
ps aux | grep python
ls -lah | less
```

## 6. 重定向与管道

```bash
echo "hello" > output.txt       # 覆盖写入
echo "world" >> output.txt      # 追加写入
grep "ERROR" app.log > errors.txt
command 2> error.log             # 重定向标准错误
```

`>` 会覆盖原文件，`>>` 才是追加，使用前应确认目标文件。

## 7. 权限与用户身份

```bash
ls -l script.sh
chmod +x script.sh
chmod 644 config.txt
whoami
id
sudo command
```

Linux 权限分为读 `r`、写 `w`、执行 `x`，并分别作用于文件所有者、所属组和其他用户。`chmod 644` 表示所有者可读写，其他用户只读。`sudo` 仅应在确实需要管理员权限时使用。

## 8. 进程和系统状态

```bash
ps aux                    # 查看进程快照
top                       # 动态查看系统负载
kill 1234                 # 请求 PID 1234 正常退出
kill -9 1234              # 强制终止，仅作为最后手段
df -h                     # 查看磁盘文件系统使用量
du -sh ./project          # 查看目录占用空间
free -h                   # 查看内存使用量
uname -a                  # 查看内核与系统信息
```

先用 `ps` 确认 PID。普通 `kill` 发送 `SIGTERM`，允许程序清理资源；`kill -9` 发送不可捕获的 `SIGKILL`。

## 9. 网络相关命令

```bash
ip addr                   # 查看网络接口和 IP
ip route                  # 查看路由表
ping -c 4 8.8.8.8         # 测试网络连通性
curl https://example.com  # 发起 HTTP 请求
ss -tulpn                 # 查看监听端口和连接
ssh user@192.168.1.100    # 登录远程主机
scp file.txt user@192.168.1.100:/tmp/
```

## 10. 压缩与解压

```bash
tar -czf project.tar.gz project/
tar -xzf project.tar.gz
zip -r project.zip project/
unzip project.zip
```

## 11. 软件包管理

Ubuntu/Debian：

```bash
sudo apt update
sudo apt install git
sudo apt remove git
```

Fedora/RHEL 系列：

```bash
sudo dnf install git
sudo dnf remove git
```

不同发行版的软件包管理器和包名可能不同，应优先参考对应发行版的官方文档。

## 12. 综合练习

```bash
mkdir -p linux_practice/logs
cd linux_practice
printf "INFO startup\nERROR connection failed\n" > logs/app.log
grep -n "ERROR" logs/app.log > error_summary.txt
cat error_summary.txt
cp error_summary.txt error_summary.backup.txt
ls -lah
```

这个例子依次练习了目录创建、路径切换、输出重定向、文本搜索、复制和目录查看。

## 常见错误

- 混淆绝对路径和相对路径：操作前使用 `pwd` 确认当前位置。
- 忘记 Linux 文件名区分大小写：`File.txt` 与 `file.txt` 是两个文件。
- 直接复制带有 `$` 的教程命令：文档中的 `$` 往往只是普通用户提示符，不是命令内容。
- 滥用 `sudo` 或 `rm -rf`：先理解权限和删除范围，再执行高风险命令。
- 用 `kill -9` 代替正常退出：优先使用普通 `kill`，让进程有机会清理资源。

## 快速检查

1. `>` 与 `>>` 有什么区别？
2. 为什么删除目录前应先执行 `pwd` 和 `ls`？
3. 如何递归查找当前目录中包含 `TODO` 的文件？
4. `chmod +x script.sh` 改变了什么？

