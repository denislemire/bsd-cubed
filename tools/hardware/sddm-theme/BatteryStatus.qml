// SPDX-License-Identifier: BSD-2-Clause
import QtQuick 2.15

Image {
    id: indicator
    cache: false
    visible: status === Image.Ready
    fillMode: Image.PreserveAspectFit
    source: ""
    Accessible.name: "Battery"

    property string statusFile: "file:///tmp/bsd3-sddm-battery.svg"
    property int generation: 0

    Timer {
        interval: 15000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            indicator.generation += 1
            indicator.source = indicator.statusFile + "?" + indicator.generation
        }
    }
}
