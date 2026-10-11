# RTX51 Tiny 移植到 SC95F7617B

将本仓库 `src/templete/` 中的 **RTX51 Tiny (v2.02)** 内核移植到 `soc/` 下的 SinOne
**SC95F7617B**（超级 1T 8051 内核，Fsys = 32 MHz）单片机工程。本目录是一个**独立**的 Keil
uVision 工程，不修改厂商 demo（`SC95F7617B_..._Demo_Code/`）。

## 目录内容

| 文件 | 说明 |
|------|------|
| `RTX51_Tiny.uvproj` | Keil 工程（目标名 `RTX51 Tiny`，`RTOS = RTX-51 Tiny`） |
| `Conf_tny.A51` | RTX51 Tiny 配置 + 内核（`main` / Timer0 中断 / 任务切换），已按 32 MHz 适配 |
| `STARTUP.A51` | 标准 8051 启动代码（取自厂商 demo，未修改） |
| `rtx_app.c` | 示例应用：task0 初始化、task1 周期打印、task2 信号唤醒 |
| `uart.c` / `uart.h` | UART0 查询发送驱动（115200 bps @ 32 MHz） |
| `README.md` | 本文件 |

## 关键移植点

### 1. 系统节拍（INT_CLOCK）

RTX51 Tiny 用 **Timer0** 作为系统时钟。SC95F7617B 的 `TMCON.0`（`T0FD`）选择 Timer0 时钟源
（数据手册 p.102）：

- `T0FD = 0` → T0 时钟 = `Fsys/12` = 32 MHz / 12 = **2.6667 MHz**（一个机器周期 0.375 µs）
- `T0FD = 1` → T0 时钟 = `Fsys` = 32 MHz

由于 16 位定时器最大只有 65536 个计数（`Fsys` 下仅 2.05 ms），无法得到 10 ms 节拍，故本移植
**保持 `T0FD = 0`（Fsys/12）**，并取：

```
INT_CLOCK = 26667   ->   26667 x 0.375us ≈ 10.00ms  (系统节拍 10ms)
```

内核启动代码在 `ORL TMOD,#01H` 之前插入 `ANL TMCON,#0FEH` 显式清零 `T0FD`，避免被其它初始化
改动。`TIMESHARING = 5`（时间片 50 ms）。

> 与厂商 demo 的区别：厂商 `Timer_Init.c` 用 `TMCON = 0x07` 把 T0/T1/T2 都设为 `Fsys` 并占用
> Timer0 做 PWM。本工程中 **Timer0 归内核所有**，其它模块（如 UART）只能使用 Timer1/Timer2/3/4。

### 2. 工程与库

- 目标选项选择 **Operating System = RTX-51 Tiny**（`<RTOS>1</RTOS>`），uVision 会自动链接
  `C51\LIB\RTX51TNY.LIB`（系统调用库，与仓库 `src/templete/RTX51TNY.LIB` 同为 v2.02）。
- 应用**提供** `Conf_tny.A51`，它定义了 `main`、`os_switch_task`、任务表等内核符号；因此库中的
  内核模块不会被链接出来，配置以本工程为准。`?RTX_MAXTASKN` 由 C51 依据 `_task_` 编号自动生成。
- 头文件 `#include <rtx51tny.h>` 走 Keil 默认 `C51\INC`；`Function_Init.H` / `SC95F761xB_C.H`
  通过 IncludePath 指向 `..\SC95F7617B_7616B_7615B_7613B_Demo_Code\H`。
- 与厂商 demo 的冲突项已排除：`main.c`（重复定义 `main`）与 `Timer_Init.c`（重复占用 Timer0
  中断向量 0x0B）**不在本工程中**。

### 3. 代码空间

沿用 SoC 工具链设置：LX51 链接、`REMOVEUNUSED`、`CodeRange = 0x100`。内核的中断向量（复位
0x0000、Timer0 0x000B）仍由绝对段放在低地址，应用代码从 0x100 起。

## 编译

- 直接双击打开 `RTX51_Tiny.uvproj` 编译；或命令行：
  `UV4.exe -b RTX51_Tiny.uvproj -o build.log`
- 输出：`Objects\RTX51_Tiny.hex`

## 上电现象

UART0（**P2.0 = TX / P2.1 = RX**，115200-8-N-1）开机打印一行横幅，之后每秒输出一行：

```
RTX51 Tiny on SC95F7617B (32MHz, 10ms tick)
task2: got signal, t = 1 s
task2: got signal, t = 2 s
task2: got signal, t = 3 s
...
```

- task1（producer）用 `os_wait(K_IVL,100,0)` 每 ~1s 周期唤醒，递增计数并 `os_send_signal(2)`；
- task2（printer）用 `os_wait(K_SIG,0,0)` 等待信号并打印，演示 RTX51 Tiny 的**时间(interval)**
  与**信号(signal)**两种等待方式。
- 串口发送函数只由 task2 调用，避免非重入函数被多个任务共享而产生的链接器 L15 覆盖警告。
