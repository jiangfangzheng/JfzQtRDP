# JfzQtRDP

基于C++、Qt6、QWindowKit开发的RDP远程桌面管理小工具

之前做过[JfzRDP](https://github.com/jiangfangzheng/JfzRDP)，想测下AI发展怎么样了，突发奇想基于AI用Qt再复刻一个小工具，哈哈。

## 项目概述

JfzQtRDP 是一个基于 **Qt 6 Widgets + QWindowKit + Windows RDP ActiveX（mstscax.dll）** 的 Windows 远程桌面客户端。

界面使用 Qt 自带的 `windows11` 原生样式，没有使用任何 QSS/CSS 美化。窗口标题栏由 QWindowKit 接管，标题栏里放置了标签页和 Windows 11 风格的最小化、最大化、关闭按钮。

主要功能：

- 默认显示 `Home` 标签页。
- 手动连接：服务器、账号、密码、端口和 `Link` 按钮。
- 从 `server.json` 读取保存的服务器列表、最大化状态、宽度和高度。
- 保存的服务器列表：列表选择、右侧参数信息、`连接` 按钮。
- 新连接在标题栏新建标签页，标签页名称取 `ServerName`（手动连接时取服务器地址）。
- 鼠标中键点击 RDP 标签页会断开该连接并关闭标签页。
- 右上角关闭按钮弹出退出确认框，默认焦点和默认按钮均为“否”。
- RDP 意外断线后自动重连 3 次，仍失败则显示断开提示。
- `Maximized=false` 时窗口和 RDP 区域锁定为配置的 `Width`、`Height`；`Maximized=true` 时启动最大化，RDP 画面铺满标签页区域。

## 直接运行

双击 `JfzQtRDP.exe` 即可运行。程序第一次启动时，如果 `server.json` 不存在，会在 **JfzQtRDP.exe 同一目录** 生成默认配置。默认配置包含示例中的 `jfz123` 和 `JfzVictus-192` 两项。

> 运行日志写入同目录的 `JfzQtRDP.log`，每次启动覆盖。日志中不会记录密码。

## 使用说明

### 手动连接

`Home` 页面左侧“手动连接”分组：

- 服务器：默认 `192.168.233.254`
- 账号：默认 `Sandeepin`
- 密码：默认空
- 端口：`1`～`65535`，默认 `3389`
- `Link` 按钮：打开新的 RDP 标签页

手动连接没有 `ServerName` 字段，因此标签页名称使用服务器地址。

### 保存的服务器列表

右侧“保存的服务器列表”分组：

- 顶部显示从 `server.json` 读取的 `最大化：是/否  宽度：1920  高度：1080`
- 列表显示 `ServerName`
- 选中某一项后，右侧“参数信息”显示服务器、账号、密码、端口
- 点击“连接”按钮打开新的 RDP 标签页
- 列表项也支持双击连接

### 标签页操作

- 标题栏中的 `Home` 标签页不可关闭。
- RDP 标签页标题为 `ServerName`（手动连接为服务器地址）。
- 鼠标中键点击 RDP 标签页：断开该 RDP 连接并关闭标签页。
- 关闭标签页前会调用 ActiveX 的 `Disconnect()`。

### 退出确认

点击标题栏右上角关闭按钮（或按 `Alt+F4`）会弹出“退出确认”对话框：

- 默认按钮：`否`
- 默认键盘焦点：`否`
- 选择 `是`：断开所有 RDP 连接并退出。
- 选择 `否`、按 `Esc` 或关闭对话框：继续运行。

### 窗口大小规则

配置项 `Maximized` 控制启动时的窗口状态：

- `Maximized: true`
  - 启动时最大化。
  - 忽略 `Width`、`Height` 作为 RDP 桌面尺寸，RDP 使用标签页实际可用区域。
  - 最大化时使用的 RDP 桌面尺寸例如 3840×2080（取决于屏幕工作区和标题栏高度）。
  - 本程序启用 `SmartSizing`，窗口尺寸变化时由 RDP 控件缩放画面填充。
- `Maximized: false`
  - 程序窗口固定，不能拖动边框调整大小，最大化按钮禁用，双击标题栏也不会最大化。
  - 标签页内容区域锁定为 `Width × Height`；窗口客户区高度为 `Height + 32`（32 为标题栏高度）。
  - 例如 `Width=1920, Height=1080` 时，RDP 区域为 1920×1080，窗口客户区为 1920×1112。

## server.json

文件位置：与 `JfzQtRDP.exe` 同目录。

默认生成内容示例：

```json
{
    "Height": 1080,
    "Maximized": false,
    "ServerList": [
        {
            "Password": "123456",
            "Port": 3389,
            "Server": "127.0.0.1",
            "ServerName": "jfz123",
            "UserName": "Sandeepin"
        },
        {
            "Password": "1",
            "Port": 3389,
            "Server": "192.168.233.168",
            "ServerName": "JfzVictus-192",
            "UserName": "Sandeepin"
        }
    ],
    "Width": 1920
}
```

字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `Maximized` | bool | 是否启动最大化 |
| `Width` | int | 非最大化时 RDP 标签页内容宽度 |
| `Height` | int | 非最大化时 RDP 标签页内容高度 |
| `ServerList` | array | 保存的服务器列表 |
| `ServerName` | string | 标签页显示名称 |
| `Server` | string | 服务器地址或主机名 |
| `UserName` | string | 登录账号 |
| `Password` | string | 登录密码（按需求明文保存） |
| `Port` | int | RDP 端口，1～65535 |

说明：

- 如果文件不存在，程序自动生成默认文件。
- 如果 JSON 格式错误，程序使用默认配置启动，并弹窗提示；不会覆盖原文件。
- `Port` 超出范围时会夹紧到 1～65535。
- `Width`、`Height` 会限制在 320～16384、240～16384 的安全范围内。

## 已知限制

- 只支持 Windows。RDP 控件依赖系统的 `mstscax.dll`。
- 按需求，`server.json` 中的密码是明文。
- 隐藏了 ActiveX 自带连接栏，因此断开/重连状态使用本程序自己的横幅/失败页提示。
- `SmartSizing` 会按控件大小缩放远程画面；如果远程桌面和标签页宽高比不同，可能出现黑边，这是 RDP 缩放行为。
- 程序不会自动修改 `server.json`；修改配置后需要重启程序生效。
- 日志文件为 `JfzQtRDP.log`，位于 exe 同目录。
