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

import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.ArrayList;

public class PcapFileWriter {

    private final static byte PCAP_HEADER[] = {
            (byte) 0xd4, (byte) 0xc3, (byte) 0xb2, (byte) 0xa1,    // magic number
            (byte) 0x02, (byte) 0x00, (byte) 0x04, (byte) 0x00,    // version numbers
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,    // thiszone
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,    // sigfigs
            (byte) 0xff, (byte) 0xff, (byte) 0x00, (byte) 0x00,    // snaplen
            (byte) 0x7f, (byte) 0x00, (byte) 0x00, (byte) 0x00};    // wifi
    private DataOutputStream outputStream;
    private String fileName;
    private String filePath;


    public PcapFileWriter(String filePath, String fileName) {
        this.fileName = fileName;
        this.filePath = filePath;
        if(!this.filePath.endsWith("/"))
            this.filePath = this.filePath.concat("/");

        try {
            outputStream = new DataOutputStream(new FileOutputStream(this.filePath + this.fileName));
            outputStream.write(PCAP_HEADER);
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

    public boolean writePacket(Packet packet) {
        try {
            outputStream = new DataOutputStream(new FileOutputStream(filePath + fileName, true));
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
