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

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.Environment;
import android.os.IBinder;

import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.gui.SharkFragment;
import de.tu_darmstadt.seemoo.nexmon.net.IFrameReceiver;
import de.tu_darmstadt.seemoo.nexmon.net.MonitorModeService;

public class WiresharkService extends Service implements IFrameReceiver {

    private final IBinder mBinder = new MyBinder();
    private ArrayList<SharkListElement> sharkListElement = new ArrayList<SharkListElement>();
    private PcapFileWriter writer;
    private int receivedPackets;
    private boolean isCapturing = false;

    private long startTime;

    @Override
    public void onCreate() {
        super.onCreate();

    }


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return Service.START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void packetReceived(Packet packet) {
        writer.writePacket(packet);
        receivedPackets++;
        packet._number = receivedPackets;
        addToList(SharkListElement.getSharkListElementSmall(packet));
        packet.cleanDissection();
        sendBroadcast(new Intent("de.tu_darmstadt.seemoo.nexmon.NEW_FRAME"));
    }

    private void addToList(SharkListElement element) {
        if (sharkListElement.size() == SharkFragment.PACKET_AMOUNT_TO_SHOW)
            sharkListElement.clear();
        sharkListElement.add(element);

    }

    public void startLiveCapturing() {

        if (!isCapturing) {
            startTime = System.currentTimeMillis();
            sharkListElement.clear();
            receivedPackets = 0;
            MyApplication.deleteTempFile();
            String tmpFile = "tmp_" + System.currentTimeMillis() + ".pcap";
            MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).edit().putString("reader", Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + tmpFile).commit();

            writer = new PcapFileWriter(tmpFile);
            MyApplication.getFrameReceiver().getObserver().addObserver(this);
            isCapturing = true;

            sendMonitorModeBroadcast(true);
        }
    }

    private void sendMonitorModeBroadcast(boolean monitorModeNeeded) {
        Intent intent = new Intent(MonitorModeService.INTENT_RECEIVER);
        intent.putExtra(MonitorModeService.MONITOR_MODE_ID, MonitorModeService.MONITOR_MODE_WIRESHARK);
        intent.putExtra(MonitorModeService.MONITOR_MODE_NEED, monitorModeNeeded);

        MyApplication.getAppContext().sendBroadcast(intent);
    }

    public void toggleLiveCapturing() {
        if (isCapturing)
            stopLiveCapturing();
        else
            startLiveCapturing();
    }


    public void stopLiveCapturing() {
        if (isCapturing) {
            isCapturing = false;

            MyApplication.getFrameReceiver().getObserver().removeObserver(this);
            sendMonitorModeBroadcast(false);
        }
    }

    public boolean isCapturing() {
        return isCapturing;
    }

    public ArrayList<SharkListElement> getList() {
        return sharkListElement;
    }

    public void clearList() {
        boolean wasCapturing = isCapturing;
        stopLiveCapturing();
        MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).edit().remove("reader").apply();

        sendBroadcast(new Intent("de.tu_darmstadt.seemoo.nexmon.NEW_FRAME"));

        if (wasCapturing)
            startLiveCapturing();


    }



    public class MyBinder extends Binder {
        public WiresharkService getService() {
            return WiresharkService.this;
        }
    }


}
