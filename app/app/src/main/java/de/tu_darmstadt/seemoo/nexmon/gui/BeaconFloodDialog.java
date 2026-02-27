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

import android.app.DialogFragment;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.google.gson.Gson;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;
import de.tu_darmstadt.seemoo.nexmon.stations.BeaconFloodAttack;

/**
 * Created by fabian on 9/21/16.
 */
public class BeaconFloodDialog extends DialogFragment{

        Button btnBeaconFloodStart;
;
        Attack attack;

        public BeaconFloodDialog() {
        }

        public static BeaconFloodDialog newInstance() {
            BeaconFloodDialog fragment = new BeaconFloodDialog();
            return fragment;
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
        }




        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                                 Bundle savedInstanceState) {
            View view = inflater.inflate(R.layout.beaconflood_dialog, container);


            getDialog().setTitle("BeaconFlood Attack");

            btnBeaconFloodStart = (Button) view.findViewById(R.id.button_start_beaconflood);


            btnBeaconFloodStart.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                        AccessPoint ap = new AccessPoint("FF:FF:FF:FF:FF:FF");
                        attack = new BeaconFloodAttack(ap);
                        String ob = new Gson().toJson(attack);
                        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
                        intent.putExtra("ATTACK", ob);
                        intent.putExtra("ATTACK_TYPE", Attack.ATTACK_BEACON_FLOOD);
                        MyApplication.getAppContext().sendBroadcast(intent);
                        dismiss();

                }
            });

            return view;
        }




}
