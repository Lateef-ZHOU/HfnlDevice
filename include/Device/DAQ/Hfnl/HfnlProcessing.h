#pragma once
#include <array>
#include <memory>
#include <QString>
#include <QVector>
#include "Algorithm/Wind/RadialWind.h"
#include "Algorithm/Wind/WindProfile.h"
#include "Acquisition/AcqSaver.h"
#include "Device/DAQ/Hfnl/HfnlRunSide.h"

// 型号帧处理工厂；Emitter 只提供结果信号，不向其读取或写入业务成员。
// 每条处理线程只创建一份；四束环及写位置仅由该闭包维护。
// 任务保证首次 sync 前写满四束，后续同步点仍使用最近四个互异方向。
template<class Emitter>
auto makeHfnlProcessor(Emitter *self)
{
    return [self, homRing_ = std::array<WindProfile::Input, WindProfile::kBeams>{}, cdlRing_ = std::array<WindProfile::Input, WindProfile::kBeams>{}, ringW_ = 0]
        (const std::shared_ptr<const HfnlCollector> &collector) mutable {
        if (!collector || !collector->settled())
            return; // 只处理完整帧
        const auto &[pose, saveRoot, sessionDir, sync] = collector->context();
        const auto &[homSpec, cdlSpec] = collector->constFrame();

        // 目录名由编排层给（一组一个会话目录，同步之后换新的）；文件名来自这一帧的产物。
        const QString fileStem = AcqSaver::fileStem(collector->frameTime());

        // 1. HOM 3D 谱 → 推 GUI → 异步落盘（立刻起线程，不等后续计算）
        // **发出后本数组只读**：GUI 的 3D 模块直接接住这份份额、不深拷，故之后径向风计算与落盘
        // 都只许读它。
        Q_EMIT self->homSpecReady(homSpec);
        if (saveRoot)
            AcqSaver::saveSpec(*saveRoot, sessionDir, fileStem, homSpec, QStringLiteral("HOM"));

        // 2. CDL 3D 谱
        Q_EMIT self->cdlSpecReady(cdlSpec);
        if (saveRoot)
            AcqSaver::saveSpec(*saveRoot, sessionDir, fileStem, cdlSpec, QStringLiteral("CDL"));

        // 3. HOM 2D 风速 + CNR
        const auto [homWS, homCNR] = RadialWind::Calc(homSpec, HfnlConfig::homSpecDbin, HfnlConfig::homSpecFreq,
                                                     HfnlConfig::homFreq2WsSlope, HfnlConfig::homFreq2WsIntercept);
        Q_EMIT self->homWSReady(homWS);
        Q_EMIT self->homCNRReady(homCNR);

        // 4. CDL 2D 风速 + CNR
        const auto [cdlWS, cdlCNR] = RadialWind::Calc(cdlSpec, HfnlConfig::cdlSpecDbin, HfnlConfig::cdlSpecFreq,
                                                     HfnlConfig::cdlFreq2WsSlope, HfnlConfig::cdlFreq2WsIntercept);
        Q_EMIT self->cdlWSReady(cdlWS);
        Q_EMIT self->cdlCNRReady(cdlCNR);

        // 5. 落盘径向风
        // 通道顺序 CDL 在前、HOM 在后——下游解析脚本认的 JSON 字段顺序。
        if (saveRoot)
            AcqSaver::saveRadialWind(*saveRoot, sessionDir, fileStem, AcqSaver::timeStem(collector->frameTime()),
                                     {{QStringLiteral("CDL"), {cdlWS, cdlCNR}},
                                      {QStringLiteral("HOM"), {homWS, homCNR}}});

        // 6. 风廓线：本束进环 → 同步位为真就反演一次（见本函数上方的环注）。
        //    朝向是编排层经云台族口 `worldOf` 换好的**世界朝向**（安装标定已计入）；空 = 未知。
        homRing_[ringW_] = {pose, homWS};
        cdlRing_[ringW_] = {pose, cdlWS};
        ringW_ = (ringW_ + 1) % WindProfile::kBeams;
        if (!sync)
            return;
        // 环直接以 span 交给 Calc，只看不拷。
        const auto homProfile = WindProfile::Calc(homRing_, HfnlConfig::homDBinDist, collector->frameTime());
        const auto cdlProfile = WindProfile::Calc(cdlRing_, HfnlConfig::cdlDBinDist, collector->frameTime());
        if (homProfile)
        {
            Q_EMIT self->homProfileReady(homProfile);
            if (saveRoot)
                AcqSaver::saveWindProfile(homProfile, *saveRoot, sessionDir, QStringLiteral("HOM"));
        }
        if (cdlProfile)
        {
            Q_EMIT self->cdlProfileReady(cdlProfile);
            if (saveRoot)
                AcqSaver::saveWindProfile(cdlProfile, *saveRoot, sessionDir, QStringLiteral("CDL"));
        }
    };
}
