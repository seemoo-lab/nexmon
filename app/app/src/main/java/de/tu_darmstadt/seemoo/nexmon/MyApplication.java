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

package de.tu_darmstadt.seemoo.nexmon;


import android.app.ActivityManager;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.graphics.Typeface;
import android.hardware.SensorManager;
import android.location.LocationManager;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.os.Vibrator;
import android.telephony.TelephonyManager;
import android.text.SpannableStringBuilder;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.WindowManager;
import android.widget.Toast;

import com.google.android.gms.analytics.GoogleAnalytics;
import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.Tracker;
import com.stericson.RootShell.execution.Command;
import com.stericson.RootTools.RootTools;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Locale;
import java.util.UUID;

import de.tu_darmstadt.seemoo.nexmon.net.FrameReceiver;
import de.tu_darmstadt.seemoo.nexmon.net.MonitorModeService;
import de.tu_darmstadt.seemoo.nexmon.net.RawSocketReceiveService;
import de.tu_darmstadt.seemoo.nexmon.sharky.WiresharkService;
import de.tu_darmstadt.seemoo.nexmon.stations.APfinderService;
import de.tu_darmstadt.seemoo.nexmon.stations.AttackService;


public class MyApplication extends Application {


    private static Tracker mTracker;

    /**
     * Gets the default {@link Tracker} for this {@link Application}.
     * @return tracker
     */
    synchronized public static Tracker getDefaultTracker() {
        if (mTracker == null) {
            GoogleAnalytics analytics = GoogleAnalytics.getInstance(getAppContext());
            // To enable debug logging use: adb shell setprop log.tag.GAv4 DEBUG
            mTracker = analytics.newTracker(R.xml.global_tracker);
            mTracker.set("&uid", nexmonUID);
            mTracker.send(new HitBuilders.EventBuilder()
                    .setCategory("Device to Client-ID")
                    .setLabel("Device: " + Build.MODEL + " Client-ID: " + nexmonUID)
                    .setAction("Tracking started")
                    .build());
        }
        return mTracker;
    }

    private static final String TAG = "PenTestSuite";

    private static final String APP_PACKAGE = "de.tu_darmstadt.seemoo.nexmon";

    public static final String WLAN_INTERFACE = "wlan0";

    private static Context context;

    private static ActivityManager activityManager;

    private static PackageManager packageManager;

    private static WindowManager windowManager;

    private static WifiManager wifiManager;

    private static LocationManager locationManager;

    private static TelephonyManager telephonyManager;

    private static AudioManager audioManager;

    private static LayoutInflater layoutInflater;

    private static ConnectivityManager connectivityManager;

    private static SensorManager sensorManager;

    private static Vibrator vibrator;

    private static BluetoothAdapter bluetoothAdapter;

    private static AssetManager assetManager;

    private static FrameReceiver frameReceiver;

    public static boolean dissectionRunning = false;

    public static boolean getRunning = false;

    private static boolean isRawproxyAvailable = false;

    private static boolean isRawproxyreverseAvailable = false;



    private static boolean isBcmFirmwareAvailable = false;


    private static boolean isNexmonFirmwareAvailable = false;

    public static boolean isRootGranted = false;

    private static boolean isMonitorModeAvailable = false;

    private static boolean isInjectionAvailable = false;

    private static boolean isNexutilAvailable = false;

    private static boolean isNexutilNew = false;

    public static boolean isLibInstalledCorrectly() {
        return isLibInstalledCorrectly;
    }

    private static boolean isLibInstalledCorrectly = false;

    public static String getNexmonUID() {
        return nexmonUID;
    }

    private static String nexmonUID;


    //public native void fakeauth(String bssid, String essid, String interfaceName, int reassocTiming, int keepaliveTiming, int packetTiming, String id);

    private static boolean isFrameReceiverRunning = false;
    private static NotificationManager notificationManager;


    public static BluetoothAdapter getBluetoothAdapter() {
        return bluetoothAdapter;

    }



    public static AudioManager getAudioManager() {
        return audioManager;
    }

    public static AssetManager getAssetManager() {
        return assetManager;
    }

    public static Vibrator getVibrator() {

        return vibrator;
    }

    public static ConnectivityManager getConnectivityManager() {
        return connectivityManager;
    }

    public static int calcDevicePixels(int deviceIndependentPixel) {
        return (int) (deviceIndependentPixel * MyApplication.getAppContext().getResources().getDisplayMetrics().density + 0.5f);
    }

    public static Context getAppContext() {
        return context;
    }

    public static ActivityManager getActivityManager() {
        return activityManager;
    }

    public static PackageManager getPackManager() {
        return packageManager;
    }

    public static WindowManager getWindowManager() {
        return windowManager;
    }

    public static WifiManager getWifiManager() {
        return wifiManager;
    }

    public static LocationManager getLocationManager() {
        return locationManager;
    }

    public static TelephonyManager getTelephonyManager() {
        return telephonyManager;
    }

    public static LayoutInflater getLayoutInflater() {
        return layoutInflater;
    }

    public static SensorManager getSensorManager() {
        return sensorManager;
    }

    public static FrameReceiver getFrameReceiver() {
        return frameReceiver;
    }

    public static NotificationManager getNotificationManager() { return notificationManager; }

    public static boolean isRawproxyAvailable() {
        return isRawproxyAvailable;
    }

    public static boolean isRawproxyreverseAvailable() {
        return isRawproxyreverseAvailable;
    }

    public static boolean isNexutilAvailable() {
        return isNexutilAvailable;
    }

    public static boolean isNexmonFirmwareAvailable() {
        return isNexmonFirmwareAvailable;
    }

    public static boolean isBcmFirmwareAvailable() {
        return isBcmFirmwareAvailable;
    }

    public static boolean isMonitorModeAvailable() {
        return isMonitorModeAvailable;
    }

    public static boolean isInjectionAvailable() {
        return isInjectionAvailable;
    }


    /**
     * from: http://stackoverflow.com/a/24754132/1285331
     * Makes a substring of a string bold.
     *
     * @param text       Full text
     * @param textToBold Text you want to make bold
     * @return String with bold substring
     */
    public static SpannableStringBuilder makeSectionOfTextBold(String text, String textToBold) {

        SpannableStringBuilder builder = new SpannableStringBuilder();

        if (textToBold.length() > 0 && !textToBold.trim().equals("")) {

            //for counting start/end indexes
            String testText = text.toLowerCase(Locale.US);
            String testTextToBold = textToBold.toLowerCase(Locale.US);
            int startingIndex = testText.indexOf(testTextToBold);
            int endingIndex = startingIndex + testTextToBold.length();
            //for counting start/end indexes

            if (startingIndex < 0 || endingIndex < 0) {
                return builder.append(text);
            } else if (startingIndex >= 0 && endingIndex >= 0) {

                builder.append(text);
                builder.setSpan(new StyleSpan(Typeface.BOLD), startingIndex, endingIndex, 0);
            }
        } else {
            return builder.append(text);
        }

        return builder;
    }

    public static SpannableStringBuilder appendWithColor(SpannableStringBuilder builder, String appendText, int color) {
        int start = builder.length();
        builder.append(appendText);
        int end = builder.length();
        builder.setSpan(new ForegroundColorSpan(color), start, end, 0);
        return builder;
    }

    /*
     * from: http://stackoverflow.com/a/28721045/1285331
     */
    public static int hex2decimal(String s) {
        String digits = "0123456789ABCDEF";
        s = s.toUpperCase();
        int val = 0;
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            int d = digits.indexOf(c);
            val = 16 * val + d;
        }
        return val;
    }

    public static void deleteTempFile() {
        try {
            String fileP = MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).getString("reader", "");
            if (fileP != null && !fileP.equals("") && fileP.startsWith(Environment.getExternalStorageDirectory().getAbsolutePath()+ "/tmp_")) {
                File file = new File(fileP);
                file.delete();
            }
            MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).edit().remove("reader").commit();
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    public native static int wiresharkInit();

    public native static void wiresharkTestGetAll();

    @Override
    public void onCreate() {

        super.onCreate();
        MyApplication.context = getApplicationContext();
        activityManager = (ActivityManager) MyApplication.getAppContext().getSystemService(Context.ACTIVITY_SERVICE);
        packageManager = MyApplication.getAppContext().getPackageManager();
        windowManager = (WindowManager) MyApplication.getAppContext().getSystemService(Context.WINDOW_SERVICE);
        wifiManager = (WifiManager) MyApplication.getAppContext().getSystemService(WIFI_SERVICE);
        locationManager = (LocationManager) MyApplication.getAppContext().getSystemService(LOCATION_SERVICE);
        telephonyManager = (TelephonyManager) MyApplication.getAppContext().getSystemService(Context.TELEPHONY_SERVICE);
        layoutInflater = (LayoutInflater) MyApplication.getAppContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        connectivityManager = (ConnectivityManager) MyApplication.getAppContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        sensorManager = (SensorManager) MyApplication.getAppContext().getSystemService(SENSOR_SERVICE);
        vibrator = (Vibrator) MyApplication.getAppContext().getSystemService(VIBRATOR_SERVICE);
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        audioManager = (AudioManager) MyApplication.getAppContext().getSystemService(Context.AUDIO_SERVICE);
        notificationManager = (NotificationManager) MyApplication.getAppContext().getSystemService(NOTIFICATION_SERVICE);
        assetManager = context.getAssets();

        if (MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.NEXMON_ID", Context.MODE_PRIVATE).contains("nexmon_id")) {
            nexmonUID = MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.NEXMON_ID", Context.MODE_PRIVATE).getString("nexmon_id", "NO_ID");
        } else {
            nexmonUID = UUID.randomUUID().toString();
            MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.NEXMON_ID", Context.MODE_PRIVATE).edit().putString("nexmon_id", nexmonUID).commit();
        }

        initLibs();
        MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).edit().clear().commit();

        frameReceiver = new FrameReceiver();

        startService(new Intent(getAppContext(), MonitorModeService.class));
        startService(new Intent(getAppContext(), WiresharkService.class));
        startService(new Intent(getAppContext(), APfinderService.class));
        startService(new Intent(getAppContext(), AttackService.class));
        startService(new Intent(getAppContext(), RawSocketReceiveService.class));

       // showSurveyNotification();
    }

    public void initLibs() {
        String r = "";

        try {
            System.loadLibrary("wireshark_helper");
            System.loadLibrary("nexmonpentestsuite");
            isLibInstalledCorrectly = true;

            if (wiresharkInit() != 1)
                r += "Failed to initialize wireshark library...\n";

        } catch (UnsatisfiedLinkError e) {
            isLibInstalledCorrectly = false;
        }

        Log.d(TAG, r);
    }



    public static void evaluateAll() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                evaluateInstallation();
                evaluateBCMfirmware();
            }
        }).start();


    }

    private static void evaluateCapabilities() {

        try {
            Command command = new Command(0, "ifconfig wlan0 up", "nexutil -g 400") {

                boolean tempMonitor = false;
                boolean tempInjection = false;
                boolean tempNexmonFirmware = false;
                @Override
                public void commandOutput(int id, String line) {
                    if(line.contains("0x000000: 07")) {
                        tempMonitor = true;
                        tempInjection = true;
                        tempNexmonFirmware = true;
                    } else if(line.contains("0x000000: 03") || line.contains("0x000000: 01")) {
                        tempMonitor = true;
                        tempInjection = false;
                        tempNexmonFirmware = true;
                    } else if(line.contains("__nex_driver_io: error")) {
                        tempMonitor = false;
                        tempInjection = false;
                        tempNexmonFirmware = false;
                    }

                    super.commandOutput(id, line);
                }


                @Override
                public void commandCompleted(int id, int exitcode) {
                    isMonitorModeAvailable = tempMonitor;
                    isInjectionAvailable = tempInjection;
                    isNexmonFirmwareAvailable = tempNexmonFirmware;
                    super.commandCompleted(id, exitcode);
                }
            };
            RootTools.getShell(true).add(command);
        } catch(Exception e) {e.printStackTrace();}


    }

    public static void evaluateBCMfirmware() {
        try {
            Command command = new Command(0, "ifconfig wlan0 up", "nexutil -g 0") {

                boolean temp = false;
                @Override
                public void commandOutput(int id, String line) {
                    if(line.contains("77 6c e4 14"))
                        temp = true;

                    super.commandOutput(id, line);
                }

                @Override
                public void commandCompleted(int id, int exitCode) {
                    isBcmFirmwareAvailable = temp;
                    evaluateCapabilities();
                    super.commandCompleted(id, exitCode);
                }
            };
            RootTools.getShell(true).add(command);
        } catch(Exception e) {e.printStackTrace();}
    }




    public static SpannableStringBuilder getInstallInfo() {
        SpannableStringBuilder ssBuilder = new SpannableStringBuilder();

        if(!isRootGranted) {
            ssBuilder.append("We need root to proceed!", new ForegroundColorSpan(Color.RED), 0);
            return ssBuilder;
        }

        if(MyApplication.getAppContext().checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ssBuilder.append("We need read / write permission for your storage!", new ForegroundColorSpan(Color.RED), 0);
            return ssBuilder;
        }

        ssBuilder.append("Tools installation status:\n\n", new StyleSpan(Typeface.BOLD), 0);

        if(isNexutilAvailable) {
            ssBuilder.append("nexutil\n", new ForegroundColorSpan(Color.GREEN), 0);
            if(!isNexutilNew)
                ssBuilder.append(" (Please make sure to update nexutil to the newest version!)\n", new ForegroundColorSpan(Color.RED), 0);
        } else
            ssBuilder.append("nexutil\n", new ForegroundColorSpan(Color.RED), 0);

        if(isRawproxyAvailable)
            ssBuilder.append("rawproxy\n", new ForegroundColorSpan(Color.GREEN), 0);
        else
            ssBuilder.append("rawproxy\n", new ForegroundColorSpan(Color.RED), 0);

        if(isRawproxyreverseAvailable)
            ssBuilder.append("rawproxyreverse\n\n", new ForegroundColorSpan(Color.GREEN), 0);
        else
            ssBuilder.append("rawproxyreverse\n\n", new ForegroundColorSpan(Color.RED), 0);

        if(!isNexutilAvailable || !isRawproxyAvailable || !isRawproxyreverseAvailable)
            ssBuilder.append("You have to install all required tools in order to use nexmon. You can do so by clicking the menu button at the upper left corner and select \"Tools\".\n\n");

        ssBuilder.append("Firmware installation status:\n\n", new StyleSpan(Typeface.BOLD), 0);



        if(isNexutilAvailable) {

            if(isBcmFirmwareAvailable) {
                if(isNexmonFirmwareAvailable) {
                    ssBuilder.append("Nexmon Firmware", new ForegroundColorSpan(Color.GREEN), 0);
                } else {
                    ssBuilder.append("Nexmon Firmware\n\n", new ForegroundColorSpan(Color.RED), 0);
                    ssBuilder.append("You have to install the nexmon firmware in order to use this app. You can do so by clicking the menu button at the upper left corner and select \"Firmware\".\n\n");
                }
            } else {
                ssBuilder.append("Your device is not supported by nexmon!\n\n", new ForegroundColorSpan(Color.RED), 0);
            }


        } else {
            ssBuilder.append("We can't search for the nexmon firmware without nexutil.", new ForegroundColorSpan(Color.RED), 0);
        }

        return ssBuilder;
    }

    public static void evaluateInstallation() {
        isRawproxyAvailable = isBinaryAvailable("rawproxy");
        isRawproxyreverseAvailable = isBinaryAvailable("rawproxyreverse");
        isNexutilAvailable = isBinaryAvailable("nexutil");
        isNexutilNew = isNexutilNew();
    }

    public static boolean isBinaryAvailable(String binary) {
        boolean isAvailable = true;
        String[] cmdline = { "sh", "-c", "type " + binary };
        try {
            Process p = Runtime.getRuntime().exec(cmdline);
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(p.getInputStream()));

            String line;
            while ((line = in.readLine()) != null) {
                if(line.contains("not found"))
                    isAvailable = false;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return isAvailable;
    }

    public static boolean isNexutilNew() {
        boolean isNew = false;
        String[] cmdline = { "nexutil", "--version"};
        try {
            Process p = Runtime.getRuntime().exec(cmdline);
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(p.getInputStream()));

            String line;
            while ((line = in.readLine()) != null) {
                if(line.contains("1.0-"))
                    isNew = true;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return isNew;
    }

    public static void toast(String msg) {
        try {
            Toast.makeText(MyApplication.getAppContext(), msg, Toast.LENGTH_SHORT).show();
        } catch(Exception e) {e.printStackTrace();}
    }

    public static void showSurveyNotification() {
        Uri webpage = Uri.parse("http://survey.seemoo.tu-darmstadt.de/limesurvey/index.php/465539?N00=" + nexmonUID);

        Intent intent = new Intent(Intent.ACTION_VIEW, webpage);

        PendingIntent pendingIntent = PendingIntent.getActivity(getAppContext(), 99999, intent, 0);

        Notification n  = new Notification.Builder(getAppContext())
                .setContentTitle("Nexmon Survey")
                .setContentText("Help us to improve Nexmon and take a short survey!")
                .setAutoCancel(false)
                .setSmallIcon(R.drawable.x_logo)
                .setOngoing(false)
                .setContentIntent(pendingIntent)
                .build();

        getNotificationManager().notify(99999, n);
    }

}