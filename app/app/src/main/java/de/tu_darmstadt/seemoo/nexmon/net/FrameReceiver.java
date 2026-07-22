/*
 * Nexmon PenTestSuite
 * Copyright (C) 2016 Fabian Knapp
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package de.tu_darmstadt.seemoo.nexmon.net;

import android.util.Log;

import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;

public class FrameReceiver {

    int packetNo = 0;

    private FrameObserver observer = new FrameObserver();

    public FrameObserver getObserver() {
        return observer;
    }

    public native String getMyData();

    public native void stopSniffing();


    public void receivePaketSocket(byte[] packets, byte[] header) {
        Packet packet;

        switch (MonitorModeService.monitorModeType) {
            case MONITOR_IEEE80211:
            case MONITOR_IEEE80211_BADFCS:
                packet = new Packet(Packet.LinkType.IEEE_802_11);
                break;
            case MONITOR_RADIOTAP:
            case MONITOR_RADIOTAP_BADFCS:
                packet = new Packet(Packet.LinkType.IEEE_802_11_WLAN_RADIOTAP);
                break;
            default:
                return;
        }

        packet._rawData = packets;
        packet._rawHeader = header;
        packet._headerLen = header.length;
        packet._dataLen = packets.length;

        observer.packetReceived(packet);
    }
}
