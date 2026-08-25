# Windows 终端通过 SSH 远程控制 Linux 虚拟机

本文解释如何从 Windows Terminal 或 PowerShell 通过 SSH 登录同一台计算机或局域网中的 Linux 虚拟机，并说明连接背后的工作机制。

## 1. SSH 是什么

SSH（Secure Shell）是一种加密的远程登录协议。Windows 运行 SSH 客户端，Linux 虚拟机运行 SSH 服务端。连接成功后，Windows 终端中的键盘输入会被加密传给虚拟机中的 Shell，命令执行结果再通过同一条加密连接返回。

```text
Windows Terminal
    -> OpenSSH client
    -> TCP/IP network
    -> virtual machine IP:22
    -> Linux sshd service
    -> authenticated Linux shell
```

SSH 通常使用 TCP 22 端口。它提供三项核心安全能力：

1. 加密：防止网络中的第三方直接读取命令、密码和返回内容。
2. 主机身份验证：客户端使用主机密钥确认连接的是预期服务器。
3. 用户身份验证：服务端通过密码或公钥确认登录用户身份。

SSH 只是远程控制通道。命令实际由 Linux 虚拟机执行，消耗的也是虚拟机的 CPU、内存和磁盘资源。

## 2. 虚拟机网络模式

Windows 能否连接虚拟机，首先取决于两者之间是否存在可达的 IP 网络路径。

### NAT 模式

虚拟机通过宿主机共享网络访问外部网络。多数桌面虚拟化软件还会创建一个仅在宿主机与虚拟机之间使用的虚拟网段，因此宿主机通常可以直接访问虚拟机 IP。若软件的 NAT 配置不允许直接访问，则需要设置端口转发，例如把 Windows 的 `127.0.0.1:2222` 转发到虚拟机的 `22` 端口。

### Bridged 模式

桥接模式让虚拟机像局域网中的独立设备一样获取 IP。Windows 通常可直接连接该 IP，但连接是否成功还受局域网隔离和防火墙规则影响。

### Host-only 模式

该模式建立仅供宿主机与虚拟机通信的私有网络，适合不希望虚拟机暴露给外部局域网的实验环境。虚拟机可能无法直接访问互联网，除非额外配置路由或第二块虚拟网卡。

对于本机学习环境，优先尝试 NAT；需要其他局域网设备访问虚拟机时再考虑桥接。网络模式更改后，虚拟机 IP 可能发生变化。

## 3. Linux 虚拟机端配置

以下示例适用于 Ubuntu/Debian。

### 安装并启动 OpenSSH Server

```bash
sudo apt update
sudo apt install openssh-server
sudo systemctl enable --now ssh
```

确认服务状态与监听端口：

```bash
systemctl status ssh
ss -tlnp | grep ':22'
```

在 Fedora/RHEL 系列中，软件包和服务通常名为 `openssh-server` 与 `sshd`：

```bash
sudo dnf install openssh-server
sudo systemctl enable --now sshd
```

### 获取虚拟机 IP 地址

```bash
hostname -I
ip addr
```

假设得到 `192.168.56.101`。不要使用 `127.0.0.1`，因为虚拟机中的 `127.0.0.1` 只代表虚拟机自身；Windows 中的 `127.0.0.1` 则代表 Windows 自身。

### 检查防火墙

Ubuntu 使用 UFW 时：

```bash
sudo ufw allow OpenSSH
sudo ufw status
```

只应开放实际需要的端口。若 SSH 仅供宿主机使用，可进一步把规则限制到宿主机或虚拟网段。

## 4. 从 Windows 建立连接

Windows 10/11 通常提供 OpenSSH 客户端。在 PowerShell 中确认：

```powershell
ssh -V
```

使用 Linux 用户名和虚拟机 IP 登录：

```powershell
ssh linuxuser@192.168.56.101
```

首次连接会显示服务器主机密钥指纹。应在虚拟机中运行下面的命令并核对指纹，而不是不加检查地输入 `yes`：

```bash
ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub
```

确认一致后接受主机密钥，再输入 Linux 用户密码。密码输入时终端不会显示字符或星号，这是正常的安全行为。

退出远程会话：

```bash
exit
```

如果 NAT 使用端口转发，Windows 端命令可能是：

```powershell
ssh -p 2222 linuxuser@127.0.0.1
```

这里的 `2222` 是宿主机端口，虚拟化软件会把流量转发到虚拟机的 TCP 22 端口。

## 5. 使用密钥登录

密码认证简单，但公钥认证更适合长期使用和自动化。

### 在 Windows 生成密钥

```powershell
ssh-keygen -t ed25519 -C "windows-to-linux-vm"
```

默认生成：

- 私钥：`$HOME\.ssh\id_ed25519`，必须保密。
- 公钥：`$HOME\.ssh\id_ed25519.pub`，可以复制到服务器。

建议为私钥设置口令。私钥始终留在 Windows；传到 Linux 的只是公钥。

### 将公钥加入 Linux

Windows 默认不一定包含 `ssh-copy-id`，可以在 PowerShell 中执行：

```powershell
Get-Content "$HOME\.ssh\id_ed25519.pub" | ssh linuxuser@192.168.56.101 "umask 077; mkdir -p ~/.ssh; cat >> ~/.ssh/authorized_keys"
```

再次登录验证：

```powershell
ssh linuxuser@192.168.56.101
```

密钥认证的原理不是“发送私钥”。服务器发送一次性挑战，客户端用私钥完成签名，服务器使用 `authorized_keys` 中的公钥验证签名。私钥不会离开 Windows。

只有在密钥登录验证成功并保留可用的管理会话后，才考虑关闭密码登录。修改 `/etc/ssh/sshd_config` 后应先检查语法，再重载服务：

```bash
sudo sshd -t
sudo systemctl reload ssh
```

## 6. 简化连接配置

编辑 Windows 用户目录下的 `$HOME\.ssh\config`：

```sshconfig
Host linux-vm
    HostName 192.168.56.101
    User linuxuser
    Port 22
    IdentityFile ~/.ssh/id_ed25519
```

之后可以直接运行：

```powershell
ssh linux-vm
```

如果虚拟机使用 DHCP，IP 可能变化。可以在虚拟化软件中配置固定租约，或在可解析的网络环境中使用稳定主机名。

## 7. 远程执行与文件传输

执行单条远程命令：

```powershell
ssh linux-vm "uname -a && uptime"
```

上传文件：

```powershell
scp .\app.py linux-vm:~/project/
```

下载文件：

```powershell
scp linux-vm:~/project/result.txt .\
```

递归复制目录：

```powershell
scp -r .\project linux-vm:~/
```

`scp` 复用 SSH 的身份验证和加密通道。Windows 路径由本地 PowerShell 解释，`linux-vm:~/...` 则表示远程 Linux 路径。

## 8. 一次 SSH 连接如何建立

连接过程可以概括为：

1. Windows 根据 IP 和端口，通过 TCP 与虚拟机建立可靠连接。
2. 客户端和服务端协商 SSH 协议版本、密钥交换算法、加密算法与完整性算法。
3. 双方执行密钥交换，生成本次会话使用的对称加密密钥。
4. 服务端使用主机私钥证明其身份；客户端检查本地 `known_hosts` 中的主机公钥记录。
5. 用户通过密码或公钥完成身份验证。
6. 服务端为用户创建会话和 Shell，双方在加密通道中传输输入、输出与控制信息。

公钥密码学主要用于身份验证和安全地建立会话密钥；会话中的大量数据通常使用效率更高的对称加密算法处理。

## 9. 常见故障定位

按网络路径从低到高逐层检查：

### `Connection timed out`

通常表示 IP 不可达、网络模式不允许通信或防火墙丢弃流量。

```powershell
Test-NetConnection 192.168.56.101 -Port 22
```

同时检查虚拟机 IP、网络模式和防火墙。

### `Connection refused`

主机可达，但目标端口没有服务监听，或防火墙主动拒绝。进入虚拟机控制台检查：

```bash
sudo systemctl status ssh
sudo ss -tlnp | grep ':22'
```

### `Permission denied`

网络和 SSH 服务已经正常，问题出在用户身份验证。检查用户名、公钥内容及权限：

```bash
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
```

调试客户端认证过程：

```powershell
ssh -vvv linuxuser@192.168.56.101
```

### 主机密钥发生变化

重装虚拟机或重新生成 SSH 主机密钥后，客户端可能报告主机身份变化。先通过虚拟机控制台核实新指纹和变化原因；确认安全后再删除旧记录：

```powershell
ssh-keygen -R 192.168.56.101
```

不要在未确认原因时忽略警告，因为这也可能意味着中间人攻击。

## 10. 最小验证清单

1. 虚拟机中 `systemctl status ssh` 显示服务正在运行。
2. 虚拟机中 `ss -tlnp` 显示 TCP 22 端口正在监听。
3. Windows 中 `Test-NetConnection <VM-IP> -Port 22` 成功。
4. 第一次登录时已核对服务器主机密钥指纹。
5. 登录后运行 `hostname`、`whoami` 和 `pwd`，确认当前操作对象确实是虚拟机。
6. 密钥登录成功后再决定是否关闭密码认证。

## 快速检查

1. 为什么 Windows 和 Linux 虚拟机中的 `127.0.0.1` 不是同一个网络端点？
2. `Connection timed out` 与 `Connection refused` 通常分别说明什么？
3. 公钥认证过程中，私钥是否会发送到 Linux 虚拟机？
4. NAT、桥接和 Host-only 三种模式的主要差异是什么？

