# OSInfo Base Tools

五个独立项目：coreutils/（含 tar）、sysinfo/、mount/（含 umount）、mkfs/、systemctl/。每个目录各有源码、infoide-project.json、调试配置、Makefile 和自己的 build/，通过已安装的 InfoIDE SDK 编译，无需内核源码。

```sh
make -j
# 或显式选择 SDK：make -j OSINFO_SDK=/absolute/path/to/sdk
```

项目根目录使用 ./infoide 二进制，已加入 .gitignore。先在 Linux 安装 InfoIDE 的完整发行包即可自动定位 SDK；各编译参数由各自 JSON 决定。

make 先在各项目 build/packages/ 生成 coreutils.inpk、sysinfo.inpk、mount.inpk、mkfs.inpk、systemctl.inpk，再用 C++ InfoIDE 的 bundle 命令汇总到 build/osinfo-base-tools.zip。单独构建可 make -C coreutils 或 make -C systemctl；不再生成单体 osinfo-base-tools.inpk。

systemctl 通过服务管理 IPC 提供 list/status/start/stop/restart/enable/disable，修改操作验证有效 UID。普通 kill、killall、pkill 不能终止服务。ps/pkill/killall/who 从 /proc 读取；ps 支持默认、-a、-x、-e/-A、-ef 和 -t TTY。批量信号先收集目标，再发送，避免把新恢复的 shell 再次选入。进程文件与服务协议升级后，必须用同一版 SDK 重新构建全部项目。

将 ZIP 放进 OS 仓库 packages/ 后执行 ./configure。configure 直接解压并逐包校验，OS 主构建只消费包。sysinfo 从 FS 获取版本和 hostname；版本文件和动态链接器属于系统基础镜像。

coreutils/archive/gzip.h 随本项目保留，tar 不依赖相邻 inpkg 或 OS 私有源码。现有登录、sudo、凭据与信号行为保持原有实现。
