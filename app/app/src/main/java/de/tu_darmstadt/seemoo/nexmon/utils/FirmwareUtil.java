package de.tu_darmstadt.seemoo.nexmon.utils;

import android.util.Log;

import java.util.List;

import eu.chainfire.libsuperuser.Shell;

/**
 * Created by matthias on 30.06.17.
 */

public class FirmwareUtil {
    private static FirmwareUtil instance;

    private FirmwareUtil() {
        instance = this;
    }
	
    public static FirmwareUtil getInstance() {
    	return instance == null ? new FirmwareUtil() : instance;
    }

    public static boolean isBroadcomChip() {
        Shell.SU.run("ifconfig wlan0 up");
        return Nexutil.getInstance().getIntIoctl(0) == 0x14e46c77;
	}

    public static String getCapabilities() {
        Shell.SU.run("ifconfig wlan0 up");
        String capabilities = Nexutil.getInstance().getIoctl(400);
        return capabilities;
    }
}
