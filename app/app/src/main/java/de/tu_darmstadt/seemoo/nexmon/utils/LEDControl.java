package de.tu_darmstadt.seemoo.nexmon.utils;

import android.graphics.Color;

import java.util.List;

import eu.chainfire.libsuperuser.Shell;

/**
 * Created by matthias on 30.05.17.
 */

public class LEDControl {
    private static final String LED_GREEN_PATH = "/sys/class/leds/green";
    private static final String LED_BLUE_PATH = "/sys/class/leds/blue";
    private static final String LED_RED_PATH = "/sys/class/leds/red";

    private static final String LED_BRIGHTNESS_FILE = "brightness";
    private static final String LED_ON_OFF_MS_FILE = "on_off_ms";
    private static final String LED_RGB_START_FILE = "rgb_start";

    private static int getBrightness(String led_path) {
        List<String> out = Shell.SU.run("cat " + led_path + "/" + LED_BRIGHTNESS_FILE);
        int ret;

        try {
            ret = Integer.parseInt(out.toString());
        } catch (NumberFormatException e) {
            ret = -1;
        }

        return ret;
    }

    public static int getBrightnessGreen() {
        return getBrightness(LED_GREEN_PATH);
    }

    public static int getBrightnessRed() {
        return getBrightness(LED_RED_PATH);
    }

    public static int getBrightnessBlue() {
        return getBrightness(LED_BLUE_PATH);
    }

    private static void setBrightness(String led_path, int value) {
        Shell.SU.run("echo " + value + " > " + led_path + "/" + LED_BRIGHTNESS_FILE);
    }

    public static void setBrightnessGreen(int value) {
        setBrightness(LED_GREEN_PATH, value);
    }

    public static void setBrightnessRed(int value) {
        setBrightness(LED_RED_PATH, value);
    }

    public static void setBrightnessBlue(int value) {
        setBrightness(LED_BLUE_PATH, value);
    }

    public static void setBrightnessRGB(int r, int g, int b) {
        setBrightness(LED_RED_PATH, r);
        setBrightness(LED_GREEN_PATH, g);
        setBrightness(LED_BLUE_PATH, b);
    }

    public static void setBrightnessRGB(int rgb) {
        setBrightnessRGB((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
    }

    private static int[] getOnOffMs(String led_path) {
        List<String> out = Shell.SU.run("cat " + led_path + "/" + LED_ON_OFF_MS_FILE);
        int ret[] = new int[2];

        String arr[] = out.toString().split(" ");

        try {
            ret[0] = Integer.parseInt(arr[0]);
            ret[1] = Integer.parseInt(arr[1]);
        } catch (NumberFormatException e) {
            ret[0] = -1;
            ret[1] = -1;
        }

        return ret;
    }

    public static int[] getOnOffMsGreen() {
        return getOnOffMs(LED_GREEN_PATH);
    }

    public static int[] getOnOffMsRed() {
        return getOnOffMs(LED_RED_PATH);
    }

    public static int[] getOnOffMsBlue() {
        return getOnOffMs(LED_BLUE_PATH);
    }

    private static void setOnOffMs(String led_path, int on, int off) {
        Shell.SU.run("echo " + on + " " + off + " > " + led_path + "/" + LED_ON_OFF_MS_FILE);
    }

    public static void setOnOffMsGreen(int on, int off) {
        setOnOffMs(LED_GREEN_PATH, on, off);
    }

    public static void setOnOffMsRed(int on, int off) {
        setOnOffMs(LED_RED_PATH, on, off);
    }

    public static void setOnOffMsBlue(int on, int off) {
        setOnOffMs(LED_BLUE_PATH, on, off);
    }

    public static void setOnOffMsRGB(int on, int off) {
        setOnOffMsRed(on, off);
        setOnOffMsGreen(on, off);
        setOnOffMsBlue(on, off);
    }

    public static void activateLED() {
        Shell.SU.run("echo 0 > " + LED_GREEN_PATH + "/" + LED_RGB_START_FILE);
        Shell.SU.run("echo 1 > " + LED_GREEN_PATH + "/" + LED_RGB_START_FILE);
    }

    public static void deactivateLED() {
        Shell.SU.run("echo 0 > " + LED_GREEN_PATH + "/" + LED_RGB_START_FILE);
    }
}
