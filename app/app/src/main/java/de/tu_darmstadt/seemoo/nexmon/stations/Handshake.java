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

package de.tu_darmstadt.seemoo.nexmon.stations;

import java.util.HashMap;

import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;

/**
 * Created by fabian on 11/13/16.
 */

public class Handshake {
    private static final int MAX_TIME_DIFFERENCE = 3000;
    public String stationMac;
    public HashMap<Integer, Packet> hsPackets = new HashMap<>(4);
    public long lastTime;
    public boolean isComplete = false;

    public void addHandshakeMessage(int number, Packet packet) {
        if(number == 1 && !hsPackets.containsKey(2)) {
            hsPackets.put(1, packet);
            lastTime = System.currentTimeMillis();
        } else if(number == 2 && !hsPackets.containsKey(2)) {
            if((System.currentTimeMillis() - lastTime) < MAX_TIME_DIFFERENCE) {
                hsPackets.put(2, packet);
                lastTime = System.currentTimeMillis();
                isComplete = true;
            }
        } else if(number == 3 && hsPackets.containsKey(2) && !hsPackets.containsKey(3)) {
            hsPackets.put(3, packet);
            lastTime = System.currentTimeMillis();
        } else if(number == 4 && hsPackets.containsKey(3) && !hsPackets.containsKey(4)) {
            hsPackets.put(4, packet);
            lastTime = System.currentTimeMillis();
        }
    }}
