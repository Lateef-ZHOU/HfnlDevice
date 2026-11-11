#pragma once
#include <optional>
#include <algorithm>
#include <QByteArray>
#include <QList>
#include <QVector>
#include <QtEndian>
#include <QtGlobal> // quint64（谱元素字节）/ qint64 + Q_INT64_C（3D 值域上界）

#include "DAQCollector.h" // DAQCollector（族收集器基）

// ============================================================
// HfnlRunSide.h — 国实卡型号夹的**运行侧**：常量店 + 收集器。
// 文件名不叫 Config：Config 该是纯常量店（云台族的 SqafConfig.h / McztConfig.h 那样），而本文件还装着一个有
// 行为的类；名随其实。内部的 `namespace HfnlConfig` 那一段确实只是常量店。
// 夹内皆可读、夹外不读（界面要的量由官邸以属性转交）。
// 名字带型号前缀而非裸"DeviceConfig"：装配根同 TU 包含多家设备头，裸名 namespace 会静默
// 合并撞名。
//
// **为什么这两样在一起**：常量（判齐阈值 / 谱维度 / 采集参数量程）正是收集器的取材处。
// 收集器和协议常量供官邸与处理工厂共同使用；本文件不反向包含二者。
// 处理、落盘与风廓线由族层按通道规格完成，图表由 QML 创建并接线。
// 本文件对下只依赖 DAQCollector.h / Qt，是型号夹里的叶子。
// ============================================================
namespace HfnlConfig {
    // ── 谱矩阵维度 + 帧内偏移（以 quint64 元素计）────────────────
    static constexpr int homSpecDbin = 80;
    static constexpr int homSpecFreq = 160;
    static constexpr int cdlSpecDbin = 400;
    static constexpr int cdlSpecFreq = 80;
    static constexpr int homSpecElement = homSpecDbin * homSpecFreq;
    static constexpr int cdlSpecElement = cdlSpecDbin * cdlSpecFreq;
    static constexpr int homSpecElementIndex = 0;
    static constexpr int cdlSpecElementIndex = homSpecElementIndex + homSpecElement;

    // ── 物理换算（频点 → 频率、距离门 → 距离）：500 MSPS，HOM 1024 点、CDL 512 点 FFT，波长 1.55 μm；
    // 零风速在频带中心（第 freq/2 个频点）。────────────────────
    static constexpr double homBinMHz = 500. / 1024.;
    static constexpr double cdlBinMHz = 500. / 512.;
    static constexpr double wavelengthM = 1.55e-6; // 两路共用激光
    static constexpr double rangeStartM = 0.; // 两路第 0 门斜距；第 k 门 = rangeStartM + k·门距
    static constexpr double homDBinDist = 150.;
    static constexpr double cdlDBinDist = 60.;

    // ── 帧的字节结构 ────────────────────────────────────────────
    static constexpr int elementByte = sizeof(quint64);
    static constexpr int homSpecByte = elementByte * homSpecElement;
    static constexpr int cdlSpecByte = elementByte * cdlSpecElement;
    static constexpr int allSpecByte = homSpecByte + cdlSpecByte;

    // 型号固定网络参数；用户只选择本机网卡，设备 MAC 显式提供。
    static constexpr auto hostIPv4 = "192.168.0.3";
    static constexpr int hostPort = 8080;
    static constexpr auto deviceIPv4 = "192.168.0.2";
    static constexpr int devicePort = 8080;
    static constexpr char deviceMAC[] = "00:11:22:33:44:55";
    // 读取政策（唤醒间隔 / 内核缓冲）不在这儿：那是族级常量，住 DAQFamily::kPcap*（由本族的流量形状决定）。

    // ── 采集参数默认值/范围 ─────────────────────────────────────────────────
    static constexpr int accumDefault = 1; // 累积时间（秒）——语义见收集器 buildStart 注
    static constexpr int accumMin = 1;
    static constexpr int accumMax = 3600; // 范围待确认

    // ── 绘图显示范围（随卡走：型号内参数归型号夹）──
    // 3D 功率谱值域：HOM 像素 = I²+Q²（I/Q 各 21bit 有符号，|I|,|Q| ≤ 2²⁰）物理最大 ≈ 2.2×10¹²，CDL 像素 42bit
    // ≈ 4.4×10¹²——valMax 统一取整数上界 10¹³；轴量程即谱维度（0..dbin / 0..freq），直取帧常量。
    static constexpr qint64 plt3dValMax = Q_INT64_C(9999999999999);
    // 2D 风速 | CNR 的值轴量程：WS ±60 m/s、CNR ±25 dB 为物理范围（两通道同）。
    // 距离轴量程不设常量：它恒等于「门数 × 门距」，Wind2DPanel 自己推得出来。
    static constexpr int plt2dWsMin = -60;
    static constexpr int plt2dWsMax = 60;
} // namespace HfnlConfig

// 两路通道 HOM（下标 0）、CDL（下标 1）；工厂为每次尝试备齐两路 double，收包线程直接解码，
// 收齐末包即按族标准方法处理。
// 看门狗重试只读出生即定的参数，与收包写缓冲互不干涉。
// 每次启动发送一份 HOM/CDL 数据后硬件停止发送；缺包使本次无法收齐，由看门狗重新发起采集。
class HfnlCollector final : public DAQCollector
{
public:
    // 一帧 = HOM 全部元素后接 CDL 全部元素，每个元素 8 字节大端。
    static constexpr qsizetype FrameBytes =
        qsizetype(HfnlConfig::homSpecElement + HfnlConfig::cdlSpecElement) * HfnlConfig::elementByte;
    // 族模板层按已校验的请求参数直接构造完整实例。
    explicit HfnlCollector(const DAQParameters &parameters)
        : DAQCollector(parameters, HfnlConfig::wavelengthM) {}

    // ── 指示区：命令 / 时限 / 造下一只（[主]，纯函数纪律：同参数同输出）──
    // 启动命令：p0/p1 固定双包，现与参数无关。看门狗换斗重发即再发本双包。
    // **[预留] 累积时间本该编在这里**：parameters.accumSec 的语义是"卡累积这么久再吐一张谱"，那是
    // 启动这次采集的参数。目前卡没实现这个功能（是采集卡落后于我们），故本口暂时不吃它，
    // 它眼下只兑现了 timeoutMs 那一半用途。卡跟上之后改的是 acqCommands，本口签名不变。
    [[nodiscard]] QList<QByteArray> buildStart() const override { return acqCommands(); }
    // 单次数据发送完毕后硬件自动停止；buildStop 沿用基类空表，当前没有额外的 abort 命令。
    // 单次尝试时限 = 累积时间 + 2 s 余量：到点未齐即换新斗重发（重试是 Device 内政，不出协程）。
    [[nodiscard]] int timeoutMs() const override { return (parameters.accumSec + 2) * 1000; }
    // 同参数、独立的初始收集区；只读取原对象的不可变配置。

    // 可变点：吃一口料，只校验元素对齐并按序放置字节；凑齐的那一口解析两路功率谱（构造通道即算径向风与 CNR）。
    // **帧时刻盖在首包，用的是首包的到料时刻**（用户定），而不是"判齐那一拍现读系统时间"——后者
    // 两处都偏晚：
    //   · **偏在链路上**：现读的是"我们处理到这里"的时刻，把内核缓冲 / 定时唤醒 / 线程调度
    //     全算了进去；而 `at` 是网卡驱动在收包中断里盖的章，是本链路最靠前的一份观测。
    //   · **偏在帧内**：收到首包时，这一帧在设备侧**早已成形**——末包只是把它搬完。
    //   判据：**每份观测都只会比事件晚，那就取最早的那一份。**
    // 缺包会使元素数不足，当前尝试不交付；看门狗超时后换新收集器并重发启动命令。
    // **frameTime 不能交给协程生成**（对照 sessionTime）：sessionTime 是编排层的**决定**（"这轮会话
    // 从此刻算起"），任何线程任何时刻定都对；frameTime 是**测量**（"这一帧在 T 开始"），有唯一
    // 正确值，由收包线程交给 Collector。协程恢复在跨线程队列投递之后，那时读表量到的是
    // "主线程什么时候腾出手"——而主线程正是最忙的那个（设备与图表控制都在它上面）。
private: // NVI 可变点：派生只实现、不调用（公开的是基类那张非虚的皮 feed）

    std::optional<std::vector<DAQChannel>> assemble(QByteArrayView bytes, RxTime at) override
    {
        // 硬件保证元素不跨包；非法长度整包拒收，不缓存半个元素，也不部分接收超长包。
        if (bytes.isEmpty() || bytes.size() % HfnlConfig::elementByte != 0 || bytes.size() > FrameBytes - received_)
            return std::nullopt;
        if (received_ == 0)
            frameTime_ = at;
        std::memcpy(raw_.data() + received_, bytes.data(), static_cast<std::size_t>(bytes.size()));
        received_ += bytes.size();
        if (received_ != FrameBytes)
            return std::nullopt;
        auto channels = decode();
        raw_ = QByteArray();
        return channels;
    }

    // HOM 元素为 21 位有符号 I/Q，功率 = I² + Q²（先整数求和再精确转 double）；CDL 元素低 42 位即功率。
    std::vector<DAQChannel> decode() const
    {
        const char *const bytes = raw_.constData();
        const auto word = [bytes](const int i) { return qFromBigEndian<quint64>(bytes + i * HfnlConfig::elementByte); };
        LinearArray2D<double> hom(HfnlConfig::homSpecDbin, HfnlConfig::homSpecFreq, HfnlConfig::rangeStartM,
                                  HfnlConfig::homDBinDist, 0.0, HfnlConfig::homBinMHz);
        hom.reserve(HfnlConfig::homSpecElement);
        for (int i = 0; i < HfnlConfig::homSpecElement; ++i)
        {
            const quint64 w = word(i);
            const qint64 re = static_cast<qint64>(((w >> 21) & 0x1FFFFFULL) ^ 0x100000ULL) - 0x100000;
            const qint64 im = static_cast<qint64>((w & 0x1FFFFFULL) ^ 0x100000ULL) - 0x100000;
            hom.push_back(static_cast<double>(re * re + im * im));
        }
        LinearArray2D<double> cdl(HfnlConfig::cdlSpecDbin, HfnlConfig::cdlSpecFreq, HfnlConfig::rangeStartM,
                                  HfnlConfig::cdlDBinDist, 0.0, HfnlConfig::cdlBinMHz);
        cdl.reserve(HfnlConfig::cdlSpecElement);
        for (int i = 0; i < HfnlConfig::cdlSpecElement; ++i)
            cdl.push_back(static_cast<double>(word(HfnlConfig::homSpecElement + i) & 0x3FFFFFFFFFFULL));
        std::vector<DAQChannel> channels;
        channels.reserve(2);
        channels.emplace_back(QStringLiteral("HOM"), std::move(hom), HfnlConfig::homSpecFreq / 2 * HfnlConfig::homBinMHz,
                              wavelengthM());
        channels.emplace_back(QStringLiteral("CDL"), std::move(cdl), HfnlConfig::cdlSpecFreq / 2 * HfnlConfig::cdlBinMHz,
                              wavelengthM());
        return channels;
    }

    QByteArray raw_{FrameBytes, Qt::Uninitialized}; // 原样帧：HOM 元素后接 CDL 元素；解析后释放
    qsizetype received_ = 0;                        // 已收字节，凑满 FrameBytes 即齐；可在单包内跨 HOM/CDL 分界

private:
    // ── 采集启动命令（p0/p1 双包）──
    // 字节序列按位保真搬自 V6 的 DAQNPCapture::SendStartReceive（现场标定的魔法字节，含义
    // 未逆向；两包仅第 12/13 字节不同：{32,6} vs {54,0}）。此处只出 payload——全信封由
    // PcapHandle::onTX 组包。
    [[nodiscard]] static QList<QByteArray> acqCommands()
    {
        static constexpr quint8 p0[] = {0, 16, 103, 0, 177, 134, 170, 0, 204, 221, 238, 170, 32, 6, 0,
                                        0, 0,  0,   0, 0,   0,   0,   0, 0,   0,   0,   0,   0,  0};
        static constexpr quint8 p1[] = {0, 16, 103, 0, 177, 134, 170, 0, 204, 221, 238, 170, 54, 0, 0,
                                        0, 0,  0,   0, 0,   0,   0,   0, 0,   0,   0,   0,   0,  0};
        return {QByteArray::fromRawData(reinterpret_cast<const char *>(p0), sizeof p0),
                QByteArray::fromRawData(reinterpret_cast<const char *>(p1), sizeof p1)}; // 静态存储期，零拷贝视图安全
    }
};

Q_DECLARE_METATYPE(std::shared_ptr<const HfnlCollector>)
