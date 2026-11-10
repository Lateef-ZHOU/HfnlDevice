# HfnlDevice

2026-11-10 用户定（收包与解析分离）：收包只校验 8 字节元素对齐并按序 `store`；`decode` 按 HOM（21 位 I/Q 求功率）与 CDL（低 42 位）两段换算。

2026-11-10 用户定（二维线性数组）：两路通道规格给出功率谱网格（门距、频点宽度）；页面功率谱显示真实斜距与频率。

2026-11-10 用户定（波长与抽稀）：两路共用 `wavelengthM = 1.55 μm` 交族基，删除逐通道换算系数。

2026-11-10 用户定（线性数组）：通道规格以 `radialWind = cnr = LinearArray<double>(起点, 门距)` 给出距离轴；`radialReady` 发线性数组，页面原样交给径向风图。

2026-11-10 用户定：通道规格改为起点 + 步长（第 0 门在 0 m），`radialReady` 不带斜距、`profileReady` 发均匀风廓线；页面统一用 `Wind2DView`（配置含 `rangeStart`）与 `WindBarbItem`。页面测试补充族信号经页面交给图窗不出 QML 类型警告的检查。同时修正 2026-11-09 DAQ 第一步引入的回归：族层当时发非均匀风廓线，本页均匀 `WindBarbItem` 收不进（风羽图无数据）；通道斜距也由原 (k+1)·门距 变成 k·门距。现按用户定统一为第 0 门在 0 m。

2026-11-09 用户定（DAQ 第二步）：Collector 的 createPrototype（只读原型）改为 `create(const DAQParameters&)`，按请求参数造完整实例并备齐收集区；makeSelf 只供看门狗重试，复用 create。

2026-11-09 用户定：删除处理线程、中间 Core 与型号处理 lambda；末端直接继承 `DAQDevice<HandleT, CollectorT>`，唯一 QObject 来自 Handle。Collector 继承族基的通道数组，型号只给通道规格并按协议整理功率谱，收齐末包调用 complete() 由族基在收包线程做标准处理；显示、落盘（每帧一个 JSON）与风廓线由族层完成。页面改接族信号 spectrumReady/radialReady/profileReady（按通道下标），family 改为 QtObject，删除 device.device 自引用。通道 0 = HOM（80 门 × 160 点、150 m），1 = CDL（400 门 × 80 点、60 m）；频点宽 500/1024、500/512 MHz，零风速在频带中心，0.775 m/s/MHz，换算与原斜率截距一致。删除 HfnlProcessing.h。验证：独立构建零警告，CTest 1/1。

2026-11-03 用户定：网口连接区仅保留网卡选择和连接/断开，删除 IP、端口等固定参数展示及对应 QML 参数字段；实际连接配置、校验、ARP 和收发关闭行为保持。

Windows 验证：通过主程序本地源码覆盖增量构建，五个网口官邸 qmllint 目标成功；仍有 QObject 属性类型的既有静态提示。八秒离屏冒烟通过，仅既有字体提示；未进行硬件操作或 Linux 验证。证据：LiDAR_DataACQ/build/build-network-ui.log、smoke-network-ui.json。

2026-11-03 上传接口：官邸仍拥有唯一具体 Core；Core 继承 `Device<HandleT, CollectorT>`，Device 直接继承 Handle 并重写 onData。Core 只提供 Qt 属性、锁信号与末端预析构；不增设上传回调、状态缓存或逐包信号转发。协议、Collector 与 UI 逻辑保持。

采集图窗统一使用 `HOM·功率谱`、`CDL·功率谱`、`HOM·径向风`、`CDL·径向风`、`HOM·风廓线`、`CDL·风廓线`；径向风页继续包含风速和 CNR。仅调整显示名称，页面顺序、历史与处理/保存接口保持。


2026-10-28 用户定：连接区只选择网卡，`open(interfaceIndex)` 使用 HfnlConfig 固定参数：本机 `192.168.0.3:8080`，设备 `192.168.0.2:8080`，设备 MAC `00:11:22:33:44:55`。QML 不再保存或编辑 IP、端口与 MAC；Handle 仍持本次连接快照并负责重连，固定 MAC 沿用直接发送 UDP 的路径。

本轮 Windows Qt 6.11.2 Release、qmllint 及既有页面冒烟 1/1 通过，无编译/QML 警告；证据为 `build/build-fixed-connection.log`、`build/test-fixed-connection.log`。未连接实物，不重复 Linux/协议全套，仅本地提交。

LiDAR 的独立官邸：具体 C++ 后端、Collector/协议/处理逻辑及其 QML 页面在同一仓库维护。来自 LiDAR_DataACQ master `5cc6d65`，本次分仓不改变运行行为。

## 边界与接入

- QML 模块：`Lidar.Hfnl`；页面：`HfnlModule`。
- CMake 目标：`Lidar::HfnlDevice`；静态插件：`LidarHfnlDeviceplugin`，最终可执行文件显式 `Q_IMPORT_QML_PLUGIN(LidarHfnlDevicePlugin)`，并导入 TaskUI/UI/Charts 的静态插件。
- 页面以 attached 对象拥有唯一具体后端，后端不能在 QML 直接创建。设备通过只读 `family` 借出族引用；任务页面只接收族列表，不依赖设备官邸。
- 依赖由 `cmake/Dependencies.cmake` 固定到提交；可用标准 FetchContent 源码覆盖做联调。多个官邸并用时，先开启所有需要的 HandleCore/TaskCore 可选项，再配置共享库，避免已创建目标后补开模块。
- 需要 libpcap >= 1.10.0；Windows 使用 Npcap SDK 与已安装的 Npcap 运行时，Linux 使用 libpcap。驱动不随仓库分发。

## 构建与验证

C++20、CMake >= 3.28、Qt 6.11.2（Quick/Qml/GuiPrivate/ShaderTools）。Windows 使用 MSVC x64；Linux 使用 GCC，Qt 与原生依赖由环境提供。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<Qt前缀;需要的原生依赖前缀>"
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target LidarHfnlDevice_qmllint
```

独立测试逐头编译、创建真实页面并销毁后端；不连接实物设备。硬件收发仍需对应设备实测。主程序负责跨官邸仿真、任务及取消链回归。

## 许可

MIT，Copyright (c) 2026 周涛（ZHOU TAO）。Qt、libserialport、libpcap/Npcap 等依赖各自授权，MIT 不替代这些许可；详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。本仓库不包含第三方 SDK、运行库或驱动。

CI：托管 Linux 自动验证；Windows 原生依赖使用预装 SDK/运行时的自托管入口，默认跳过不代表通过。本机 Windows 验证记录与主程序分仓记录一起保存。

本轮本机验证（2026-10-28）：Windows Qt 6.11.2 Release 构建、CMake qmllint 和既有页面冒烟 1/1 通过；未连接实物。日志：build/build-connection-summary.log、build/test-connection-summary.log。

## 资源阶段通知（2026-11-02）

连接采用 HandleCore 的 `connectionChanged(ResourcePhase)`；主动关闭前在所属线程同步发 `AboutToRelease`，完成事件用于回读现状。页面借出已有 `deviceState` 机制对象，不再保存 isOpen/isLocked/holder 的 QML 转交属性；标签通过连接/锁完成阶段回读，不恢复通用 statusChanged。固定依赖已同步；本轮提交尚未推送，验证使用显式本地源码覆盖。


2026-11-03 验证：Windows Qt 6.11.2 主程序 Release 编译、15 组 QML 检查、跨官邸生命周期和三任务回归 7/7 通过。接口联调使用相邻源码覆盖，新依赖固定为本地提交，尚未推送；未连接实物设备或更新板卡。

## 源码目录（2026-11-03）

公开文件直接放在 `include/`，引用时使用文件名，不再携带原单仓库的业务目录前缀。CMake 目标名称、C++ 类型和 QML 模块名保持不变；使用方须同步更新头文件路径及固定依赖版本。

目录整理验证（2026-11-03）：Windows：公开头独立编译、页面测试 1/1 及主程序构建通过。本轮通过本地源码覆盖验证，固定依赖同步更新到本地提交；尚未推送，不代表已验证远端拉取。

## 2026-11-04：设备日志直接接线（用户定）

删除具体设备的 logSource 自引用属性，QML 直接监听现有设备对象的 logMessage/connectionChanged；设备失败日志直接调用继承自 Handle 的信号。协议、收发、重连和关闭顺序不变。 本轮仅本地整理，未推送；Windows 集成验证记录见主程序维护操作。

## 2026-11-04：设备身份与占用查询（用户定）

设备名称/副标题由构造传入 DeviceBase 的公开 const 成员，Qt 通过 MEMBER CONSTANT 直接暴露；占用查询只保留 holder，Qt 用无效 QVariant 表示空闲、字符串表示占用，删除 isLocked 属性。既有连接、重连、收发和取消逻辑保持。

本轮仅本地整理；集成验证记录见主程序 docs/maintenance/validation.md，不推送。

## 2026-11-04：进料在消费端取值（用户定）

Collector 的 onFeed 接收 optional<QByteArray> 的 const 引用，由本型号解析端取出字节。空输入由 DeviceCore 公共 feed 拦截；HOM/CDL 解析及收齐规则不变。

本轮仅本地整理，不推送；集成验证见主程序维护记录。


### 2026-11-05 公共视觉与连接布局

网口连接区使用 UICore ConnectionSection，仅收拢布局，枚举、选择、命令及日志仍归官邸。 本轮仅本地整理，未推送；Windows 集成验证记录见主程序维护操作。
