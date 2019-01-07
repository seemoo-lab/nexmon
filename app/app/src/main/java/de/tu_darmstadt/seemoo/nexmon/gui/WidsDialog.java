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

import com.google.gson.Gson;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;
import de.tu_darmstadt.seemoo.nexmon.stations.WidsAttack;

/**
 * Created by fabian on 9/24/16.
 */
public class WidsDialog extends AttackDialog {

    Button btnStart;
    CheckBox cbChanHop;
    CheckBox cbZeroChaos;


    Attack attack;

    public WidsDialog() {
    }

    public static WidsDialog newInstance(AccessPoint ap) {
        WidsDialog fragment = new WidsDialog();
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
        View view = inflater.inflate(R.layout.wids_dialog, container);


        getDialog().setTitle("WIDS Attack");

        btnStart = (Button) view.findViewById(R.id.btn_start_wids);
        cbChanHop = (CheckBox) view.findViewById(R.id.cb_wids_chanhop);
        cbZeroChaos = (CheckBox) view.findViewById(R.id.cb_wids_zerochaos);


        btnStart.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                attack = new WidsAttack(ap, cbChanHop.isChecked(), cbZeroChaos.isChecked());
                String ob = new Gson().toJson(attack);
                Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
                intent.putExtra("ATTACK", ob);
                intent.putExtra("ATTACK_TYPE", Attack.ATTACK_WIDS);
                MyApplication.getAppContext().sendBroadcast(intent);
                dismiss();

            }
        });

        return view;
    }


}