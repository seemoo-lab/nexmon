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


import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Spinner;

import com.google.gson.Gson;

import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.ArpReplayAttack;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;

/**
 * A simple {@link Fragment} subclass.
 * Use the {@link ArpReplayDialog#newInstance} factory method to
 * create an instance of this fragment.
 */
public class ArpReplayDialog extends AttackDialog {

    private Button btnArpStart;
    private Spinner spinnerStations;
    private CheckBox cbUseArpFile;
    private Button btnSelectArpFile;
    private ArrayAdapter<String> stationsAdapter;
    private Attack attack;
    private static final int CHOOSE_FILE = 15;

    private String arpFileDir = "/sdcard/default.arp";



    public ArpReplayDialog() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @param ap Which AccessPoint to attack.
     * @return A new instance of fragment ArpReplayDialog.
     */
    // TODO: Rename and change types and number of parameters
    public static ArpReplayDialog newInstance(AccessPoint ap) {
        ArpReplayDialog fragment = new ArpReplayDialog();
        Bundle args = new Bundle();
        args.putString(ACCESSPOINT, new Gson().toJson(ap));
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            ap = new Gson().fromJson(getArguments().getString(ACCESSPOINT), AccessPoint.class);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_arp_replay_dialog, container, false);

        getDialog().setTitle("ARP Replay Attack");

        btnArpStart = (Button) view.findViewById(R.id.btn_start_arpreplay);
        spinnerStations = (Spinner) view.findViewById(R.id.spinner_arpreplay_stations);
      //  btnSelectArpFile = (Button) view.findViewById(R.id.btn_arpreplay_arp_file);
      //  cbUseArpFile = (CheckBox) view.findViewById(R.id.cb_arpreplay_use_arp_file);

        ArrayList<String> stations = new ArrayList<String>();

        stations.addAll(ap.getStations().keySet());

        stationsAdapter = new ArrayAdapter<>(getActivity(), android.R.layout.simple_spinner_item, stations);
        stationsAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerStations.setAdapter(stationsAdapter);

      /*  cbUseArpFile.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked)
                    btnSelectArpFile.setActivated(true);
                else
                    btnSelectArpFile.setActivated(false);
            }
        });

        btnSelectArpFile.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent fileIntent = new Intent(Intent.ACTION_GET_CONTENT);
                fileIntent.setType("file/*");
                startActivityForResult(fileIntent, CHOOSE_FILE);
            }
        });*/

        btnArpStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    String station = ((String) spinnerStations.getSelectedItem()).trim();

                    attack = new ArpReplayAttack(ap, station, false, "", false);
                    String ob = new Gson().toJson(attack);
                    Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
                    intent.putExtra("ATTACK", ob);
                    intent.putExtra("ATTACK_TYPE", Attack.ATTACK_ARP_REPLAY);
                    MyApplication.getAppContext().sendBroadcast(intent);
                    dismiss();
                } catch(Exception e) {e.printStackTrace();}
            }
        });

        return view;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, final Intent data) {
        try {
            if (requestCode == CHOOSE_FILE && data != null && data.getData() != null) {
                arpFileDir = data.getData().getPath();
            }
        } catch(Exception e) {e.printStackTrace();}
    }

}
