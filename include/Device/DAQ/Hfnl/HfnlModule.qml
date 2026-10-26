pragma ComponentBehavior: Bound
import QtQuick
import Lidar.UI
import Lidar.TaskUI
import Lidar.Charts
import QtQuick.Controls

// 末端页面独持具体型号后端，在这里连接业务、图窗与控制分区。
ModulePage {
    id: root
    readonly property string kind: "device"
    readonly property DAQDeviceBase family: internals.device
    property var acquisitionParameters
    property int accum: acquisitionParameters.accumDefault
    property url saveDir: acquisitionParameters.saveDirDefault
    property bool save: false
    sections: [connectionSection, acquisitionSection].filter(item => item !== null)
    // 待连接表单；编辑不修改 Handle 的运行快照。
    property int networkInterfaceIndex: internals.connectionParameters.networkInterfaceDefault
    property string hostIPv4: internals.connectionParameters.hostIPv4Default
    property int hostPort: internals.connectionParameters.hostPortDefault
    property string deviceIPv4: internals.connectionParameters.deviceIPv4Default
    property int devicePort: internals.connectionParameters.devicePortDefault
    property string deviceMAC: internals.connectionParameters.deviceMACDefault
    mainPages: [
        Page {
            title: qsTr("HOM 功率谱 3D")
            contentItem: SurfaceView {
                id: homSurface
                specification: internals.chartParameters.homPlotConfiguration
                Connections {
                    target: internals.device
                    function onHomSpecReady(values) { homSurface.updateSpectrum(values) }
                }
            }
        },
        Page {
            title: qsTr("CDL 功率谱 3D")
            contentItem: SurfaceView {
                id: cdlSurface
                specification: internals.chartParameters.cdlPlotConfiguration
                Connections {
                    target: internals.device
                    function onCdlSpecReady(values) { cdlSurface.updateSpectrum(values) }
                }
            }
        },
        Page {
            title: qsTr("HOM 风速 | CNR")
            contentItem: Wind2DView {
                id: homWind
                specification: internals.chartParameters.homPlotConfiguration
                Connections {
                    target: internals.device
                    function onHomWSReady(values) { homWind.updateWS(values) }
                    function onHomCNRReady(values) { homWind.updateCNR(values) }
                }
            }
        },
        Page {
            title: qsTr("CDL 风速 | CNR")
            contentItem: Wind2DView {
                id: cdlWind
                specification: internals.chartParameters.cdlPlotConfiguration
                Connections {
                    target: internals.device
                    function onCdlWSReady(values) { cdlWind.updateWS(values) }
                    function onCdlCNRReady(values) { cdlWind.updateCNR(values) }
                }
            }
        },
        Page {
            title: qsTr("HOM 风羽图")
            contentItem: WindBarbItem {
                id: homBarb
                readonly property var spec: internals.chartParameters.homPlotConfiguration
                configuration: ({ heightBins: spec.rows, maxHeight: spec.rows * spec.gateSpacing,
                    windowSeconds: spec.barbWindow, columnSeconds: spec.barbColumn })
                Connections {
                    target: internals.device
                    function onHomProfileReady(profile) { homBarb.appendColumn(profile) }
                }
            }
        },
        Page {
            title: qsTr("CDL 风羽图")
            contentItem: WindBarbItem {
                id: cdlBarb
                readonly property var spec: internals.chartParameters.cdlPlotConfiguration
                configuration: ({ heightBins: spec.rows, maxHeight: spec.rows * spec.gateSpacing,
                    windowSeconds: spec.barbWindow, columnSeconds: spec.barbColumn })
                Connections {
                    target: internals.device
                    function onCdlProfileReady(profile) { cdlBarb.appendColumn(profile) }
                }
            }
        }
    ]
    readonly property Section connectionSection: PcapCard {
        device: internals.device
        settings: root
        parameters: internals.connectionParameters
    }
    acquisitionParameters: internals.device.acquisitionParameters
    readonly property Section acquisitionSection: DAQControl {
        device: internals.device
        acquisition: root
    }
    objectName: "HfnlModule"
    readonly property string name: internals.device.device.displayName
    readonly property string subtitle: internals.device.device.subtitle
    readonly property bool isOpen: internals.device.device.isOpen
    readonly property bool isLocked: internals.device.device.isLocked
    readonly property string holder: internals.device.device.holder


    QtObject {
        id: internals
        readonly property var chartParameters: device.chartParameters()
        readonly property var connectionParameters: device.connectionParameters
        readonly property HfnlDevice device: root.HfnlDevice
    }
    Connections {
        target: internals.device.device.logSource
        function onLogMessage(text): void { root.logMessage(text) }
    }
}
