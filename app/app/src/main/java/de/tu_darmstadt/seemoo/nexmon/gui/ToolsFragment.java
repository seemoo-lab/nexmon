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

package de.tu_darmstadt.seemoo.nexmon.gui;

import android.app.Fragment;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Environment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.Toast;

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

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;


public class ToolsFragment extends Fragment {

    private final static int GUI_SHOW_TOAST = 111;
    private final static int GUI_SHOW_LOADING = 112;
    private final static int GUI_DISMISS_LOADING = 113;

	private String sdCardPath;
    private CatLoadingView loadingView;
    private Handler guiHandler;
    private CheckBox chkRawproxy;
    private CheckBox chkRawproxyreverse;
    private CheckBox chkNexutil;
    private CheckBox chkDhdutil;
    private CheckBox chkTcpdump;
    private CheckBox chkAircrack;
    private CheckBox chkLibfakeioctl;
    private CheckBox chkNetcat;
    private CheckBox chkIw;
    private CheckBox chkWirelessTools;
    private CheckBox chkMdk3;
    private CheckBox chkSocat;
    private Spinner spnBinInstallLocation;
    private Spinner spnLibInstallLocation;
    private Button btnInstall;

    public ToolsFragment() {
    	sdCardPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/";
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @return A new instance of fragment.
     */
    public static ToolsFragment newInstance() {
    	ToolsFragment fragment = new ToolsFragment();
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
                    case GUI_SHOW_TOAST:
                        try {
                            Toast.makeText(getActivity(), (String) msg.obj, Toast.LENGTH_SHORT).show();
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                        break;
                    case GUI_SHOW_LOADING:
                        loadingView = new CatLoadingView();
                        loadingView.setCancelable(false);
                        loadingView.show(getFragmentManager(), "");
                        break;
                    case GUI_DISMISS_LOADING:
                        loadingView.dismiss();
                        break;
                    default:
                        break;
                }
            }
        };

        try {
            getActivity().setTitle("Nexmon: Tools");
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_tools, container, false);

        chkRawproxy = (CheckBox) view.findViewById(R.id.chkRawproxy);
        chkRawproxyreverse = (CheckBox) view.findViewById(R.id.chkRawproxyreverse);
        chkNexutil = (CheckBox) view.findViewById(R.id.chkNexutil);
        chkDhdutil = (CheckBox) view.findViewById(R.id.chkDhdutil);
        chkTcpdump = (CheckBox) view.findViewById(R.id.chkTcpdump);
        chkAircrack = (CheckBox) view.findViewById(R.id.chkAircrack);
        chkLibfakeioctl = (CheckBox) view.findViewById(R.id.chkLibfakeioctl);
        chkNetcat = (CheckBox) view.findViewById(R.id.chkNetcat);
        chkIw = (CheckBox) view.findViewById(R.id.chkIw);
        chkWirelessTools = (CheckBox) view.findViewById(R.id.chkWirelessTools);
        chkMdk3 = (CheckBox) view.findViewById(R.id.chkMdk3);
        chkSocat = (CheckBox) view.findViewById(R.id.chkSocat);
        spnBinInstallLocation = (Spinner) view.findViewById(R.id.spnBinInstallLocation);
        spnLibInstallLocation = (Spinner) view.findViewById(R.id.spnLibInstallLocation);
        btnInstall = (Button) view.findViewById(R.id.btnInstall);

        initializeInstallLocationSpinners();

        btnInstall.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onClickInstall();
            }
        });

        return view;
    }

    private void initializeInstallLocationSpinners() {
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(MyApplication.getAppContext(),
                R.array.binInstallLocations, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spnBinInstallLocation.setAdapter(adapter);


        adapter = ArrayAdapter.createFromResource(MyApplication.getAppContext(),
                R.array.libInstallLocations, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spnLibInstallLocation.setAdapter(adapter);

        String Model = android.os.Build.MODEL;
        if(Model.contains("Nexus 6P")) {
            spnBinInstallLocation.setSelection(4);  // set /su/xbin
            spnLibInstallLocation.setSelection(2);  // set /su/lib
        } else if(Model.contains("Nexus 5")) {
            spnBinInstallLocation.setSelection(0);  // set /system/bin
            spnLibInstallLocation.setSelection(0);  // set /system/lib
    	}
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
        if(!folder.exists()) folder.mkdir();

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

    private void copyExtractedAsset(final String installLocation, final String filename) throws TimeoutException, IOException, RootDeniedException {
        RootTools.getShell(true).add(new Command(0, "mount -o rw,remount /system", "mount -o rw,remount /", //root for /su path
                "rm -f " + installLocation + "/" + filename,
                "cp " + sdCardPath + "nexmon/" + filename + " " + installLocation,
                "chmod 755 " + installLocation + "/" + filename) {

            @Override
            public void commandCompleted(int id, int exitcode) {
                super.commandCompleted(id, exitcode);
                File file = new File(installLocation + "/" + filename);
                if(!file.exists()) {
                    toast("ERROR: Can't install to " + installLocation + "/" + filename);
                }
            }
        });
    }

    public void onClickInstall() {
        final String binInstallLocation = spnBinInstallLocation.getSelectedItem().toString();
        final String libInstallLocation = spnLibInstallLocation.getSelectedItem().toString();

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    guiHandler.sendEmptyMessage(GUI_SHOW_LOADING);
                    extractAssets();

                    if (chkRawproxy.isChecked()) {
                        //toast("Installing rawproxy ...");
                        copyExtractedAsset(binInstallLocation, "rawproxy");
                    }

                    if (chkRawproxyreverse.isChecked()) {
                        //toast("Installing rawproxyreverse ...");
                        copyExtractedAsset(binInstallLocation, "rawproxyreverse");
                    }

                    if (chkDhdutil.isChecked()) {
                        //toast("Installing dhdutil ...");
                        copyExtractedAsset(binInstallLocation, "dhdutil");
                    }

                    if (chkNexutil.isChecked()) {
                        //toast("Installing nexutil ...");
                        copyExtractedAsset(binInstallLocation, "nexutil");
                    }

                    if (chkTcpdump.isChecked()) {
                        //toast("Installing tcpdump ...");
                        copyExtractedAsset(binInstallLocation, "tcpdump");
                    }

                    if (chkLibfakeioctl.isChecked()) {
                        //toast("Installing tcpdump ...");
                        copyExtractedAsset(libInstallLocation, "libfakeioctl.so");
                    }

                    if (chkAircrack.isChecked()) {
                        //toast("Installing aircrack-ng suite ...");
                        copyExtractedAsset(binInstallLocation, "airbase-ng");
                        copyExtractedAsset(binInstallLocation, "aircrack-ng");
                        copyExtractedAsset(binInstallLocation, "airdecap-ng");
                        copyExtractedAsset(binInstallLocation, "airdecloak-ng");
                        copyExtractedAsset(binInstallLocation, "aireplay-ng");
                        copyExtractedAsset(binInstallLocation, "airodump-ng");
                        copyExtractedAsset(binInstallLocation, "airolib-ng");
                        copyExtractedAsset(binInstallLocation, "airserv-ng");
                        copyExtractedAsset(binInstallLocation, "airtun-ng");
                        copyExtractedAsset(binInstallLocation, "besside-ng");
                        copyExtractedAsset(binInstallLocation, "buddy-ng");
                        copyExtractedAsset(binInstallLocation, "easside-ng");
                        copyExtractedAsset(binInstallLocation, "ivstools");
                        copyExtractedAsset(binInstallLocation, "kstats");
                        copyExtractedAsset(binInstallLocation, "makeivs-ng");
                        copyExtractedAsset(binInstallLocation, "packetforge-ng");
                        copyExtractedAsset(binInstallLocation, "tkiptun-ng");
                        copyExtractedAsset(binInstallLocation, "wesside-ng");
                        copyExtractedAsset(binInstallLocation, "wpaclean");
                    }

                    if (chkNetcat.isChecked()) {
                        //toast("Installing netcat ...");
			copyExtractedAsset(binInstallLocation, "nc");
                    }

                    if (chkIw.isChecked()) {
                        //toast("Installing iw ...");
                        copyExtractedAsset(binInstallLocation, "iw");
                    }

                    if (chkWirelessTools.isChecked()) {
                        //toast("Installing wireless-tools ...");
                        copyExtractedAsset(binInstallLocation, "iwconfig");
                        copyExtractedAsset(binInstallLocation, "iwlist");
                        copyExtractedAsset(binInstallLocation, "iwpriv");
                    }

                    if (chkMdk3.isChecked()) {
                        //toast("Installing mdk3 ...");
                        copyExtractedAsset(binInstallLocation, "mdk3");
                    }

                    if (chkSocat.isChecked()) {
                        //toast("Installing mdk3 ...");
                        copyExtractedAsset(binInstallLocation, "socat");
                    }

                    Thread.sleep(3000);
                    MyApplication.evaluateAll();
                } catch (Exception e) {
                    e.printStackTrace();
                } finally {
                    guiHandler.sendEmptyMessage(GUI_DISMISS_LOADING);
                }
            }
        }).start();
    }

    private void toast(String msg) {
        Message message = guiHandler.obtainMessage();
        message.what = GUI_SHOW_TOAST;
        message.obj = msg;
        guiHandler.sendMessage(message);
    }
}
