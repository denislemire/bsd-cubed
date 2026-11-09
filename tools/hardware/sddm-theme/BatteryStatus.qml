// SPDX-License-Identifier: BSD-2-Clause
import QtQuick 2.0
import org.kde.plasma.private.battery

Text {
    id: indicator
    BatteryControlModel { id: battery }
    visible: battery.hasInternalBatteries && battery.hasCumulative && battery.percent >= 0 && battery.percent <= 100
    text: visible ? qsTr("Battery %1%").arg(Math.round(battery.percent)) + (battery.pluggedIn ? qsTr(" · Plugged in") : "") : ""
    color: "#f2f2f2"
    font.pixelSize: 16
    Accessible.name: text
}
