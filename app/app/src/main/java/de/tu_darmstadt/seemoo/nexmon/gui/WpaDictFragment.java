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
import android.widget.Spinner;
import android.widget.Toast;

import com.google.gson.Gson;
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
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;
import de.tu_darmstadt.seemoo.nexmon.stations.WpaDictAttack;

/**
 * Created by fabian on 10/1/16.
 */
public class WpaDictFragment extends Fragment {

    private static final int HANDLER_SHOW_LOADING = 1;
    private static final int HANDLER_DISMISS_LOADING = 2;
    private static final int HANDLER_UPDATE_SPINNER = 3;
    private static final int HANDLER_SHOW_TOAST = 4;
    private static final int REQ_CODE_SRC1 = 10;
    private static final int REQ_CODE_SRC2 = 11;
    private static EditText etDictHashFile;
    private static EditText etPcapFile;
    private static EditText etEssid;
    private static Button btnSelectDictHashFile;
    private static Button btnSelectPcapFile;
    private static Button btnStart;
    private static Button btnScanAp;
    private Spinner spinnerAp;
    private CatLoadingView loadingView;
    private String dictHashFile;
    private String pcapFile;
    private Attack attack;
    private HashMap<String, String> bssidToSsid = new HashMap<String, String>();
    private Handler guiHandler;
    public WpaDictFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @return A new instance of fragment AttackInfoFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static WpaDictFragment newInstance() {
        WpaDictFragment fragment = new WpaDictFragment();
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setupGuiHandler();



    }

    private void setupGuiHandler() {
        guiHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                try {
                    switch(msg.what) {
                        case HANDLER_SHOW_LOADING:
                            loadingView = new CatLoadingView();
                            loadingView.setCancelable(false);
                            loadingView.show(getFragmentManager(), "");
                            break;
                        case HANDLER_DISMISS_LOADING:
                            loadingView.dismiss();
                            break;
                        case HANDLER_UPDATE_SPINNER:
                            updateSpinner();
                            break;
                        case HANDLER_SHOW_TOAST:
                            try {
                                Toast.makeText(getActivity(), (String) msg.obj, Toast.LENGTH_SHORT).show();
                            } catch(Exception e) {e.printStackTrace();}
                            break;
                        default:
                            break;
                    }
                } catch(Exception e) {e.printStackTrace();}
            }
        };
    }



    @Override
    public void onResume() {
        super.onResume();

    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        if (container != null) {
            container.removeAllViews();
        }
        View view = inflater.inflate(R.layout.fragment_wpadict, container, false);

        etDictHashFile = (EditText) view.findViewById(R.id.et_wpadict_dicthashfile);
        etPcapFile = (EditText) view.findViewById(R.id.et_wpadict_pcapfile);
        etEssid = (EditText) view.findViewById(R.id.et_wpadict_ssid);
        btnSelectDictHashFile = (Button) view.findViewById(R.id.btn_wpadict_dicthashfile);
        btnSelectPcapFile = (Button) view.findViewById(R.id.btn_wpadict_pcapfile);
        btnStart = (Button) view.findViewById(R.id.btn_wpadict_start);
        btnScanAp = (Button) view.findViewById(R.id.btn_wpadict_scan_ap);
        spinnerAp = (Spinner) view.findViewById(R.id.spinner_wpadict_ap);

        btnScanAp.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                pcapFile = etPcapFile.getText().toString();
                scanForAPs();
            }
        });

        btnSelectDictHashFile.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                selectFile(REQ_CODE_SRC1);
            }
        });

        btnSelectPcapFile.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                selectFile(REQ_CODE_SRC2);
            }
        });

        btnStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startAttack();
            }
        });

        spinnerAp.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                evaluateSpinner();
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });

        spinnerAp.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                evaluateSpinner();
                return false;
            }
        });

        return view;
    }

    private void startAttack() {

        AccessPoint ap = new AccessPoint("FF:FF:FF:FF:FF:FF");
        ap.setSsid(etEssid.getText().toString());
        attack = new WpaDictAttack(ap, etDictHashFile.getText().toString(), false, etPcapFile.getText().toString());
        String ob = new Gson().toJson(attack);
        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
        intent.putExtra("ATTACK", ob);
        intent.putExtra("ATTACK_TYPE", Attack.ATTACK_WPA_DICT);
        MyApplication.getAppContext().sendBroadcast(intent);

    }

    private void evaluateSpinner() {
        String selectedItem = (String) spinnerAp.getSelectedItem();
        if(selectedItem != null && !selectedItem.equals("")) {
            String[] item = selectedItem.split("-", 2);
            String ssid = item[1].trim();

            etEssid.setText(ssid);
        }
    }

    private void selectFile(int requestCode) {
        Intent i = new Intent(Intent.ACTION_GET_CONTENT);

        i.putExtra(FilePickerActivity.EXTRA_ALLOW_MULTIPLE, false);
        i.putExtra(FilePickerActivity.EXTRA_ALLOW_CREATE_DIR, true);
        i.putExtra(FilePickerActivity.EXTRA_MODE, FilePickerActivity.MODE_FILE);
        i.putExtra(FilePickerActivity.EXTRA_START_PATH, Environment.getExternalStorageDirectory().getPath());

        startActivityForResult(i, requestCode);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, final Intent data) {
        if (requestCode == REQ_CODE_SRC1 && data != null && data.getData() != null) {
            dictHashFile = data.getData().getPath();
            etDictHashFile.setText(dictHashFile);
        }
        else if (requestCode == REQ_CODE_SRC2 && data != null && data.getData() != null) {
            pcapFile = data.getData().getPath();
            etPcapFile.setText(pcapFile);
        }
    }

    private void updateSpinner() {

        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(getActivity(),
                android.R.layout.simple_spinner_item, getSpinnerEntry());
        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerAp.setAdapter(dataAdapter);

    }

    private void scanForAPs() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                guiHandler.sendEmptyMessage(HANDLER_SHOW_LOADING);
                try {
                    if(isPcapFile()) {
                        PcapFileReader reader = new PcapFileReader(pcapFile);
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
                    } else {
                        Message msg = guiHandler.obtainMessage();
                        msg.what = HANDLER_SHOW_TOAST;
                        msg.obj = "Not a valid pcap file!";
                        guiHandler.sendMessage(msg);
                    }
                } catch(Exception e) {
                    e.printStackTrace();
                    Message msg = guiHandler.obtainMessage();
                    msg.what = HANDLER_SHOW_TOAST;
                    msg.obj = e.getMessage();
                    guiHandler.sendMessage(msg);
                }

                guiHandler.sendEmptyMessage(HANDLER_UPDATE_SPINNER);
                guiHandler.sendEmptyMessage(HANDLER_DISMISS_LOADING);
            }
        }).start();
    }

    private boolean isPcapFile() {
        if(pcapFile == null)
            return false;
        String tmp = pcapFile.toLowerCase();
        return (tmp.endsWith(".pcap") || tmp.endsWith(".cap"));
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

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        try {
            getActivity().setTitle("Nexmon: WPA Dict Attack");
        } catch(Exception e) {e.printStackTrace();}
    }
}
