package de.tu_darmstadt.seemoo.nexmon.net;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.support.annotation.Nullable;

import com.stericson.RootShell.execution.Command;
import com.stericson.RootTools.RootTools;

import java.util.HashMap;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;

/**
 * Created by fabian on 11/25/16.
 */

public class MonitorModeService extends Service {

    public static final int MONITOR_MODE_ATTACKS = 500;
    public static final int MONITOR_MODE_AIRODUMP = 501;
    public static final int MONITOR_MODE_WIRESHARK = 502;

    public static final String MONITOR_MODE_ID = "nameid";
    public static final String MONITOR_MODE_NEED = "need";

    public static final int COMMAND_CHECK_MONITOR = 30;


    public static final String INTENT_RECEIVER = "de.tu_darmstadt.seemoo.nexmon.MONITOR_MODE";


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

        final Command command = new Command(COMMAND_CHECK_MONITOR, "nexutil -m", "nexutil -n") {
            @Override
            public void commandOutput(int id, String line) {
                if(id == COMMAND_CHECK_MONITOR) {
                    if(line.contains("monitor: 0")) {
                        try {
                            RootTools.getShell(true).add(new StartMonitorModeCommand());

                            // START NOTIFICAITON
                            MyApplication.showSurveyNotification();

                        } catch(Exception e) {e.printStackTrace();}
                    }
                }

                super.commandOutput(id, line);
            }
        };

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    RootTools.getShell(true).add(command);
                } catch(Exception e) {e.printStackTrace();}
            }
        }).start();
    }

    private void stopMonitorMode() {

        final Command command = new Command(COMMAND_CHECK_MONITOR, "nexutil -m", "nexutil -n") {
            @Override
            public void commandOutput(int id, String line) {
                if(id == COMMAND_CHECK_MONITOR) {
                    if(line.contains("monitor: 2")) {
                        try {
                            RootTools.getShell(true).add(new StopMonitorModeCommand());

                            // STOP NOTIFICATION
                            MyApplication.dismissSurveyNotification();

                        } catch(Exception e) {e.printStackTrace();}
                    }
                }

                super.commandOutput(id, line);
            }
        };

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    RootTools.getShell(true).add(command);
                } catch(Exception e) {e.printStackTrace();}
            }
        }).start();
    }



    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private class StartMonitorModeCommand extends Command {
        public static final int COMMAND_START_MONITOR = 31;

        public StartMonitorModeCommand() {
            super(COMMAND_START_MONITOR, "nexutil -s 52 -v 1 -c1 -m2", "rawproxy -i wlan0 -p " + android.os.Process.myPid() + " &", "rawproxyreverse -i wlan0 -p " + android.os.Process.myPid() + " &");
        }
    }

    private class StopMonitorModeCommand extends Command {
        public static final int COMMAND_STOP_MONITOR = 32;

        public StopMonitorModeCommand() {
            super(COMMAND_STOP_MONITOR, "nexutil -c0 -m0", "pkill rawproxy", "pkill rawproxyreverse");
        }
    }

}
