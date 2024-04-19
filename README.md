# la32r-Linux

此内核基于 Linux 5.14.0-rc2 移植而来，支持 [LoongArch 32 Reduced（la32r）](https://www.loongson.cn/uploads/images/2023041918122813624.%E9%BE%99%E8%8A%AF%E6%9E%B6%E6%9E%8432%E4%BD%8D%E7%B2%BE%E7%AE%80%E7%89%88%E5%8F%82%E8%80%83%E6%89%8B%E5%86%8C_r1p03.pdf)指令集架构。目前，该内核支持在 la32r 的 3 个不同平台上运行：[la32r-QEMU](https://gitee.com/loongson-edu/la32r-QEMU)，[chiplab/loongson-soc](https://gitee.com/loongson-edu/chiplab) 和全流程平台参考 SOC。

## 内核编译

1. 克隆仓库

    ```shell
    git clone https://gitee.com/loongson-edu/la32r-Linux.git --depth=1
    cd la32r-Linux
    ```

2. 配置环境变量（假设交叉工具链安装在了 `/opt/loongson-gnu-toolchain-8.3-x86_64-loongarch32r-linux-gnusf-v2.0/` 下）

    ```shell
    export PATH=/opt/loongson-gnu-toolchain-8.3-x86_64-loongarch32r-linux-gnusf-v2.0/bin:$PATH
    export CROSS_COMPILE=loongarch32r-linux-gnusf-
    export ARCH=loongarch
    ```

3. 配置内核编译选项，**3 个平台的差异通过此步区分**

    > - la32r-QEMU 与 chiplab/loongson-soc 使用的是完全相同的代码，不同的是 la32r-QEMU 平台只模拟了 uart 这一个外设。
    > - 这 3 个平台的差异实际上是通过 `arch/loongarch/boot/dts/loongson/loongson32_*.dts` 区分的，查看这些 dts 文件，可以进一步了解不同平台的外设地址空间差异。
    > - chiplab/loongson-soc 和全流程平台参考 SOC 均支持 nand flash，但是在他们的 dts 文件中，描述 nand 的结点均被 `#if 0` 注释掉了，如需启用，将 `#if 0` 和 `#endif` 删除即可。
    > - 全流程平台参考 SOC 支持 SD 卡，但是在 `la32_bx_defconfig` 中默认关闭了 SD 卡驱动，如需启用需自行开启。

    |平台|默认配置|配置文件|
    |:---:|:---:|:---:|
    |la32r-QEMU| `make la32_defconfig` | `arch/loongarch/configs/la32_defconfig` |
    |chiplab/loongson-soc| `make la32_defconfig` | `arch/loongarch/configs/la32_defconfig` |
    |全流程平台参考 SOC| `make la32_bx_defconfig` | `arch/loongarch/configs/la32_bx_defconfig` |

4. 配置根文件系统路径

    la32r-Linux 在 3 个平台中均采用 ramfs，编译好的根文件系统在内核构建过程中直接被链接在内核中，因此需要在编译内核前对根文件系统的路径进行配置。

    > 编译根文件系统可使用 [la32r-buildroot](https://gitee.com/loongson-edu/la32r-buildroot)

    配置根文件系统路径的方式有 2 种：

    - 修改 .config 文件中的 `CONFIG_INITRAMFS_SOURCE` 参数，例：`CONFIG_INITRAMFS_SOURCE="~/linux-5.14-loongarch32/initrd_pck32"`
    - 在 `make menuconfig` 中 `General Setup -> Initramfs source file(s)` 修改

5. 编译内核，内核的 ELF 文件为当前目录下的 `vmlinux`

    ```shell
    make -j`nproc`
    ```

6. 裁剪 `vmlinux`，将其中多余的符号信息去除，以减小内核体积

    ```shell
    loongarch32r-linux-gnusf-strip vmlinux
    ```

> 注：如果想采用自动化编译流程，可参考 `la_build.sh`，**其中需要自行配置 `CROSS_COMPILE` 变量，并在打开 `menuconfig` 后自行修改根文件系统目录**，生成的 ELF 文件为 `la_build/vmlinux`。
