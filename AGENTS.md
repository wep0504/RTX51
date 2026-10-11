# AGENTS.md — 项目记忆与工作约定（RTX51 / SC95F7617B）

> 用途：给未来的 AI 会话/协作者提供持久记忆，避免重复踩坑、重复调研。
> 最后更新：2026-10-11。凡与源码/数据手册冲突处，以源码/手册为准。

---

## 1. 这个仓库是什么

`RTX51`（OS-Q）仓库里**并排存在 4 套互相重叠的 8051 代码**，动手前先用
`docs/CODE_FLOW.md` §1 分清归属，别改错地方：

| 目录 | 内容 | 是否可编译 |
|------|------|-----------|
| `src/templete/` | **官方 RTX51 Tiny v2.02**：`RTX51TNY.LIB` + `os_*.a51` + `Conf_tny.A51` | 库工程 |
| `src/rtx/` | 自研"时间片轮询"内核（**与 RTX51 无关**，`OS.h` 依赖缺失，编译不过） | 否 |
| `src/usr/` | DMX512 舞台灯应用（STC15），全仓库唯一完整自洽 | 是 |
| `project/src/` + `project/keil/demo.uvproj` | 电机 PID 应用，include 路径断裂 | 否 |
| `soc/` | **SinOne SC95F7617B** 目标：厂商 demo + 数据手册 + 本次移植工程 | 见 §5 |

关键澄清：RTX51 Tiny 的**内核（`main`/调度/Timer0 中断）在 `Conf_tny.A51`**，`.LIB` 只提供
`os_*` 系统调用；模板工程必须自带 `Conf_tny.A51`。

## 2. 环境 / 工具链（已实测）

- Keil：`D:\Keil_v5`，C51 **V9.60.7**，LX51 V4.66，`UV4.exe`。
- 命令行编译（会阻塞等待）：
  ```
  Start-Process D:\Keil_v5\UV4\UV4.exe `
    -ArgumentList '-b','"<x.uvproj>"','-o','"<build.log>"' -Wait -PassThru
  ```
  **ExitCode：0=0错误0警告；1=有警告；≥2=有错误。** 日志同时写入 `Objects\<name>.build_log.htm`。
- 标准 RTX 资源：`C51\LIB\RTX51TNY.LIB`、`C51\INC\RTX51TNY.H`；官方例程 `C51\RtxTiny2\Examples\`。
- ⚠️ **编码坑（重要）**：`soc/**/*.c`、`*.H` 是 **GBK**，用 `grep`/ripgrep 工具会报
  `Invalid ripgrep match output`。改用 PowerShell：
  `Select-String -Path ... -Pattern ... -Encoding Default`。
  `src/**/*.A51`、`*.md` 是 **UTF-8**。新写文件统一用 **UTF-8**；**源码注释建议 ASCII**
  以免 C51/编辑器编码问题（编译不受影响，纯稳健性考虑）。
- 数据手册 PDF 有文本层，可用 Python `pdfminer.high_level.extract_text` 提取（表格会错行，
  需按字段名重组）。

## 3. RTX51 Tiny v2.02 关键事实（源码级）

- 目标选 **Operating System = RTX-51 Tiny**（uvproj `<RTOS>1</RTOS>`）→ uVision **自动链接**
  `C51\LIB\RTX51TNY.LIB`，**无需**手动加库文件。
- `?RTX_MAXTASKN` 由 **C51 依 `_task_` 编号自动生成**（最高任务号），不是库定义。
- **Timer0 被内核独占**：`interrupt 1` / 向量 `0x0B`。用户代码禁止再定义 `interrupt 1`。
- `INT_REGBANK=1` ⇒ `?RTX_REGISTERBANK=8`，内核变量在 `DSEG AT 0x08`：
  `?RTX_SAVEACC=0x0A`、`?RTX_SAVEPSW=0x0B`、`?RTX_CURRENTTASK=0x0C`、`?RTX_ROBINTIME=0x0D`。
  ⚠️ **这些地址在寄存器组 1（0x08~0x0F）内 → 用户中断函数禁止 `using 1`**（会覆盖内核变量）。
- `os_wait(typ, ticks, dummy)`：`typ` = `K_SIG(0x01)`/`K_TMO(0x02)`/`K_IVL(0x80)` 按位或；
  `ticks` 是 **uchar(0~255)**；返回 `SIG_EVENT(0x04)`/`TMO_EVENT(0x08)`/`RDY_EVENT(0x80)`。
  - `K_TMO`：每次调用**重装**计时器（长期漂移）；`K_IVL`：计时器**累加**（周期更准）。
  - `ticks==0` 且带 TMO/IVL：**立即返回**（非阻塞）。
- **信号是标志位不是计数器**：目标 `os_wait(K_SIG)` 消费前多次发送会被合并成一次。
  `os_send_signal` 源码返回 **0（成功）/1（此前已有未决信号）/0xFF（越界）**。
- 任务函数**永不返回**（`while(1)` 或自删）；必须存在 `_task_ 0`；最大任务号 15。
- **非重入函数被多个任务调用 → 链接器 `L15`**。对策优先级：①让函数只属于一个任务的调用树
  （本次采用"单任务独占 UART"）②`reentrant`（注意重入栈与 RTX 的 IDATA 任务栈不能重叠）
  ③屏蔽（不推荐）。在 `TIMESHARING!=0` 下时间片可在**任意指令边界**切换，风险真实。
- 中断里只能调 `isr_send_signal` / `isr_set_ready`；**不可**在中断里 `os_wait`/`os_create_task`/
  `os_delete_task`/`os_send_signal`。

## 4. SC95F7617B 适配要点（已落地）

- 内核：**超级 1T 8051**，`Fsys=32MHz`；SFR 与标准 8051 兼容（`TMOD 0x89`/`TL0 0x8A`/
  `TH0 0x8C`/`TCON 0x88`/`IE 0xA8`/`ET0`/`TR0` 等）。
- `TMCON(0x8E).0 = T0FD`：**0 → T0 时钟 = Fsys/12；1 → Fsys**（数据手册 **p.102**）。
  本移植取 **0**（32/12 = 2.6667 MHz），`INT_CLOCK=26667` → **~10ms 节拍**。
  公式：`INT_CLOCK = tick秒 × Fsys/12`；16 位上限 → 单 tick ≤ ~24.6ms；`ticks` uchar → 单次等待 ≤ ~2.55s。
  内核 `main` 里加了 `ANL TMCON,#0FEH`（需自行 `TMCON DATA 08EH`，因 `$NOMOD51`）。
- **UART0 波特率发生器用 Timer1**（`TMCON.1 T1FD=1`）。其它模块计时用 Timer1~4 / BTM，别碰 Timer0。
- 中断向量（C51 interrupt n / 向量）：INT0=0/0003H、**Timer0=1/000BH(内核用)**、INT1=2/0013H、
  Timer1=3/001BH、UART0=4/0023H、Timer2=5/002BH、ADC=6/0033H、USCI0=7/003BH、PWM=8/0043H、
  BTM=9/004BH、INT2=10/0053H、CMP=12/0063H、Timer3=13/006BH、Timer4=14/0073H、USCI1~5=15~19。
- 布局：`CodeRange=0x100`；IDATA 256B（`RAMTOP=0FFH`）；XRAM 4KB；内存模型 SMALL。

## 5. 已交付产物（本次工作）

`soc/RTX51_Tiny_Demo/`（独立工程，不改厂商 demo）：

| 文件 | 说明 |
|------|------|
| `RTX51_Tiny.uvproj` | Keil 工程（RTOS=RTX-51 Tiny；IncludePath 指向厂商 `H/`） |
| `Conf_tny.A51` | v2.02 内核/配置，已按 32MHz 适配（INT_CLOCK=26667，加 TMCON 清零） |
| `STARTUP.A51` | 厂商标准启动代码（原样） |
| `rtx_app.c` / `uart.c` / `uart.h` | 示例：task0 初始化、task1 周期(信号)、task2 串口打印 |
| `README.md` | 移植说明与现象 |
| `USAGE.md` | **RTX51 Tiny 完整使用手册**（API/配置/适配/陷阱/模板，11 章） |

验证：**0 Error 0 Warning**，`data=32.1 xdata=0 const=77 code=1309`；hex 验证
`0x0000: LJMP STARTUP1`、`0x000B: LJMP TIMERINT`；map 验证内核符号来自 `Conf_tny.obj`、
系统调用来自 `C51\LIB\RTX51TNY.LIB`、`?RTX_CURRENTTASK=0x0C`。
构建产物（`Objects/`、`Listings/`、生成的 `.uvopt/.uvgui`）**已清理，勿提交**（`.gitignore` 覆盖部分）。

## 6. 工作约定（本仓库）

1. 先读 `docs/CODE_FLOW.md` 区分 4 套代码归属，再动手。
2. 新硬件工程放 `soc/<name>/`，做**独立工程**，不改厂商 demo；新 RTOS 集成排除冲突文件
   （定义 `main` 或 `interrupt 1` 的源文件）。
3. 中文文档/注释；源码内注释用 ASCII；新文件 UTF-8。
4. 结论要能**溯源**到 `文件:行` 或数据手册页码；不确定就查源码/手册，不臆测。
5. 改动前确认 **目标 MCU / 主频 / 工具链**，因为它们决定 `INT_CLOCK`、`TMCON`、代码布局。
6. 用 Keil 命令行**实际编译验证**（0/0）再宣称完成；验证后清理构建产物。

---

参考入口：`soc/RTX51_Tiny_Demo/USAGE.md`（RTX51 手册）、`soc/RTX51_Tiny_Demo/README.md`（移植）、
`docs/CODE_FLOW.md`（仓库代码流程）、`CLAUDE.md`（编码行为准则）。
