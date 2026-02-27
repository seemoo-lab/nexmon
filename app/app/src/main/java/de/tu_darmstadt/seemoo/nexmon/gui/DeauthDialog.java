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
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;

import com.google.gson.Gson;

import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;
import de.tu_darmstadt.seemoo.nexmon.stations.DeAuthAttack;

public class DeauthDialog extends AttackDialog {

    Spinner spinner;
    Button btnDeauthStart;
    EditText etAmount;
    ArrayAdapter<String> stationsAdapter;
    Attack attack;

    public DeauthDialog() {
    }

    public static DeauthDialog newInstance(AccessPoint ap) {
        DeauthDialog fragment = new DeauthDialog();
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
        View view = inflater.inflate(R.layout.deauth_dialog, container);


        getDialog().setTitle("Deauth Attack");

        spinner = (Spinner) view.findViewById(R.id.spinner_stations);
        btnDeauthStart = (Button) view.findViewById(R.id.button_start_deauth);
        etAmount = (EditText) view.findViewById(R.id.et_deauth_amount);


        ArrayList<String> stations = new ArrayList<String>();

        stations.addAll(ap.getStations().keySet());

        stationsAdapter = new ArrayAdapter<String>(getActivity(), android.R.layout.simple_spinner_item, stations);
        stationsAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(stationsAdapter);

        btnDeauthStart.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                String stationMac = (String) spinner.getSelectedItem();
                if(stationMac != null && !stationMac.equals("")) {
                    int amount;
                    try {
                        amount = Integer.valueOf(etAmount.getText().toString());
                    } catch(Exception e) {
                        e.printStackTrace();
                        MyApplication.toast("Invalid amount! Set amount to 500.");
                        amount = 500;
                    }

                    attack = new DeAuthAttack(ap, stationMac.trim(), amount);
                    String ob = new Gson().toJson(attack);
                    Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
                    intent.putExtra("ATTACK", ob);
                    intent.putExtra("ATTACK_TYPE", Attack.ATTACK_DE_AUTH);
                    MyApplication.getAppContext().sendBroadcast(intent);
                    dismiss();
                }
            }
        });

        return view;
    }


}
