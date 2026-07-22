package de.tu_darmstadt.seemoo.nexmon.gui;

import android.app.Fragment;
import android.content.Intent;
import android.content.res.AssetManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.nononsenseapps.filepicker.FilePickerActivity;
import com.roger.catloadinglibrary.CatLoadingView;
import com.stericson.RootShell.exceptions.RootDeniedException;
import com.stericson.RootShell.execution.Command;
import com.stericson.RootTools.RootTools;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.TimeoutException;

import de.tu_darmstadt.seemoo.nexmon.FirmwareUtils;
import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;


public class FirmwareFragment extends Fragment implements View.OnClickListener {

    private static final int UPDATE_TV_FIRMWARE_VERSION = 50;
    private static final int UPDATE_BUTTON_ENABLED = 51;
    private static final int GUI_SHOW_LOADING = 52;
    private static final int GUI_DISMISS_LOADING = 53;
    private static final int GUI_SHOW_TOAST = 54;
    private static final int GUI_UPDATE_TV_FIRMWARE_PATH = 55;

    private static final int COMMAND_RESTART_WLAN = 101;
    private static final int COMMAND_BACKUP_FIRMWARE = 102;
    private static final int COMMAND_FIRMWARE_VERSION = 103;
    private static final int COMMAND_FIRMWARE_RESTORE = 104;

    private static final int FIRMWARE_PATH = 201;

    private Button btnCreateFirmwareBackup;
    private Button btnInstallNexmonFirmware;
    private Button btnRestoreFirmwareBackup;
    private Button btnSelectFirmware;
    private Button btnSearchFirmware;

    private String sdCardPath;
    private Spinner spnDevice;

    private TextView tvFirmwareVersionOutput;
    private TextView tvFirmwarePath;

    private CatLoadingView loadingView;
    private Handler guiHandler;

    private String mountPoint = "";
    private String fwPathEnd = "";
    private String fwNameBeginning = "";
    private String fwNameEnd = "";

    private boolean firmwareFound = false;

    public FirmwareFragment() {
        sdCardPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/";
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @return A new instance of fragment.
     */
    public static FirmwareFragment newInstance() {
        FirmwareFragment fragment = new FirmwareFragment();
        return fragment;
    }


    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        guiHandler = new Handler() {

            @SuppressWarnings("unchecked")
            public void handleMessage(Message msg) {
                super.handleMessage(msg);

                switch (msg.what) {
                    case UPDATE_TV_FIRMWARE_VERSION:
                        tvFirmwareVersionOutput.setText((String) msg.obj);
                        break;
                    case UPDATE_BUTTON_ENABLED:
                        setContentVisibility();
                        break;
                    case GUI_SHOW_LOADING:
                        loadingView = new CatLoadingView();
                        loadingView.setCancelable(false);
                        loadingView.show(getFragmentManager(), "");
                        break;
                    case GUI_DISMISS_LOADING:
                        loadingView.dismiss();
                        break;
                    case GUI_SHOW_TOAST:
                        Toast.makeText(MyApplication.getAppContext(), (String) msg.obj, Toast.LENGTH_SHORT).show();
                        break;
                    case GUI_UPDATE_TV_FIRMWARE_PATH:
                        tvFirmwarePath.setText((String) msg.obj);
                        evaluateFirmware();
                        break;
                    default:
                        break;


                }
            }
        };

        try {
            getActivity().setTitle("Nexmon: Firmware");
        } catch(Exception e) {e.printStackTrace();}


    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        if (container != null) {
            container.removeAllViews();
        }

        View view = inflater.inflate(R.layout.fragment_nexmon, container, false);

        getGuiElements(view);

        setContentVisibility();
        return view;
    }



    private void toast(String msg) {
        try {
            Message message = guiHandler.obtainMessage();
            message.what = GUI_SHOW_TOAST;
            message.obj = msg;
            guiHandler.sendMessage(message);
        } catch(Exception e) {e.printStackTrace();}
    }

    private void toastLogcatError() {
        toast("Sorry, there was an error, find out more about it using logcat");
    }

    private void getGuiElements(View view) {

        btnCreateFirmwareBackup = (Button) view.findViewById(R.id.btnCreateFirmwareBackup);
        btnInstallNexmonFirmware = (Button) view.findViewById(R.id.btnInstallNexmonFirmware);

        btnRestoreFirmwareBackup = (Button) view.findViewById(R.id.btnRestoreFirmwareBackup);
        btnSelectFirmware = (Button) view.findViewById(R.id.btnSelectFirmware);
        btnSearchFirmware = (Button) view.findViewById(R.id.btnSearchFirmware);


        tvFirmwarePath = (TextView) view.findViewById(R.id.tvFirmwarePath);
        tvFirmwareVersionOutput = (TextView) view.findViewById(R.id.tvFirmwareVersionOutput);
        spnDevice = (Spinner) view.findViewById(R.id.spnDevice);

        btnInstallNexmonFirmware.setOnClickListener(this);
        btnRestoreFirmwareBackup.setOnClickListener(this);
        btnCreateFirmwareBackup.setOnClickListener(this);

        btnSelectFirmware.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                selectFile(FIRMWARE_PATH);
            }
        });

        btnSearchFirmware.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String path = FirmwareUtils.getFirmwarePathComplete();
                if(path != null) {
                    Message msg = guiHandler.obtainMessage(GUI_UPDATE_TV_FIRMWARE_PATH, path);
                    guiHandler.sendMessage(msg);
                }
            }
        });

        spnDevice.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                //evaluateFirmware();
                //onClickPrintFirmwareVersion(fwPathEnd + fwNameEnd);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        String Model = android.os.Build.MODEL;
        if(Model.contains(FirmwareUtils.DEVICE_NEXUS6P)) {
            spnDevice.setSelection(2);
        } else if(Model.contains(FirmwareUtils.DEVICE_NEXUS5)) {
            spnDevice.setSelection(1);
        } else if(Model.contains(FirmwareUtils.DEVICE_SGS2)) {
            spnDevice.setSelection(0);
        }
        evaluateFirmware();
    }

    private void setContentVisibility() {
        if(!(new File(fwPathEnd + fwNameEnd).exists())) {
            btnCreateFirmwareBackup.setEnabled(false);
            btnRestoreFirmwareBackup.setEnabled(false);
            btnInstallNexmonFirmware.setEnabled(false);
        }else if ((new File(sdCardPath + "nexmon/" + fwNameEnd + ".bac")).exists()) {
            btnCreateFirmwareBackup.setEnabled(false);
            btnInstallNexmonFirmware.setEnabled(true);
            btnRestoreFirmwareBackup.setEnabled(true);
        } else {
            btnCreateFirmwareBackup.setEnabled(true);
            btnRestoreFirmwareBackup.setEnabled(false);
            btnInstallNexmonFirmware.setEnabled(false);
        }
    }



    public void onClickCreateFirmwareBackup() {
        evaluateFirmware();
        final Command command = new Command(COMMAND_BACKUP_FIRMWARE, "cp " + fwPathEnd + fwNameEnd + " " + sdCardPath + "nexmon/" + fwNameEnd + ".bac") {

            @Override
            public void commandOutput(int id, String line) {
                toast(line);
                super.commandOutput(id, line);
            }

            @Override
            public void commandCompleted(int id, int exitcode) {
                guiHandler.sendEmptyMessage(UPDATE_BUTTON_ENABLED);
                super.commandCompleted(id, exitcode);
            }
        };

        new Thread(new Runnable() {
            @Override
            public void run() {

                try {
                    RootTools.getShell(true).add(command);

                guiHandler.sendEmptyMessage(GUI_SHOW_LOADING);
                Thread.sleep(3000);
                guiHandler.sendEmptyMessage(GUI_DISMISS_LOADING);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }).start();


        setContentVisibility();
    }

    public void onClickRestoreFirmwareBackup() {
        evaluateFirmware();
        Command command = new Command(COMMAND_FIRMWARE_RESTORE, "mount -o rw,remount " + mountPoint, "cp " + sdCardPath + "nexmon/" + fwNameEnd + ".bac " + fwPathEnd + fwNameEnd) {
            @Override
            public void commandOutput(int id, String line) {
                if(id == COMMAND_FIRMWARE_RESTORE)
                    toast(line);
                super.commandOutput(id, line);
            }

            @Override
            public void commandCompleted(int id, int exitcode) {
                if(id == COMMAND_FIRMWARE_RESTORE)
                    toast("Restore finished!");

                super.commandCompleted(id, exitcode);
            }
        };
        try {
            RootTools.getShell(true).add(command);
        } catch (Exception e) {
            e.printStackTrace();
            toastLogcatError();
        }
    }

    public void onClickInstallNexmonFirmware() {
        final Command command = new Command(COMMAND_RESTART_WLAN, "ifconfig wlan0 down", "ifconfig wlan0 up") {
            @Override
            public void commandCompleted(int id, int exitcode) {
                if(id == COMMAND_RESTART_WLAN) {
                    MyApplication.evaluateAll();
                }
                super.commandCompleted(id, exitcode);
            }
        };
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    evaluateFirmware();
                    guiHandler.sendEmptyMessage(GUI_SHOW_LOADING);

                    extractAssets();
                    copyExtractedAsset(fwPathEnd, fwNameBeginning);
                    //Log.e("INSTALL PATH", "fwPathEnd: " + fwPathEnd + " fwNameEnd: " + fwNameEnd + " fwNameBeginning: " + fwNameBeginning);
                    RootTools.getShell(true).add(command);
                    Thread.sleep(5000);
                    guiHandler.sendEmptyMessage(GUI_DISMISS_LOADING);
                } catch(Exception e) {e.printStackTrace();}
            }
        }).start();
    }


    private void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        while (in.read(buffer) != -1) {
            out.write(buffer);
        }
    }

    private void extractAssets() throws IOException {
        AssetManager assetManager = MyApplication.getAssetManager();
        String[] files = null;
        files = assetManager.list("nexmon");
        File folder = new File(sdCardPath + "/nexmon");
        if (!folder.exists()) folder.mkdir();

        if (files != null) for (String filename : files) {
            InputStream in = null;
            OutputStream out = null;
            in = assetManager.open("nexmon/" + filename);
            File outFile = new File(sdCardPath + "/nexmon", filename);
            out = new FileOutputStream(outFile);
            copyFile(in, out);
            if (in != null) in.close();
            if (out != null) out.close();
        }
    }

    private void copyExtractedAsset(String installLocation, String filename) throws TimeoutException, IOException, RootDeniedException {

        RootTools.getShell(true).add(new Command(0, "mount -o rw,remount " + mountPoint,
                "cp " + sdCardPath + "nexmon/" + filename + " " + installLocation + fwNameEnd,
                "chmod 755 " + installLocation + fwNameEnd) {

            @Override
            public void commandOutput(int id, String line) {
                toast(line);
                super.commandOutput(id, line);
            }
        });
    }


    public void onClickPrintFirmwareVersion(String fullPath) {
        final Command command = new Command(COMMAND_FIRMWARE_VERSION, "strings " + fullPath + " | grep \"CRC:\"") {

            boolean fwidFound = false;

            @Override
            public void commandOutput(int id, String line) {
                if(id == COMMAND_FIRMWARE_VERSION) {
                    String out;

                    fwidFound = true;
                    try {
                        String radioChip = line.substring(0, 10);
                        String rChip[] = radioChip.split("[a-zA-Z]+");
                        String radioId = "bcm" + rChip[0];

                        String rVersion[] = line.split(" Version: ");
                        String rVersion2[] = rVersion[1].split(" ");
                        String radioVersion = rVersion2[0].trim();
                        out = "Radio chip: " + radioId + "\nFirmware version: " + radioVersion + "\n";
                    } catch(Exception e) {
                        e.printStackTrace();
                        out = line + "\n";
                    }
                    Message msg = guiHandler.obtainMessage(UPDATE_TV_FIRMWARE_VERSION, out);
                    guiHandler.sendMessage(msg);
                }
                super.commandOutput(id, line);
            }
            
            @Override
            public void commandCompleted(int id, int exitcode) {
                super.commandCompleted(id, exitcode);
                if(!fwidFound) {
                    Message msg = guiHandler.obtainMessage(UPDATE_TV_FIRMWARE_VERSION, "Not found!\nAre you sure that this file is the right one?!");
                    guiHandler.sendMessage(msg);
                }
            }
        };
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    RootTools.getShell(true).add(command);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }).start();
    }
    private void evaluateFirmware() {
        String item = spnDevice.getSelectedItem().toString();

        String fwInfo[] = new String[4];

        if(item.startsWith("BCM4330 "))
            fwInfo = getResources().getStringArray(R.array.bcm4330);
        else if(item.startsWith("BCM4339 "))
            fwInfo = getResources().getStringArray(R.array.bcm4339);
        else if(item.startsWith("BCM4358 "))
            fwInfo = getResources().getStringArray(R.array.bcm4358);

        //String pathAndFilename[] = FirmwareUtils.getFirmwarePath();

        mountPoint = fwInfo[0];
        //fwPathEnd = pathAndFilename[0];
        fwNameBeginning = fwInfo[2];
        //fwNameEnd = pathAndFilename[1];

        String fullPath = tvFirmwarePath.getText().toString();

        if(fullPath.contains("/")) {
            int index = fullPath.lastIndexOf("/");
            fwPathEnd = fullPath.substring(0, index + 1);
            fwNameEnd = fullPath.substring(index + 1);
            onClickPrintFirmwareVersion(fullPath);
        } else {
            fwPathEnd = "";
            fwNameEnd = "";
        }
        setContentVisibility();
    }

    @Override
    public void onClick(View v) {
        switch(v.getId()) {
            case R.id.btnInstallNexmonFirmware:
                onClickInstallNexmonFirmware();
                break;
            case R.id.btnRestoreFirmwareBackup:
                onClickRestoreFirmwareBackup();
                break;
            case R.id.btnCreateFirmwareBackup:
                onClickCreateFirmwareBackup();
                break;
            default:
                break;
        }
    }

    private void selectFile(int requestCode) {
        Intent i = new Intent(Intent.ACTION_GET_CONTENT);

        i.putExtra(FilePickerActivity.EXTRA_ALLOW_MULTIPLE, false);
        i.putExtra(FilePickerActivity.EXTRA_ALLOW_CREATE_DIR, false);
        i.putExtra(FilePickerActivity.EXTRA_MODE, FilePickerActivity.MODE_FILE);
        i.putExtra(FilePickerActivity.EXTRA_START_PATH, "/vendor/firmware/");

        startActivityForResult(i, requestCode);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if(requestCode == FIRMWARE_PATH && data != null && data.getData() != null) {
            String path = data.getData().getPath();
            if(path != null && !path.equals("")) {
                Message msg = guiHandler.obtainMessage(GUI_UPDATE_TV_FIRMWARE_PATH, path);
                guiHandler.sendMessage(msg);
            }
        }
    }
}
