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
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.Spinner;
import android.widget.Toast;

import com.nononsenseapps.filepicker.FilePickerActivity;
import com.roger.catloadinglibrary.CatLoadingView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;
import de.tu_darmstadt.seemoo.nexmon.sharky.PcapFileReader;

/**
 * Created by fabian on 8/25/16.
 */
public class AirdecapFragment extends Fragment {

    private static final int SHOW_LOADING = 80;
    private static final int DISMISS_LOADING = 81;
    private static final int SHOW_TOAST = 82;
    private static final int UPDATE_SPINNER = 83;

    private static final int CHOOSE_FILE = 12;
    public static final int CRYPT_WEP = 1;
    public static final int CRYPT_WPA = 2;

    private Button btnSelectFile;
    private Button btnDecrypt;
    private Button btnScanAP;
    private Spinner spinnerSelectAp;
    private EditText edittextPassphrase;
    private EditText edittextBSSID;
    private EditText edittextSSID;
    private RadioButton radiobtnWep;
    private RadioButton radiobtnWpa;


    private PcapFileReader reader;
    private HashMap<String, String> bssidToSsid = new HashMap<String, String>();
    private String fileDir;
    private Handler guiHandler;
    private CatLoadingView loadingView;

    private String bssid = "";
    private String ssid = "";
    private String passphrase = "";
    private int encryption = CRYPT_WPA;



    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        guiHandler = new Handler() {

            @SuppressWarnings("unchecked")
            public void handleMessage(Message msg) {
                super.handleMessage(msg);

                switch (msg.what) {
                    case SHOW_LOADING:
                        loadingView = new CatLoadingView();
                        loadingView.setCancelable(false);
                        loadingView.show(getFragmentManager(), "");
                        break;
                    case DISMISS_LOADING:
                        loadingView.dismiss();
                        break;
                    case SHOW_TOAST:
                        showToast((String) msg.obj);
                        break;
                    case UPDATE_SPINNER:
                        updateSpinner();
                        break;
                }
            }
        };

        try {
            getActivity().setTitle("Nexmon: Airdecap");
        } catch(Exception e) {e.printStackTrace();}
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        if (container != null) {
            container.removeAllViews();
        }

        View view = inflater.inflate(R.layout.fragment_airdecap, container, false);

        btnSelectFile = (Button) view.findViewById(R.id.btn_select_file);
        spinnerSelectAp = (Spinner) view.findViewById(R.id.spinner_ap);
        edittextBSSID = (EditText) view.findViewById(R.id.edittext_bssid);
        edittextSSID = (EditText) view.findViewById(R.id.edittext_ssid);
        edittextPassphrase = (EditText) view.findViewById(R.id.edittext_passphrase);
        btnDecrypt = (Button) view.findViewById(R.id.btn_decrypt);
        btnScanAP = (Button) view.findViewById(R.id.btn_scan_ap);
        radiobtnWep = (RadioButton) view.findViewById(R.id.radiobtn_wep);
        radiobtnWpa = (RadioButton) view.findViewById(R.id.radiobtn_wpa);

        btnScanAP.setEnabled(false);

        btnSelectFile.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                selectFile(CHOOSE_FILE);
            }
        });

        btnDecrypt.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                passphrase = edittextPassphrase.getText().toString();
                bssid = edittextBSSID.getText().toString();
                ssid = edittextSSID.getText().toString();

                if(radiobtnWpa.isChecked())
                    encryption = CRYPT_WPA;
                else
                    encryption = CRYPT_WEP;

                startDecrypt(bssid, ssid, passphrase, encryption);
            }
        });

        btnScanAP.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                scanForAPs();
            }
        });

        spinnerSelectAp.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                evaluateSpinner();
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });

        spinnerSelectAp.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                evaluateSpinner();
                return false;
            }
        });

        return view;
    }

    private void selectFile(int requestCode) {
        Intent i = new Intent(Intent.ACTION_GET_CONTENT);

        i.putExtra(FilePickerActivity.EXTRA_ALLOW_MULTIPLE, false);
        i.putExtra(FilePickerActivity.EXTRA_ALLOW_CREATE_DIR, true);
        i.putExtra(FilePickerActivity.EXTRA_MODE, FilePickerActivity.MODE_FILE);
        i.putExtra(FilePickerActivity.EXTRA_START_PATH, Environment.getExternalStorageDirectory().getPath());

        startActivityForResult(i, requestCode);
    }



    private void evaluateSpinner() {
        String selectedItem = (String) spinnerSelectAp.getSelectedItem();
        if(selectedItem != null && !selectedItem.equals("")) {
            String[] item = selectedItem.split("-", 2);
            bssid = item[0].trim();
            ssid = item[1].trim();

            edittextBSSID.setText(bssid);
            edittextSSID.setText(ssid);
        }
    }

    private void scanForAPs() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                guiHandler.sendEmptyMessage(SHOW_LOADING);
                int packetAmount = reader.getAmountOfPackets();
                int scanStart = 1;
                int scanAmount = 500;

                while(packetAmount >= scanStart) {
                    ArrayList<Packet> packets = reader.getPackets(scanStart, scanAmount);
                    for(Packet packet : packets) {
                        if(packet.isBeacon()) {
                            String bssid = packet.getBSSIDfromBeacon();

                            String ssid = packet.getSSIDfromBeacon();
                            if(ssid != null && !ssid.equals(""))
                                bssidToSsid.put(bssid, ssid);

                        }
                    }
                    scanStart += scanAmount;
                }

                guiHandler.sendEmptyMessage(UPDATE_SPINNER);
                guiHandler.sendEmptyMessage(DISMISS_LOADING);
            }
        }).start();
    }

    private void startDecrypt(final String bssid,final String ssid, final String passphrase, final int encryption) {
        if(fileDir != null && bssid != null && ssid != null && passphrase != null) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    guiHandler.sendEmptyMessage(SHOW_LOADING);

                    // TODO should be static access.
                    Packet packet = new Packet(Packet.LinkType.IEEE_802_11_WLAN_RADIOTAP);
                    packet.decrypt(bssid, ssid, passphrase, fileDir, encryption);

                    guiHandler.sendEmptyMessage(DISMISS_LOADING);

                    String fileDirDecrypted = fileDir.substring(0, fileDir.length() - 5) + "-dec.pcap";

                    Message msg = new Message();
                    msg.what = SHOW_TOAST;
                    msg.obj = fileDirDecrypted;
                    guiHandler.sendMessage(msg);

                }
            }).start();
        } else {
            MyApplication.toast("Have you selected a file?");
        }

    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, final Intent data) {
        if (requestCode == CHOOSE_FILE && data != null && data.getData() != null) {
            try {
                fileDir = data.getData().getPath();
                reader = new PcapFileReader(fileDir);
                btnScanAP.setEnabled(true);
            } catch(Exception e) {
                e.printStackTrace();
                MyApplication.toast(e.getMessage());
                fileDir = null;
            }

        }
    }

    private void updateSpinner() {

        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(getActivity(),
                android.R.layout.simple_spinner_item, getSpinnerEntry());
        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerSelectAp.setAdapter(dataAdapter);
    }


    public ArrayList<String> getSpinnerEntry() {
        ArrayList<String> spinnerList = new ArrayList<String>();

        Iterator it = bssidToSsid.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry pair = (Map.Entry)it.next();
            spinnerList.add(pair.getKey() + " - " + pair.getValue());
            it.remove(); // avoids a ConcurrentModificationException
        }
        return spinnerList;
    }

    public void showToast(String text) {
        Toast.makeText(getActivity(), text,
                Toast.LENGTH_LONG).show();
    }

}
