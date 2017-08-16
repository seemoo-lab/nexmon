package de.tu_darmstadt.seemoo.nexmon.utils;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashSet;
import java.util.Set;

import eu.chainfire.libsuperuser.Shell;

/**
 * Created by matthias on 31.05.17.
 */

public class Assets {
    private static Set<String> upToDateFiles = new HashSet<String>();

    public static int copyFileFromAsset(Context context, String sourcePath, String destPath) {
        byte[] buff = new byte[1024];
        int len;
        InputStream in;
        OutputStream out;
        try {
            in = context.getAssets().open(sourcePath);
            File tmpFile = File.createTempFile("tmp", "file", context.getCacheDir());
            out = new FileOutputStream(tmpFile);
            // write file
            while ((len = in.read(buff)) != -1) {
                out.write(buff, 0, len);
            }
            in.close();
            out.flush();
            out.close();

            Shell.SU.run("cp " + tmpFile.getAbsolutePath() + " " + destPath);
        } catch (Exception ex) {
            ex.printStackTrace();
            return -1;
        }
        return 0;
    }

    public static String getAssetsPath(Context context, String filename) {
        byte[] buff = new byte[1024];
        int len;
        InputStream in;
        OutputStream out;

        String filesPath = context.getFilesDir().getPath();

        if (new File(filesPath + "/" + filename).isFile() && upToDateFiles.contains(filename)) {
            Log.d("ASSETS", filename + " found");
            return filesPath + "/" + filename;
        } else {
            try {
                in = context.getAssets().open("nexmon/" + filename);
                File outFile = new File(filesPath, filename);

                out = new FileOutputStream(outFile);

                // write file
                while ((len = in.read(buff)) != -1) {
                    out.write(buff, 0, len);
                }

                in.close();
                out.flush();
                out.close();

                Shell.SU.run("chmod 777 " + filesPath + "/" + filename);

                upToDateFiles.add(filename);

                Log.d("ASSETS", filename + " extracted");
                return filesPath + "/" + filename;
            } catch (Exception ex) {
                ex.printStackTrace();
                return null;
            }
        }
    }
}
