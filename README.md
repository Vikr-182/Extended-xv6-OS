# Extenstion of xv6 O.S
Intends to add functionality to xv6 and add scheduling policies.

## About xv6 
xv6 is an open-source operating system implemented by the MIT for multiprocessor and RISC-V systems. It contains basic concepts and organization of UNIX implemented like system calls, fork,exec as well as user mode and kernel mode.

## Usage Instructions
To use this clone the original repo - 

``` 
$: git clone https://github.com/Vikr-182/xv6-private
$: git clone https://github.com/mit-pdos/xv6-public
$: cp xv6-private/* xv6-public
$: cd xv6-public
```

Now run make to run xv6 - 
```
make qemu[-nox] [SCHEDFLAG={PBS,FCFS,MLFQ}]
```

