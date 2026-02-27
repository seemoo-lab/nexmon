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

package de.tu_darmstadt.seemoo.nexmon.gui;

import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.stations.Station;

public class APlistElement {

    public String bssid;
    public int beacons;
    public String ssid;
    public String cipher;
    public String auth;
    public int channel;
    public String encryption;
    public long lastSeen;
    public String signalStrength;
    public ArrayList<Station> stations;
    public boolean hasHandshake = false;


}

