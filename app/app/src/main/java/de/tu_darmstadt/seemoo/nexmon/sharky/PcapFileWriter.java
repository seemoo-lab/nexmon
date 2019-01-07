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

import android.os.Environment;
import android.util.Log;

import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.net.MonitorModeService;

public class PcapFileWriter {

    public static class PcapHeader {
        public static byte[] getBytes(int linktype) {
            ByteBuffer buf = ByteBuffer.allocate(24);
            buf.order(ByteOrder.LITTLE_ENDIAN);
            buf.putInt(0xa1b2c3d4);                     // magic number
            buf.putShort((short) 0x0002);               // major version number
            buf.putShort((short) 0x0004);               // minor version numbers
            buf.putInt(0);                              // GTM to local correction
            buf.putInt(0);                              // accuracy of timestamps
            buf.putInt(0x0000ffff);                     // max length of captured packets, in octets
            buf.putInt(linktype);  // data link type

            return buf.array();
        }

        public static byte[] getBytesWithoutLinktype() {
            ByteBuffer buf = ByteBuffer.allocate(20);
            buf.order(ByteOrder.LITTLE_ENDIAN);
            buf.putInt(0xa1b2c3d4);                     // magic number
            buf.putShort((short) 0x0002);               // major version number
            buf.putShort((short) 0x0004);               // minor version numbers
            buf.putInt(0);                              // GTM to local correction
            buf.putInt(0);                              // accuracy of timestamps
            buf.putInt(0x0000ffff);                     // max length of captured packets, in octets

            return buf.array();
        }
    }

    private DataOutputStream outputStream;
    private String fileName;
    private String filePath;
    private boolean isFirstFrame = true;


    public PcapFileWriter(String filePath, String fileName) {
        this.fileName = fileName;
        this.filePath = filePath;
        if(!this.filePath.endsWith("/"))
            this.filePath = this.filePath.concat("/");

        try {
            outputStream = new DataOutputStream(new FileOutputStream(this.filePath + this.fileName));
            //outputStream.write(PcapHeader.getBytes(Packet.PCAP_LINKTYPE_IEEE802_11));
            //outputStream.write(PcapHeader.getBytes(Packet.PCAP_LINKTYPE_RADIOTAP));
            outputStream.write(PcapHeader.getBytesWithoutLinktype());
            //outputStream.write(PcapHeader.getBytes(Packet.LinkType.IEEE_802_11_WLAN_RADIOTAP.getPcapLinktype()));
            outputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public PcapFileWriter(String fileName) {
        this(Environment.getExternalStorageDirectory().getAbsolutePath(), fileName);
    }

    public static boolean copy(String src, String dst) throws IOException {
        try {
            FileInputStream inStream = new FileInputStream(src);
            FileOutputStream outStream = new FileOutputStream(dst);
            FileChannel inChannel = inStream.getChannel();
            FileChannel outChannel = outStream.getChannel();
            inChannel.transferTo(0, inChannel.size(), outChannel);
            inStream.close();
            outStream.close();
            return true;
        } catch(Exception e) {
            e.printStackTrace();
            return false;
        }

    }

    public static boolean concat(String src1, String src2, String dst) throws IOException {
        try {
            FileInputStream inStream1 = new FileInputStream(src1);
            FileInputStream inStream2 = new FileInputStream(src2);
            FileOutputStream outStream = new FileOutputStream(dst);
            FileChannel inChannel1 = inStream1.getChannel();
            FileChannel inChannel2 = inStream2.getChannel();
            FileChannel outChannel = outStream.getChannel();
            inChannel1.transferTo(0, inChannel1.size(), outChannel);
            inChannel2.transferTo(24, inChannel2.size()-24, outChannel);
            inStream1.close();
            inStream2.close();
            outStream.close();
            return true;
        } catch(Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    private void handleFirstFrame(Packet packet) throws IOException {
        if (isFirstFrame) {
            isFirstFrame = false;

            ByteBuffer buf = ByteBuffer.allocate(4);
            buf.order(ByteOrder.LITTLE_ENDIAN);
            buf.putInt(packet._encap.getPcapLinktype());

            outputStream.write(buf.array());

            Log.d("PCAP", filePath + fileName);
        }
    }

    public boolean writePacket(Packet packet) {
        try {
            outputStream = new DataOutputStream(new FileOutputStream(filePath + fileName, true));
            handleFirstFrame(packet);
            outputStream.write(packet._rawHeader);
            outputStream.write(packet._rawData);
            outputStream.close();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }

    }

    public boolean writePackets(ArrayList<Packet> packets) {
        try {
            outputStream = new DataOutputStream(new FileOutputStream(filePath + fileName, true));

            for (Packet packet : packets) {
                handleFirstFrame(packet);
                outputStream.write(packet._rawHeader);
                outputStream.write(packet._rawData);
            }

            outputStream.close();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }


    }


}
