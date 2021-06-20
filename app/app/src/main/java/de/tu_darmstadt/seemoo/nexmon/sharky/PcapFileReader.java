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

import android.annotation.SuppressLint;
import android.util.Log;

import java.io.FileInputStream;
import java.nio.ByteOrder;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.HashMap;

public class PcapFileReader {

    public final static int PCAP_FILE_HEADER_LENGTH = 24;
    public final static int PCAP_PACKET_HEADER_LENGTH = 16;


    private String fileName;
    private FileChannel channel;
    private MappedByteBuffer buffer;
    private HashMap<Integer, Integer> posMap;
    private Packet.LinkType linktype = Packet.LinkType.IEEE_802_11_WLAN_RADIOTAP;


    public PcapFileReader(String fileName) {
        this.fileName = fileName;
        int pcapLinktype = getBuffer().getInt(20);
        calcPositions();

        linktype = Packet.LinkType.getLinktypeFromPcapValue(pcapLinktype);

        if (linktype == null)
            throw new UnsupportedOperationException("Unknown File Format.");
    }

    public int getAmountOfPackets() {
        return posMap.size();
    }

    public ArrayList<Packet> getPackets(int amount) {

        return getPackets(1, amount);
    }

    public ArrayList<Packet> getPackets(int startFrame, int amount) {
        if (amount < 1)
            amount = Integer.MAX_VALUE;
        ArrayList<Packet> list = new ArrayList<Packet>();
        if (posMap != null && !posMap.isEmpty()) {
            int offset = posMap.get(startFrame);
            Packet packet;

            for (int i = 0; i < amount; i++) {
                packet = getPacket(offset);
                packet._number = startFrame + i;
                offset += packet._headerLen + packet._dataLen;
                list.add(packet);
                getBuffer().position(offset);
                if (!getBuffer().hasRemaining())
                    break;
            }
        }


        return list;
    }

    private MappedByteBuffer getBuffer() {
        if (channel == null || !channel.isOpen()) {
            try {
                channel = new FileInputStream(fileName).getChannel();
               // buffer = channel.map(FileChannel.MapMode.READ_ONLY, PCAP_FILE_HEADER_LENGTH, channel.size() - PCAP_FILE_HEADER_LENGTH);
                buffer = channel.map(FileChannel.MapMode.READ_ONLY, 0, channel.size());

                buffer.order(ByteOrder.LITTLE_ENDIAN);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return buffer;
    }

    @SuppressLint("UseSparseArrays")
    public void calcPositions() {
        posMap = new HashMap<Integer, Integer>();

        int offset = PCAP_FILE_HEADER_LENGTH;
        //int offset = 0;

        getBuffer().position(offset);
        for (int i = 1; getBuffer().hasRemaining(); i++) {
            posMap.put(i, offset);
            offset += getBuffer().getInt(8 + offset) + PCAP_PACKET_HEADER_LENGTH;
            getBuffer().position(offset);
        }
    }

    public int getPosition(int frameNumber) {
        int offset = PCAP_FILE_HEADER_LENGTH;
        //int offset = 0;

        for (int i = 1; i < frameNumber; i++) {
            offset += getBuffer().getInt(8 + offset) + PCAP_PACKET_HEADER_LENGTH;
        }

        return offset;
    }


    public Packet getPacket(int offset) {
        Packet packet = new Packet(linktype);
        try {
            packet._rawHeader = new byte[PCAP_PACKET_HEADER_LENGTH];
            getBuffer().position(offset);
            getBuffer().get(packet._rawHeader, 0, PCAP_PACKET_HEADER_LENGTH);
            packet._headerLen = PCAP_PACKET_HEADER_LENGTH;
            getBuffer().rewind();
            int payloadLength = buffer.getInt(8 + offset);
            packet._dataLen = payloadLength;
            getBuffer().position(PCAP_PACKET_HEADER_LENGTH + offset);
            packet._rawData = new byte[payloadLength];
            getBuffer().get(packet._rawData, 0, payloadLength);

        } catch (Exception e) {
            e.printStackTrace();
        }

        return packet;
    }

    public HashMap<Integer, Integer> getPositions() {
        return posMap;
    }


    public String getFileName() {
        return fileName;
    }


}
