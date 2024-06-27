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
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
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
import de.tu_darmstadt.seemoo.nexmon.sharky.IvsFileReader;
import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;
import de.tu_darmstadt.seemoo.nexmon.sharky.PcapFileReader;
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;
import de.tu_darmstadt.seemoo.nexmon.stations.WepCrackAttack;

/**
 * A simple {@link Fragment} subclass.
 * Use the {@link AircrackWepFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class AircrackWepFragment extends Fragment implements Attack.IAttackInstanceUpdate{


    private static final int CHOOSE_FILE = 12;
    private static final int SHOW_LOADING = 80;
    private static final int DISMISS_LOADING = 81;
    private static final int SHOW_TOAST = 82;
    private static final int UPDATE_SPINNER = 83;
    private static final int BUTTON_DISABLE = 84;
    private static final int BUTTON_ENABLE = 85;

    private CatLoadingView loadingView;

    private Spinner spinnerAp;
    private EditText etBssid;
    private EditText etSsid;
    private Button btnCrack;
    private Button btnScanAp;
    private Button btnSelectFile;
    private RadioButton rbKorek;
    private CheckBox cbDecloak;
    private Handler guiHandler;

    private Attack attack;


    private HashMap<String, String> bssidToSsid = new HashMap<String, String>();
    private String fileDir;

    public AircrackWepFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @return A new instance of fragment AircrackWepFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static AircrackWepFragment newInstance() {
        AircrackWepFragment fragment = new AircrackWepFragment();
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

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
                    case BUTTON_DISABLE:
                        btnCrack.setEnabled(false);
                        break;
                    case BUTTON_ENABLE:
                        btnCrack.setEnabled(true);
                        break;
                }
            }
        };
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        if (container != null) {
            container.removeAllViews();
        }
        View view = inflater.inflate(R.layout.fragment_aircrack_wep, container, false);

        spinnerAp = (Spinner) view.findViewById(R.id.spinner_wepcrack_ap);
        etBssid = (EditText) view.findViewById(R.id.edittext_wepcrack_bssid);
        etSsid = (EditText) view.findViewById(R.id.edittext_wepcrack_ssid);
        btnCrack = (Button) view.findViewById(R.id.btn_aircrack_wepcrack);
        btnScanAp = (Button) view.findViewById(R.id.btn_wepcrack_scan_ap);
        btnSelectFile = (Button) view.findViewById(R.id.btn_wepcrack_select_file);
        rbKorek = (RadioButton) view.findViewById(R.id.rb_aircrack_korek);
        cbDecloak = (CheckBox) view.findViewById(R.id.cb_aircrack_decloak);

        btnSelectFile.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                selectFile(CHOOSE_FILE);
            }
        });

        btnScanAp.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                try {
                    scanForAPs();
                } catch(Exception e) {e.printStackTrace();}

            }
        });
        btnScanAp.setEnabled(false);
        btnCrack.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
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

    private void selectFile(int requestCode) {
        Intent i = new Intent(Intent.ACTION_GET_CONTENT);

        i.putExtra(FilePickerActivity.EXTRA_ALLOW_MULTIPLE, false);
        i.putExtra(FilePickerActivity.EXTRA_ALLOW_CREATE_DIR, true);
        i.putExtra(FilePickerActivity.EXTRA_MODE, FilePickerActivity.MODE_FILE);
        i.putExtra(FilePickerActivity.EXTRA_START_PATH, Environment.getExternalStorageDirectory().getPath());

        startActivityForResult(i, requestCode);
    }

    @Override
    public void onResume() {
        super.onResume();
        Attack.setObserver(this);
        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_INSTANCE_REQUEST");
        MyApplication.getAppContext().sendBroadcast(intent);

    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        try {
            getActivity().setTitle("Nexmon: Aircrack");
        } catch(Exception e) {e.printStackTrace();}
    }

    @Override
    public void onPause() {
        super.onPause();
        Attack.deleteObserver();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, final Intent data) {
        try {
            if (requestCode == CHOOSE_FILE && data != null && data.getData() != null) {
                fileDir = data.getData().getPath();
                btnScanAp.setEnabled(true);
            }
        } catch(Exception e) {e.printStackTrace();}
    }

    private void evaluateSpinner() {
        String selectedItem = (String) spinnerAp.getSelectedItem();
        if(selectedItem != null && !selectedItem.equals("")) {
            String[] item = selectedItem.split("-", 2);
            String bssid = item[0].trim();
            String ssid = item[1].trim();

            etBssid.setText(bssid);
            etSsid.setText(ssid);
        }
    }

    private boolean isPcapFile() {
        String tmp = fileDir.toLowerCase();
        return (tmp.endsWith(".pcap") || tmp.endsWith(".cap"));
    }

    private boolean isIvsFile() {
        String tmp = fileDir.toLowerCase();
        return tmp.endsWith(".ivs");
    }

    private void scanForAPs() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                guiHandler.sendEmptyMessage(SHOW_LOADING);
                try {
                    if(isPcapFile()) {
                        PcapFileReader reader = new PcapFileReader(fileDir);
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
                    } else if(isIvsFile()) {
                        try {
                            IvsFileReader reader = new IvsFileReader(fileDir);
                            bssidToSsid.clear();
                            bssidToSsid.putAll(reader.getAps());
                        } catch(Exception e) {e.printStackTrace();}
                    } else {
                        Message msg = new Message();
                        msg.what = SHOW_TOAST;
                        msg.obj = "Unknown File Format!";
                        guiHandler.sendMessage(msg);
                    }
                } catch(Exception e) {
                    e.printStackTrace();
                    Message msg = guiHandler.obtainMessage();
                    msg.what = SHOW_TOAST;
                    msg.obj = "Error while reading file!";
                    guiHandler.sendMessage(msg);
                }

                guiHandler.sendEmptyMessage(UPDATE_SPINNER);
                guiHandler.sendEmptyMessage(DISMISS_LOADING);
            }
        }).start();
    }

    private void updateSpinner() {

        ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(getActivity(),
                android.R.layout.simple_spinner_item, getSpinnerEntry());
        dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerAp.setAdapter(dataAdapter);

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

    private void startAttack() {
        if(fileDir != null && !fileDir.equals("")) {
            AccessPoint ap = new AccessPoint(etBssid.getText().toString());
            ap.setSsid(etSsid.getText().toString());
            attack = new WepCrackAttack(ap, true, cbDecloak.isChecked(), rbKorek.isChecked(), fileDir);
            String ob = new Gson().toJson(attack);
            Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
            intent.putExtra("ATTACK", ob);
            intent.putExtra("ATTACK_TYPE", Attack.ATTACK_WEP_CRACK);
            MyApplication.getAppContext().sendBroadcast(intent);
        } else {
            MyApplication.toast("file is not valid!");
        }
    }

    public void showToast(String text) {
        Toast.makeText(getActivity(), text,
                Toast.LENGTH_LONG).show();
    }

    @Override
    public void onAttackInstanceUpdate(HashMap<String, Integer> remainingInstances) {
        if(remainingInstances.containsKey(Attack.ATTACK_WEP_CRACK)){
            if(remainingInstances.get(Attack.ATTACK_WEP_CRACK) > 0)
                guiHandler.sendEmptyMessage(BUTTON_ENABLE);
            else
                guiHandler.sendEmptyMessage(BUTTON_DISABLE);
        }
    }
}
