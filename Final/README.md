
# FudanCompilerH2025

## 环境配置

```bash
sudo apt install -y gcc g++ cmake ninja-build flex bison clang-14 llvm-14 g++-arm-linux-gnueabihf gcc-arm-linux-gnueabihf qemu-user zip


```

##



1. HW5: .fmj -> .2.ast -> .2-semant.ast -> .3.irp
2. HW6: .3.irp -> .3-canon.irp -> .4.quad
3. HW7: .4.quad -> .4-block.quad -> .4-ssa.quad
4. HW10: .4-ssa.quad -> .4-ssa-opt.quad
5. HW8: .4-ssa.quad -> .4-prepared.quad(.4-xml.clr)
6. HW9: -> .s