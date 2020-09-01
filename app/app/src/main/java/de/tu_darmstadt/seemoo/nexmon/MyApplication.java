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
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.graphics.Typeface;
import android.hardware.SensorManager;
import android.location.LocationManager;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Environment;
import android.os.Vibrator;
import android.preference.PreferenceManager;
import android.support.annotation.Nullable;
import android.telephony.TelephonyManager;
import android.text.SpannableStringBuilder;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.WindowManager;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.List;
import java.util.Locale;
import java.util.UUID;

import de.tu_darmstadt.seemoo.nexmon.net.FrameReceiver;
import de.tu_darmstadt.seemoo.nexmon.net.MonitorModeService;
import de.tu_darmstadt.seemoo.nexmon.net.RawSocketReceiveService;
import de.tu_darmstadt.seemoo.nexmon.sharky.WiresharkService;
import de.tu_darmstadt.seemoo.nexmon.stations.APfinderService;
import de.tu_darmstadt.seemoo.nexmon.stations.AttackService;
import de.tu_darmstadt.seemoo.nexmon.utils.Dhdutil;
import de.tu_darmstadt.seemoo.nexmon.utils.Nexutil;
import eu.chainfire.libsuperuser.Shell;


public class MyApplication extends Application {

    public static boolean isAppVisible = false;

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

    private static boolean isNexutilNew = false;

    private static String nexutilVersion;

    private static String rawproxyVersion;

    private static String rawproxyreverseVersion;




    private static boolean isBcmFirmwareAvailable = false;

    private static String firmwareVersion;


    private static boolean isNexmonFirmwareAvailable = false;

    public static boolean isRootGranted = false;

    private static boolean isMonitorModeAvailable = false;

    private static boolean isInjectionAvailable = false;

    private static boolean isNexutilAvailable = false;


    public static boolean isLibInstalledCorrectly() {
        return isLibInstalledCorrectly;
    }

    private static boolean isLibInstalledCorrectly = false;

    public static String getNexmonUID() {
        return nexmonUID;
    }

    private static String nexmonUID;

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

    public static boolean isNexutilNew() {
        return isNexutilNew;
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
        Nexutil.getInstance();
        Dhdutil.getInstance();
        activityManager = (ActivityManager) MyApplication.getAppContext().getSystemService(Context.ACTIVITY_SERVICE);
        packageManager = MyApplication.getAppContext().getPackageManager();
        windowManager = (WindowManager) MyApplication.getAppContext().getSystemService(Context.WINDOW_SERVICE);
        wifiManager = (WifiManager) MyApplication.getAppContext().getApplicationContext().getSystemService(WIFI_SERVICE);
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
                evaluateFirmwareVersion();
                rawproxyreverseVersion = getVersion("rawproxyreverse");
                rawproxyVersion = getVersion("rawproxy");
                nexutilVersion = getVersion("nexutil");
            }
        }).start();


    }

    private static void evaluateCapabilities() {
        boolean tempMonitor = false;
        boolean tempInjection = false;
        boolean tempNexmonFirmware = false;

        Shell.SU.run("ifconfig wlan0 up");
        String line = Nexutil.getInstance().getIoctl(400);

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

        isMonitorModeAvailable = tempMonitor;
        isInjectionAvailable = tempInjection;
        isNexmonFirmwareAvailable = tempNexmonFirmware;
    }

    public static void evaluateBCMfirmware() {
        Shell.SU.run("ifconfig wlan0 up");
        isBcmFirmwareAvailable = Nexutil.getInstance().getIoctl(0).contains("77 6c e4 14");
        evaluateCapabilities();
    }

    public static void evaluateFirmwareVersion() {
        Shell.SU.run("ifconfig wlan0 up");
        String line = Nexutil.getInstance().getIoctl(413);

        if (line.contains("nexmon_ver")) {
            try {
                firmwareVersion = line.substring(12, line.lastIndexOf('-'));
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public static String getFirmwareVersion() {
        return firmwareVersion;
    }

    public static void evaluateInstallation() {
        isRawproxyAvailable = isBinaryAvailable("rawproxy");
        isRawproxyreverseAvailable = isBinaryAvailable("rawproxyreverse");
        isNexutilAvailable = isBinaryAvailable("nexutil");
        evaluateNexutilVersion();
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

    public static void evaluateNexutilVersion() {
        boolean isNew = false;
        String[] cmdline = { "nexutil", "--version"};
        try {
            Process p = Runtime.getRuntime().exec(cmdline);
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(p.getInputStream()));

            String line;
            while ((line = in.readLine()) != null) {
                if(line.contains(BuildConfig.VERSION_NAME))
                    isNew = true;
            }
            isNexutilNew = isNew;
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    @Nullable
    public static String getVersion(String program) {
        String[] cmdline = { program, "--version"};
        try {
            Process p = Runtime.getRuntime().exec(cmdline);
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(p.getInputStream()));

            String line;
            while ((line = in.readLine()) != null) {
                return line;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    public static boolean isFirmwareNew(String firmwarePath) {
        // strings fw.bin | grep nexmon_ver
        boolean isNew = false;
        String[] cmdline = {"strings " + firmwarePath + " | grep nexmon_ver"};
        try {
            Process p = Runtime.getRuntime().exec(cmdline);
            BufferedReader in = new BufferedReader(
                    new InputStreamReader(p.getInputStream()));

            String line;
            while ((line = in.readLine()) != null) {
                if(line.contains(BuildConfig.VERSION_NAME))
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
}
