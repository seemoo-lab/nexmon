package de.tu_darmstadt.seemoo.nexmon.net;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.util.Log;

import com.stericson.RootShell.execution.Command;
import com.stericson.RootTools.RootTools;

import java.util.HashMap;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.utils.Assets;
import de.tu_darmstadt.seemoo.nexmon.utils.Nexutil;
import eu.chainfire.libsuperuser.Shell;

/**
 * Created by fabian on 11/25/16.
 */

public class MonitorModeService extends Service {

    public static final int MONITOR_MODE_ATTACKS = 500;
    public static final int MONITOR_MODE_AIRODUMP = 501;
    public static final int MONITOR_MODE_WIRESHARK = 502;

    public static final String MONITOR_MODE_ID = "nameid";
    public static final String MONITOR_MODE_NEED = "need";

    public static final String INTENT_RECEIVER = "de.tu_darmstadt.seemoo.nexmon.MONITOR_MODE";

    public enum MonitorModeType {
        MONITOR_DISABLED(0),
        MONITOR_IEEE80211(1),
        MONITOR_RADIOTAP(2),
        MONITOR_IEEE80211_BADFCS(33),
        MONITOR_RADIOTAP_BADFCS(34);

        private final int value;

        private MonitorModeType(int value) {
            this.value = value;
        }

        public int getInt() { return value; }
    }
    public static MonitorModeType monitorModeType = MonitorModeType.MONITOR_DISABLED;


    private HashMap<Integer, Boolean> monitorModeRequested;

    private BroadcastReceiver receiver;

    @Override
    public void onCreate() {
        super.onCreate();

        monitorModeRequested = new HashMap<>(3);
        monitorModeRequested.put(MONITOR_MODE_ATTACKS, false);
        monitorModeRequested.put(MONITOR_MODE_AIRODUMP, false);
        monitorModeRequested.put(MONITOR_MODE_WIRESHARK, false);

        receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if(intent.hasExtra(MONITOR_MODE_ID) && intent.hasExtra(MONITOR_MODE_NEED)) {
                    int nameId = intent.getIntExtra(MONITOR_MODE_ID, -1);
                    boolean requested = intent.getBooleanExtra(MONITOR_MODE_NEED, false);

                    if(nameId > 0) {
                        monitorModeRequested.put(nameId, requested);
                    }
                }

                if(needMonitorMode()) {
                    startMonitorMode();
                } else {
                    stopMonitorMode();
                }
            }
        };

        IntentFilter intentFiler = new IntentFilter(INTENT_RECEIVER);
        registerReceiver(receiver, intentFiler);

    }

    private boolean needMonitorMode() {
        return monitorModeRequested.containsValue(true);
    }

    private void startMonitorMode() {
        String line = Nexutil.getInstance().getIoctl(Nexutil.WLC_GET_MONITOR);
        Log.d("SU", "startMonitorMode");

        if(line.contains("00 00 00 00")) {
            Log.d("SU", "startMonitorMode: activate");
            //monitorModeType = MonitorModeType.MONITOR_RADIOTAP;
            monitorModeType = MonitorModeType.MONITOR_IEEE80211;
            Shell.SU.run(Assets.getAssetsPath(getApplicationContext(), "nexutil") + " -s52 -v1 -c1 -m" + monitorModeType.getInt());
            Shell.SU.run(Assets.getAssetsPath(getApplicationContext(), "rawproxy") + " -i wlan0 -p " + android.os.Process.myPid() + " &");
            Shell.SU.run(Assets.getAssetsPath(getApplicationContext(), "rawproxyreverse") + " -i wlan0 -p " + android.os.Process.myPid() + " &");
        }
    }

    private void stopMonitorMode() {
        String line = Nexutil.getInstance().getIoctl(Nexutil.WLC_GET_MONITOR);
        Log.d("SU", "stopMonitorMode");

        if(line.contains("02 00 00 00") || line.contains("01 00 00 00")) {
            Log.d("SU", "stopMonitorMode: deactivate");
            Shell.SU.run(Assets.getAssetsPath(getApplicationContext(), "nexutil") + " -c0 -m0");
            Shell.SU.run("pkill rawproxy");
            Shell.SU.run("pkill rawproxyreverse");
            monitorModeType = MonitorModeType.MONITOR_DISABLED;
        }
    }



    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

}
