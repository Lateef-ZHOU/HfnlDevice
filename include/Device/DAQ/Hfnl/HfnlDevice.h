#pragma once
#include "Device/DAQ/Hfnl/HfnlProcessing.h"
#include <QDateTime>
#include <QtQml/qqmlregistration.h>
#include "UI/Chart/DAQChartSpecification.h"
#include "Acquisition/AcqSaver.h"
#include "Algorithm/Wind/VerticalWindProfile.h"
#include <memory>
#include <QString>
#include <QUrl>
#include <QVariantMap>

#include "Device/DAQ/DAQDevice.h"                  // 采集卡族两层（族基 DAQDeviceBase + 族层 DAQDevice）
#include "Device/PcapForm.h"
#include "Device/DAQ/Hfnl/HfnlRunSide.h"        // 运行侧：常量 + 收集器

// 国实官邸：组装型号命令与页面参数，并在 run 中创建双通道处理闭包。
// 结果直接发本对象信号；处理状态归闭包，图表与可编辑设置归 QML。
class HfnlDevice final
#ifdef Q_MOC_RUN
    : public DAQDeviceBase // 模板层继承的族级元对象。
#else
    : public DAQDevice<PcapHandle, HfnlCollector>
#endif
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap connectionParameters READ connectionParameters CONSTANT)
    QML_ELEMENT
    QML_ATTACHED(HfnlDevice)
    QML_UNCREATABLE("由对应 QML 官邸创建和拥有")
    // ── QML 面：连接、控制与绘图区按官邸模板装配 ──
    // 型号范围与初值交给 QML；可编辑设置由 QML 官邸持有，启动时交入。
    Q_PROPERTY(QVariantMap acquisitionParameters READ acquisitionParameters CONSTANT)

public:
    static HfnlDevice *qmlAttachedProperties(QObject *owner) { return new HfnlDevice(owner); }
    [[nodiscard]] QVariantMap connectionParameters() const
    {
        return PcapForm::connectionParameters(HfnlConfig::hostIPv4, HfnlConfig::hostPort,
                                                HfnlConfig::deviceIPv4, HfnlConfig::devicePort, HfnlConfig::deviceMAC);
    }

    // 图表规格从 HfnlConfig 取得，QML 据此装配；绘图器不认识设备型号。
    explicit HfnlDevice(QObject *parent)
        : DAQDevice(QStringLiteral("国实采集卡"), QStringLiteral("网口 · UDP · HOM|CDL"), parent)
    {
        setObjectName(QStringLiteral("Hfnl")); // ASCII 标识：落盘会话目录名用

        startProcessing();
    }

    ~HfnlDevice() override { PreDestruction(); }

    [[nodiscard]] QVariantMap acquisitionParameters() const { return {{"accumMin", HfnlConfig::accumMin}, {"accumMax", HfnlConfig::accumMax},
                 {"accumDefault", HfnlConfig::accumDefault}, {"saveDirDefault", QUrl::fromLocalFile(QDir::currentPath())}}; }

    Q_INVOKABLE [[nodiscard]] QVariantMap chartParameters() const
    {
        const auto homPlotConfiguration = DAQChartSpecification::make(
            HfnlConfig::homSpecDbin, HfnlConfig::homSpecFreq, HfnlConfig::homDBinDist,
            HfnlConfig::plt2dWsMin, HfnlConfig::plt2dWsMax, HfnlConfig::plt3dValMax);
        const auto cdlPlotConfiguration = DAQChartSpecification::make(
            HfnlConfig::cdlSpecDbin, HfnlConfig::cdlSpecFreq, HfnlConfig::cdlDBinDist,
            HfnlConfig::plt2dWsMin, HfnlConfig::plt2dWsMax, HfnlConfig::plt3dValMax);
        return {{"homPlotConfiguration", homPlotConfiguration},
                 {"cdlPlotConfiguration", cdlPlotConfiguration}};
    }

Q_SIGNALS:
    void homSpecReady(const QVector<double> &values);
    void cdlSpecReady(const QVector<double> &values);
    void homWSReady(const QVector<double> &values);
    void homCNRReady(const QVector<double> &values);
    void cdlWSReady(const QVector<double> &values);
    void cdlCNRReady(const QVector<double> &values);
    void homProfileReady(VerticalWindProfilePtr profile);
    void cdlProfileReady(VerticalWindProfilePtr profile);

public:
    // **本卡没有参数下发口**：参数下行协议目前未知，真码逆向出来之前不开这口，也不留半截接线
    // （`0xAA + 命令字 + 大端 quint16` 那套是**国耀卡的**报文格式，别抄过来）。
    // **别把两件事混为一谈**：累积时间（accumSec）该走的是**启动命令**而不是参数下发——它是
    // "启动这次采集"的参数，本该编进 buildStart，只是目前卡未实现（见 HfnlRunSide 的 accumSec_ 注）。

private:

    void run() override { consumeFrames(makeHfnlProcessor(this)); }

    void PreDestruction() noexcept final
    {
        static_assert(FinalDevice<HfnlDevice, decltype(&HfnlDevice::PreDestruction)>);
        DAQDevice::PreDestruction();
    }

// **动作对 QML 公开**：那两份 .qml 直接按名调这些槽
// （按名绑定的耦合关在型号夹内部——QML 与官邸同作者、同目录、一起改）。
public Q_SLOTS:
    bool open(const int interfaceIndex, const QString &hostIP, const int hostPort,
              const QString &deviceIP, const int devicePort, const QString &deviceMAC)
    {
        const auto params = PcapForm::parseParameters(this->device().logSource(), interfaceIndex, hostIP, hostPort, deviceIP, devicePort,
            deviceMAC, DAQDevice::kPcapReadTimeout, DAQDevice::kPcapKernelBufferBytes);
        return params && this->device().open(*params);
    }
    void close() { this->device().close(); }
    // 模板族层不能声明元对象；本类仅提供 QML 调用所需的槽入口。
    void startTest(const int accumSec, const bool save, const QUrl &folder)
    {
        DAQDevice::startTest(accumSec, save, folder, AcqSaver::sessionDir(
            QDateTime::currentDateTime(), this->objectName() + QStringLiteral("Test")));
    }
    void cancelTest() noexcept { DAQDevice::cancelTest(); }

};
