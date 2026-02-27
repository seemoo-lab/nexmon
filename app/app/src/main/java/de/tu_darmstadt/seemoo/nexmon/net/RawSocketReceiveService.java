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

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.support.annotation.Nullable;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.ServerSocket;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.sharky.PcapFileReader;


/**
 * Created by fabian on 10/31/16.
 */
public class RawSocketReceiveService extends Service {

    private ServerSocket serverSocket;
    private final String TAG = "RawSocketReceiveService";

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    byte[] packet = new byte[4096];
                    DatagramPacket datagramPacket = new DatagramPacket(packet, packet.length);
                    DatagramSocket datagramSocket = new DatagramSocket(5555, InetAddress.getLocalHost());

                    while(true) {
                        datagramSocket.receive(datagramPacket);
                        int headerLen = PcapFileReader.PCAP_PACKET_HEADER_LENGTH;
                        byte[] header = new byte[headerLen];
                        byte[] data = new byte[datagramPacket.getLength() - headerLen];

                        for(int i = 0; i < headerLen; i++)
                            header[i] = datagramPacket.getData()[i];

                        for(int i = headerLen; i < datagramPacket.getLength(); i++)
                            data[i-headerLen] = datagramPacket.getData()[i];

                        MyApplication.getFrameReceiver().receivePaketSocket(data, header);

                    }

                } catch(Exception e) {e.printStackTrace();}
            }
        }).start();


        return Service.START_NOT_STICKY;
    }


    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
