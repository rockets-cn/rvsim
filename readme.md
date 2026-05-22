# RVSim

RVSim 是一个用 C 编写的 RV32 RISC-V 模拟器。它面向学习、实验和操作系统启动验证，不追求完整硬件精确建模。

项目当前主线是 RV32：模拟器从 `0x80000000` 启动，加载裸二进制镜像到内存起始位置，并可把 DTB 放到模拟内存末端后通过 `a1` 传给客户程序。仓库也包含一个更新版本提示：如果需要同时支持 32 位和 64 位模式，可以参考 <https://github.com/telos27/rv64sim>。

## 功能概览

- RV32 整数指令基础实现，包括算术、逻辑、移位、跳转、分支、load/store。
- RV32M 乘除法指令。
- RV32A 原子内存指令的基础实现，包括 AMO 与 LR/SC。
- CSR、`ecall`、`ebreak`、`mret`、`sret`、`wfi`、`sfence.vma` 等系统指令相关路径。
- Machine mode 与 Supervisor mode 的基本 trap/interrupt 流程。
- Sv32 两级页表地址转换。
- 64 MiB 模拟内存。
- UART、CLINT timer、PLIC 和 legacy virtio block 的简化 MMIO 模型。
- Linux/no-MMU、xv6 风格启动实验所需的镜像、DTB 和测试材料。

## 当前边界

这个项目更像一个教学型/实验型模拟器，而不是完整的 RISC-V 平台模拟器。代码中仍有不少有意保留的 TODO，例如 PMP/PMA、TLB、完整 PTE 权限检查、PLIC 优先级、virtio 细节以及更严格的异常行为。

如果你要运行真实系统软件，请先把它当作“足够跑特定实验镜像的模拟器”，而不是 QEMU 的替代品。

## 构建

### macOS / Linux

需要系统 C 编译器和 `make`：

```sh
make
```

构建产物是 `rv-emulator`。

### Windows

仓库包含 Visual Studio 工程和解决方案文件，可以用 Visual Studio 打开并生成。

## 运行

模拟器通常需要传入一个二进制镜像参数，也可以额外传入一个 DTB 参数：

```sh
./rv-emulator <binary-image> [dtb]
```

常用启动命令：

```sh
make sim
```

等价于：

```sh
./rv-emulator tests/localbuild.bin tests/64mb.dtb
```

也可以直接运行其他仓库内置镜像：

```sh
./rv-emulator tests/linux32-nommu.bin
./rv-emulator tests/firmware.bin
./rv-emulator tests/t1.bin
```

如果不传镜像参数，程序会尝试加载默认文件 `asm.o`。当前仓库没有提供这个默认文件，因此通常应显式传入镜像路径。

## 启动约定

- 初始 PC：`0x80000000`
- 模拟内存：64 MiB
- 初始特权级：Machine mode
- `a0`：hart ID，当前写入 `0`
- `a1`：DTB 在客户物理地址空间中的地址；只有传入 DTB 时才有实际意义
- 字节序：little-endian

镜像会被原样加载到模拟内存起始位置。客户程序需要按 `0x80000000` 这个执行地址链接或自行处理入口地址。

## 调试 I/O

模拟器保留了一个调试 MMIO 地址，用于测试程序主动输出状态或退出：

- `x5 = 1`：打印总周期数并退出
- `x5 = 10`：打印 PC 和通用寄存器
- `x5 = 11`：按 `x6` 指向的地址打印字符串
- `x5 = 12`：按十六进制打印 `x6`

这套接口只用于本项目测试和调试，不是标准 RISC-V 设备接口。

## 测试套件

仓库包含一个修改过的 RISC-V 指令测试集合。测试套件需要本地安装 `riscv32-unknown-elf` 交叉工具链，并在测试 Makefile 中配置工具链路径。

测试固件的链接地址必须与模拟器启动地址一致；当前测试说明里记录的是 `0x20000000`，而模拟器当前入口是 `0x80000000`。如果你重新生成测试固件，请先检查并统一这两个地址。

生成测试固件后，可以用模拟器运行对应的裸二进制文件。

## 项目材料

仓库还包含若干教学和设计材料：

- RISC-V 模拟器介绍
- 中断介绍
- 虚拟内存介绍
- OS 和芯片设计入门材料
- 设备树源码与 DTB 示例
- 若干手写汇编测试和预编译二进制镜像

## 开发备注

- 主构建目标只依赖 C 源码，没有外部库依赖。
- 代码同时包含 macOS/Linux 和 Windows 的键盘输入处理路径。
- UART 输出直接写到宿主标准输出；UART 输入从宿主标准输入读取。
- timer 使用宿主机微秒时间推进，并由 CLINT 路径产生 timer interrupt。
- virtio block 使用进程内分配的 64 MiB 临时磁盘，不做持久化。

## 许可证

本项目使用 GNU GPL v3，详见 `LICENSE.txt`。
