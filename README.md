### 简介
- 进程监视器，用于监控和管理进程。它允许您设置进程的优先级、相关性以及是否启用效能模式。
- 支持 Windows 10、Windows 7、Windows 8.1 和 Windows 8。
- 优化某讯的反作弊软件进行优化，避免影响到游戏开启rtss锁帧依旧掉帧。

### 功能
- 设置进程的优先级（idle, below, normal, above, high, realtime）
- 设置进程的相关性掩码 (十六进制或last4)

### 打包说明
```shell

# 构建工程
xmake config --mode=debug

# 编译工程
xmake build -v

# 管理员运行
.\ProcessModifier.exe SGuard64.exe idle last3 --eco

.\ProcessModifier.exe SGuardSvc64.exe idle last3 --eco

```

### 使用方法
双击run.bat文件即可运行程序。
