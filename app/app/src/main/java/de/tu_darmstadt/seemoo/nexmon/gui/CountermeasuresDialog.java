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
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;

import com.google.gson.Gson;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;
import de.tu_darmstadt.seemoo.nexmon.stations.CountermeasuresAttack;

/**
 * Created by fabian on 9/23/16.
 */
public class CountermeasuresDialog extends AttackDialog {

    private Button btnStartCountermeasures;
    private EditText etSpeed;
    private EditText etPacketsPerBurst;
    private EditText etPacketPause;
    private CheckBox cbTkipExploit;

    private Attack attack;

    public static CountermeasuresDialog newInstance(AccessPoint ap) {
        CountermeasuresDialog fragment = new CountermeasuresDialog();
        Bundle args = new Bundle();
        args.putString(ACCESSPOINT,  new Gson().toJson(ap));
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
        View view = inflater.inflate(R.layout.fragment_countermeasures_dialog, container, false);

        etSpeed = (EditText) view.findViewById(R.id.et_speed);
        etPacketsPerBurst = (EditText) view.findViewById(R.id.et_packets_per_burst);
        etPacketPause = (EditText) view.findViewById(R.id.et_burst_pause);
        btnStartCountermeasures = (Button) view.findViewById(R.id.btn_start_countermeasures);
        cbTkipExploit = (CheckBox) view.findViewById(R.id.cb_tkip_qos_exploit);

        btnStartCountermeasures.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int speed = Integer.valueOf(etSpeed.getText().toString());
                int packetsPerBurst = Integer.valueOf(etPacketsPerBurst.getText().toString());
                int packetPause = Integer.valueOf(etPacketPause.getText().toString());

                attack = new CountermeasuresAttack(ap, packetPause, packetsPerBurst, speed, cbTkipExploit.isChecked());
                String ob = new Gson().toJson(attack);
                Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
                intent.putExtra("ATTACK", ob);
                intent.putExtra("ATTACK_TYPE", Attack.ATTACK_COUNTERMEASURES);
                MyApplication.getAppContext().sendBroadcast(intent);
                dismiss();
            }
        });
        return view;
    }
}
