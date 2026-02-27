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

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

import de.tu_darmstadt.seemoo.nexmon.stations.AttackInfoString;

public class FrameSender {

    public static synchronized native AttackInfoString sendFrame(byte[] frame);

    public static boolean sendViaSocket(byte[] frame) {
        try {
            DatagramPacket datagramPacket = new DatagramPacket(frame, frame.length, InetAddress.getLocalHost(), 5556);
            DatagramSocket datagramSocket = new DatagramSocket(5557, InetAddress.getLocalHost());
            datagramSocket.send(datagramPacket);
            datagramSocket.close();
            return true;
        } catch(Exception e) {
            e.printStackTrace();
            return false;
        }

    }

}
