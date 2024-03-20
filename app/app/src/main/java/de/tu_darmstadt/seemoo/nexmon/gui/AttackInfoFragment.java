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
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.SpannableStringBuilder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.google.gson.Gson;

import java.lang.reflect.Type;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;

/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link AttackInfoFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link AttackInfoFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class AttackInfoFragment extends Fragment {
    // TODO: Rename parameter arguments, choose names that match
    // the fragment initialization parameters, e.g. ARG_ITEM_NUMBER
    private static final String ARG_ATTACK = "ATTACK";
    private static final String ARG_ATTACK_TYPE = "ATTACK_TYPE";

    private static final int HANDLER_UPDATE_TEXT = 1;

    private TextView tvInfoHead;
    private TextView tvInfo;

    private SpannableStringBuilder info;

    private BroadcastReceiver attackUpdateReceiver;
    // TODO: Rename and change types of parameters
    private Attack attack;

    private Handler guiHandler;

    public AttackInfoFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @param param1 Parameter 1.
     * @param param2 Parameter 2.
     * @return A new instance of fragment AttackInfoFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static AttackInfoFragment newInstance(Attack attack) {
        AttackInfoFragment fragment = new AttackInfoFragment();
        Bundle args = new Bundle();
        args.putString(ARG_ATTACK, new Gson().toJson(attack));
        args.putString(ARG_ATTACK_TYPE, attack.getTypeString());
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            String typeString = getArguments().getString(ARG_ATTACK_TYPE);
            attack = new Gson().fromJson(getArguments().getString(ARG_ATTACK), Attack.ATTACK_TYPE.get(typeString));
        }

        setupGuiHandler();
        setupBroadcastReceiver();

    }




    private void setupGuiHandler() {
        guiHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                try {
                    switch(msg.what) {
                        case HANDLER_UPDATE_TEXT:
                            SpannableStringBuilder text = (SpannableStringBuilder) msg.obj;
                            tvInfo.setText(text);
                            break;
                        default:
                            break;
                    }
                } catch(Exception e) {e.printStackTrace();}
            }
        };
    }

    private void setupBroadcastReceiver() {
        attackUpdateReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if(intent != null && intent.hasExtra("ATTACK_TYPE") && intent.hasExtra("ATTACK"))  {

                    String attackString = intent.getStringExtra("ATTACK");
                    String attackTypeString = intent.getStringExtra("ATTACK_TYPE");
                    Type attackType = Attack.ATTACK_TYPE.get(attackTypeString);
                    Attack attackNew = new Gson().fromJson(attackString, attackType);
                    if(attackNew.getGuid() == attack.getGuid()) {
                        Message msg = new Message();
                        msg.what = HANDLER_UPDATE_TEXT;
                        msg.obj = attackNew.getRunningInfoString();
                        guiHandler.sendMessage(msg);
                    }

                }
            }
        };
    }

    @Override
    public void onResume() {
        super.onResume();
        IntentFilter intentFilter = new IntentFilter("de.tu_darmstadt.seemoo.nexmon.ATTACK_GET");
        MyApplication.getAppContext().registerReceiver(attackUpdateReceiver, intentFilter);

        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_INFO_UPDATE");
        intent.putExtra(Attack.ATTACK_ID, attack.getGuid());
        MyApplication.getAppContext().sendBroadcast(intent);

    }

    @Override
    public void onPause() {
        super.onPause();
        MyApplication.getAppContext().unregisterReceiver(attackUpdateReceiver);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        if (container != null) {
            container.removeAllViews();
        }
        View view = inflater.inflate(R.layout.fragment_attack_info, container, false);

        tvInfoHead = (TextView) view.findViewById(R.id.tv_attack_info_header);
        tvInfo = (TextView) view.findViewById(R.id.tv_attack_info);
        tvInfoHead.setText(attack.getName() + ", ID: " + attack.getGuid());
        tvInfo.setText(attack.getRunningInfoString());
        return view;
    }



}
