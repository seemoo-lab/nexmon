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

package de.tu_darmstadt.seemoo.nexmon.sharky;

import de.tu_darmstadt.seemoo.nexmon.DissectionStrings;

public class SharkListElement {

    private int number;
    private String time;
    private String source;
    private String dest;
    private String protocol;
    private int length;
    private String ssid;
    private String type;

    public static SharkListElement getSharkListElementSmall(Packet packet) {
        SharkListElement element = null;
        String value;

        element = new SharkListElement();

        element.setNumber(packet._number);

        while(!Packet.blocked.compareAndSet(false, true))
            try {
                Thread.sleep(5);
            } catch(Exception e) {e.printStackTrace();}

        if(packet._encap.equals(Packet.LinkType.IEEE_802_11_WLAN_RADIOTAP) || packet._encap.equals(Packet.LinkType.IEEE_802_11)) {
            value = packet.getField(DissectionStrings.DISS_DST_ADDR);
            if (value != null)
                element.setDest(value);
            else {
                value = packet.getField(DissectionStrings.DISS_RECEIVER_ADDR);
                if (value != null)
                    element.setDest(value);
                else
                    element.setDest("unknown");
            }

            value = packet.getField(DissectionStrings.DISS_SRC_ADDR);
            if (value != null)
                element.setSource(value);
            else
                element.setSource("unknown");
        } else {
            value = packet.getField(DissectionStrings.DISS_IP_SRC);
            if(value != null) {
                element.setSource(value);
            } else {
                value = packet.getField(DissectionStrings.DISS_IPV6_SRC);
                if(value != null)
                    element.setSource(value);
                else
                    element.setSource("unknown");
            }
            value = packet.getField(DissectionStrings.DISS_IP_DST);
            if(value != null) {
                element.setDest(value);
            } else {
                value = packet.getField(DissectionStrings.DISS_IPV6_DST);
                if(value != null)
                    element.setDest(value);
                else
                    element.setDest("unknown");
            }
        }

        value = packet.getField(DissectionStrings.DISS_PROTOCOLS);
        String proto[] = value.split(":");


        element.setProtocol(proto[proto.length - 1]);

        element.setSSID("");
        element.setType("");

        value = packet.getField(DissectionStrings.DISS_TIME);

        int start = value.indexOf(":") - 2;
        int end = value.lastIndexOf(".");
        value = value.substring(start, end);

        element.setTime(value);


        value = packet.getField(DissectionStrings.DISS_LENGTH);
        element.setLength(Integer.parseInt(value));

        Packet.blocked.set(false);
        return element;
    }

    public int getNumber() {
        return number;
    }

    public void setNumber(int number) {
        this.number = number;
    }

    public String getTime() {
        return time;
    }

    public void setTime(String time) {
        this.time = time;
    }

    public String getSource() {
        return source;
    }

    public void setSource(String source) {
        this.source = source;
    }

    public String getDest() {
        return dest;
    }

    public void setDest(String dest) {
        this.dest = dest;
    }

    public String getProtocol() {
        return protocol;
    }

    public void setProtocol(String protocol) {
        this.protocol = protocol;
    }

    public int getLength() {
        return length;
    }

    public void setLength(int length) {
        this.length = length;
    }

    public String getSSID() {
        return ssid;
    }

    public void setSSID(String ssid) {
        this.ssid = ssid;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

}
