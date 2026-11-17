## 2026-11-17：命令集合使用 vector（用户定）

Collector::buildStart/acqCommands 与设备 stopCommands 统一为 std::vector<std::vector<unsigned char>>。两条启动命令从原静态字节表直接构造自有数组，顺序和字节不变；仍无额外停机命令，原有超时与收包行为保持。

不新增命令缓存或兼容包装。验证见主程序 docs/maintenance/validation.md；使用相邻源码覆盖，暂不推送。

## 2026-11-14：UTF-8 日志接口（用户定）

准入固定日志使用 UTF-8 std::string，累积范围动态日志改为 std::format；通过 UICore 接入既有 QML 日志栏，不增加官邸转发。协议、规格和处理链不变。

需要支持 C++20 std::format 的标准库。本轮仅本地修改与验证，不推送。

验证：主程序全部官邸增量构建及 8 秒离屏冒烟通过；现有生命周期回归 8/8 通过。本轮未做实物采集。

## 2026-11-14：清理 Collector 元类型登记（用户定）

删除无用户的 Collector 只读份额 Q_DECLARE_METATYPE 声明；Collector 仅在 C++ 收集、协程及四束历史中流转。通道结果及风廓线的 Qt/QML 登记保持，算法、保存格式、取消链与数据保活方式不变。曲面轴配置和谱页保活范围暂缓；本轮不推送。

验证：主程序原 build Windows/MSVC Release 增量构建通过，覆盖本官邸的元对象、QML 类型登记及链接；8 秒离屏存活冒烟通过，stderr 为空。仅本地验证，未连接实物。证据见 LiDAR_DataACQ/build/collector-metatype-build.log 与 collector-metatype-smoke.{out,err,json}。

## 2026-11-14：静态功率谱、径向风与 CNR（用户定）

HOM/CDL 解码直接填写各自 Spectrum<Spec> 的最终数组；DAQChannel 构造径向风及独立 CNR。channelsReady 经只读共享通道元组连接对应的具体绘图器，Qt 登记留在本库；不再构造动态谱/剖面或逐帧复制轴。大端读取、平方转换、收包/判齐、四束历史和保存格式保持。

本轮仅本地联调，不推送；与配套 FamilyCore、ChartCore 及官邸源码一起使用。

验证：Windows/MSVC Release 构建及 CTest 1/1 通过；补测实际 HOM/CDL 分段收包、大端 I/Q 解码与最终固定数组位置，QML 页面和静态通道信号通过。证据：build-daq/static-frames*.log 与 Testing/Temporary/LastTest.log。

## 2026-11-14：接入 FamilyCore（用户定）

依赖声明与开关改用 FamilyCore / FAMILYCORE_WITH_*，协程头由 DeviceCore 提供。当前仅本地整理，GitHub 仍为 TaskCore，旧固定提交暂不更新；联调使用 `FETCHCONTENT_SOURCE_DIR_LIDAR_FAMILYCORE` 指向相邻 FamilyCore，并覆盖配套 DeviceCore/Algorithm 等源码。Windows 主程序或 simonly 的实际消费目标构建通过，业务与数据格式不变；未推送、未更新交付包。

## 2026-11-14：删除旧风场测试依赖

Windows/MSVC Release 页面目标构建与页面测试 1/1 通过。

配合 Algorithm 删除 WindField.h，测试改用标准风廓线帧或删除无用引用；本库生产、保存与取消逻辑不变。不推送。

## 2026-11-14 风廓线原帧直接传输（用户定）

HOM、CDL 的 profilesReady 直接携带各规格 VerticalWindProfileUnit 的只读份额元组。两张具体风羽图只接管自己的通道，不持有整组通道；Qt 类型登记和末端入口留在本仓库。四束、算法与保存格式保持。Windows/MSVC Release 构建及页面测试 1/1 通过。 本轮不推送。

# HfnlDevice

## 风廓线类型入口（2026-11-13 用户定）

改从 Algorithm 的 `WindProfile.h` 引入垂直风廓线整帧类型，跟随删除独立旧头。采集、反演、保存及四束编排行为不变。


2026-11-13 用户定：通道规格的距离门数量与频点数量分别命名为 `rangeGateCount`、`frequencyBinCount`，取代 `rows`、`columns`。同步规格使用方，数值与数据布局不变；通用二维传输对象和 SpectrumLike 的行列接口保持。

2026-11-10 用户定（凑帧归官邸）：官邸持有原样帧缓冲与已收字节，`assemble` 校验元素对齐与剩余容量后放置字节，凑满即按 HOM/CDL 换算，解析后释放缓冲。

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

验证：MSVC Release、公开头编译与 CTest 1/1 官邸冒烟通过。 使用本地依赖覆盖，未推送。

## 边界与接入

- QML 模块：`Lidar.Hfnl`；页面：`HfnlModule`。
- CMake 目标：`Lidar::HfnlDevice`；静态插件：`LidarHfnlDeviceplugin`，最终可执行文件显式 `Q_IMPORT_QML_PLUGIN(LidarHfnlDevicePlugin)`，并导入 TaskUI/UI/Charts 的静态插件。
- 页面以 attached 对象拥有唯一具体后端，后端不能在 QML 直接创建。设备通过只读 `family` 借出族引用；任务页面只接收族列表，不依赖设备官邸。
- 依赖由 `cmake/Dependencies.cmake` 固定到提交；可用标准 FetchContent 源码覆盖做联调。多个官邸并用时，先开启所有需要的 HandleCore/FamilyCore 可选项，再配置共享库，避免已创建目标后补开模块。
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


## 2026-11-13：不可变墙钟风廓线（用户定）

页面验证的风廓线夹具适配不可变整帧及共享消费视图；生产反演由 FamilyCore 按 HOM/CDL 的静态通道规格构造，协议与处理行为保持。

本轮使用本地源码覆盖验证，尚未推送；具体集成验证见主程序维护操作。

## 2026-11-13：大数组固定对象与只读传输

HOM/CDL 的功率谱、径向风和 CNR 信号与 DAQ 族统一改传只读共享数组份额，页面原样转交图表。原数据布局、换算、保存与收包流程不变。

本轮使用本地源码依赖验证；上游及本库暂不推送，远端固定版本未更新。

2026-11-13 本轮验证：Windows/MSVC Release 构建及公开头独立编译通过；build-daq 中 CTest 1/1 通过。使用本地源码依赖，未验证 Linux 或远端拉取；暂不推送。

