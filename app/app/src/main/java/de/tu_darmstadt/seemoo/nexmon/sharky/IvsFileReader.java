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

import java.io.FileInputStream;
import java.io.UnsupportedEncodingException;
import java.nio.ByteOrder;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.util.HashMap;

import de.tu_darmstadt.seemoo.nexmon.gui.MyActivity;

/**
 * Created by fabian on 9/27/16.
 */
public class IvsFileReader {

    private String fileName;
    private FileChannel channel;
    private MappedByteBuffer buffer;
    private HashMap<Integer, Integer> posMap;


    public final static int IVS_FILE_HEADER_LENGTH = 6;
    public final static int IVS_PACKET_HEADER_LENGTH = 4;

    public static final byte[] MAGIC = {(byte) 0xAE, 0x78, (byte) 0xD1, (byte) 0xFF};

    public IvsFileReader(String fileName) throws UnsupportedEncodingException {
        this.fileName = fileName;
        if(isIvsFile())
            calcPositions();
        else
            throw new UnsupportedEncodingException("No valid IVS file!");
    }

    public MappedByteBuffer getBuffer() {
        if (channel == null || !channel.isOpen()) {
            try {
                channel = new FileInputStream(fileName).getChannel();
                buffer = channel.map(FileChannel.MapMode.READ_ONLY, 0, channel.size());
                buffer.order(ByteOrder.LITTLE_ENDIAN);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        return buffer;
    }

    public boolean isIvsFile() {
        return  getBuffer().get(0) == MAGIC[0] &&
                getBuffer().get(1) == MAGIC[1] &&
                getBuffer().get(2) == MAGIC[2] &&
                getBuffer().get(3) == MAGIC[3];
    }

    private void calcPositions() {
        posMap = new HashMap<Integer, Integer>();

        int offset = IVS_FILE_HEADER_LENGTH;

        getBuffer().position(offset);
        for (int i = 1; getBuffer().hasRemaining(); i++) {
            posMap.put(i, offset);
            offset += getBuffer().getShort(2 + offset) + IVS_PACKET_HEADER_LENGTH;
            getBuffer().position(offset);
        }
    }

    public HashMap<String, String> getAps() {
        HashMap<String, String> apMap = new HashMap<>();
        for(int position : posMap.values()) {
            boolean rightType = (byte) (getBuffer().get(position) & (byte) 0x03) == 0x03;
            if(rightType) {
                int len = getBuffer().getShort(position + 2);
                byte[] mac = new byte[6];
                getBuffer().position(position + 4);
                for(int i = 0; i < 6; i++)
                    mac[i] = getBuffer().get();

                byte[] essid = new byte[len - 6];

                for(int i = 0; i < (len-6); i++)
                    essid[i] = getBuffer().get();

                String essidString = new String(essid);
                String bssidString = MyActivity.bytesToHex(mac);
                bssidString = bssidString.trim().replaceAll(" ", ":");

                apMap.put(bssidString, essidString);
            }
        }
        return apMap;
    }


}
