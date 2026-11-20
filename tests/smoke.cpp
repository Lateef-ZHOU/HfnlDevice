#include "HfnlDevice.h"
#include <optional>
#include <span>
#include <vector>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlListReference>
#include <QtQml/QQmlExtensionPlugin>
#include <QDebug>
#include <memory>
#include <QPointer>
#include "DeviceBase.h"
#include "DAQFamily/DAQDevice.h"
Q_IMPORT_QML_PLUGIN(LidarUIPlugin)
Q_IMPORT_QML_PLUGIN(LidarChartsPlugin)
Q_IMPORT_QML_PLUGIN(LidarTaskUIPlugin)
Q_IMPORT_QML_PLUGIN(LidarHfnlDevicePlugin)
struct TestKey : DAQParamsKey { TestKey() {} };
bool verifyDecodedArrays()
{
    HfnlCollector collector(DAQParameters(TestKey{}, 1, {}, {}, {}, false));
    std::vector<unsigned char> raw((HfnlConfig::homSpecElement + HfnlConfig::cdlSpecElement) * 8);
    // HOM: (-3)^2 + 4^2；CDL: 高位标志不属于 42 位功率。
    qToBigEndian<quint64>((quint64(0x1ffffd) << 21) | 4, raw.data());
    qToBigEndian<quint64>(quint64(1) << 63 | 12345, raw.data() + HfnlConfig::homSpecElement * 8);
    qToBigEndian<quint64>(67890, raw.data() + raw.size() - 8);
    const std::span<const unsigned char> bytes = raw;
    if (collector.feed(bytes.first(1024), {})) return false;
    if (!collector.feed(bytes.subspan(1024), {})) return false;
    const auto &hom = *std::get<0>(collector.channels()).spectrum;
    const auto &cdl = *std::get<1>(collector.channels()).spectrum;
    return hom.size() == HfnlConfig::homSpecElement && hom[0] == 25 && hom[hom.size()-1] == 0
        && cdl.size() == HfnlConfig::cdlSpecElement && cdl[0] == 12345 && cdl[cdl.size()-1] == 67890;
}
template<class Channels, std::size_t... I>
Channels sampleChannels(std::index_sequence<I...>)
{
    const auto channel = []<class C>() {
        using S = std::remove_const_t<typename decltype(C::spectrum)::element_type>;
        auto spectrum = std::make_unique<S>();
        std::fill(spectrum->begin(), spectrum->end(), 1.0);
        return C(std::move(spectrum), LinearMapping<0.0, 1.0, S::ColumnAxisType::count>{});
    };
    return std::make_shared<typename Channels::element_type>(channel.template operator()<std::tuple_element_t<I, typename Channels::element_type>>()...);
}
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    if (!verifyDecodedArrays()) return 20;
    QQmlEngine engine;
    bool warned = false;
    QObject::connect(&engine, &QQmlEngine::warnings, &engine, [&](const auto &) { warned = true; });
    {
        QQmlComponent component(&engine);
        component.setData("import Lidar.Hfnl\nHfnlModule {  }", QUrl());
        std::unique_ptr<QObject> page(component.create());
        if (!page) { qCritical() << component.errors(); return 1; }
        const QQmlListReference charts(page.get(), "mainPages");
        const QQmlListReference sections(page.get(), "sections");
        if (charts.count() < 1 || sections.count() < 1) return 2;
        if (page->property("name").toString().isEmpty()) return 3;
        QObject *backend = nullptr;
        int backends = 0;
        for (auto *child : page->children()) {
            if (QByteArray(child->metaObject()->className()) == "HfnlDevice") {
                backend = child;
                ++backends;
            }
        }
        if (backends != 1 || backend->parent() != page.get()) return 5;
        // 设备与采集卡族是同一个对象：family 即后端本身，可交叉转换到纯 C++ 族基。
        if (page->property("family").value<QObject *>() != backend || !dynamic_cast<DAQFamily *>(backend)) return 7;
        // 族信号经页面接线交给图窗；参数类型与图窗入口不符时 QML 会报警告（warned）。
        for (int channel = 0; channel < 2; ++channel) {
            Q_EMIT qobject_cast<HfnlDevice *>(backend)->channelsReady(sampleChannels<HfnlDevice::Channels>(std::make_index_sequence<std::tuple_size_v<typename HfnlDevice::Channels::element_type>>{}));
            HfnlProfiles profiles;
            std::apply([&](auto &...p) {
                ((p = std::make_shared<const std::remove_const_t<typename std::decay_t<decltype(p)>::element_type>>(
                    std::chrono::system_clock::now(), 30.0)), ...);
            }, profiles);
            Q_EMIT qobject_cast<HfnlDevice *>(backend)->profilesReady(profiles);
        }
        const QPointer<QObject> lifetime(backend);
        app.processEvents();
        page.reset();
        if (lifetime) return 6;
    }
    return warned ? 4 : 0;
}
