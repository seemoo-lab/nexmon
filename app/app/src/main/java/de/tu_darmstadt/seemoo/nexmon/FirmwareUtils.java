package de.tu_darmstadt.seemoo.nexmon;

import android.support.annotation.Nullable;

import java.io.File;

/**
 * Created by fabian on 1/9/17.
 */

public class FirmwareUtils {
    public static final String DEVICE_NEXUS5 = "Nexus 5";
    public static final String DEVICE_NEXUS6P = "Nexus 6P";
    public static final String DEVICE_SGS2 = "GT-I9100";

    public static String getFirmwareName() {
        return "";
    }

    /**
     * Search firmware path and filename.
     *
     * @return array of path and name or null if nothing found.
     */
    @Nullable
    public static String[] getFirmwarePath() {
        String fwPath[], fwName[];
        String returnValue[] = new String[2];

        fwPath = MyApplication.getAppContext().getResources().getStringArray(R.array.firmware_paths);
        fwName = MyApplication.getAppContext().getResources().getStringArray(R.array.firmware_names);
        File fwData;

        for(String path : fwPath) {
            for(String name : fwName) {
                fwData = new File(path + name);
                if(fwData.exists()) {
                    returnValue[0] = path;
                    returnValue[1] = name;
                    return returnValue;
                }
            }

        }

        return null;
    }

    /**
     * Search firmware path and filename.
     *
     * @return Concated path and name, null if not found.
     */
    @Nullable
    public static String getFirmwarePathComplete() {
        String fwPath[], fwName[];
        String returnValue[] = new String[2];

        fwPath = MyApplication.getAppContext().getResources().getStringArray(R.array.firmware_paths);
        fwName = MyApplication.getAppContext().getResources().getStringArray(R.array.firmware_names);
        File fwData;

        for(String path : fwPath) {
            for(String name : fwName) {
                fwData = new File(path + name);
                if(fwData.exists()) {
                    return path + name;
                }
            }

        }

        return null;
    }
}
