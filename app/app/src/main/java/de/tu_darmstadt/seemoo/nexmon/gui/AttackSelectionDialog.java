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
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.google.gson.Gson;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;
import de.tu_darmstadt.seemoo.nexmon.stations.UpcAttack;

/**
 * A simple {@link Fragment} subclass.
 * Use the {@link AttackSelectionDialog#newInstance} factory method to
 * create an instance of this fragment.
 */
public class AttackSelectionDialog extends DialogFragment implements Attack.IAttackInstanceUpdate, View.OnClickListener{

    private static final String ARG_AP = "AccessPoint";

    private static final int GUI_DISABLE_BUTTON = 40;
    private static final int GUI_ENABLE_BUTTON = 41;
    private AccessPoint accessPoint;

    private FakeAuthDialog fakeAuthDialog;
    private DeauthDialog deauthDialog;
    private ArpReplayDialog arpReplayDialog;
    private BeaconFloodDialog beaconFloodDialog;
    private AuthFloodDialog authFloodDialog;
    private CountermeasuresDialog countermeasuresDialog;
    private WidsDialog widsDialog;
    private WpsDialog wpsDialog;


    private Handler guiHandler;

    private static Button btnFakeAuth;
    private static Button btnDeAuth;
    private static Button btnArpReplay;
    private static Button btnBeaconFlood;
    private static Button btnAuthFlood;
    private static Button btnCountermeasures;
    private static Button btnWids;
    private static Button btnUpc;
    private static Button btnReaver;


    private static final HashMap<String, Button> attackButtons = new HashMap<String, Button>();
    private HashMap<String, Integer> remainingInstances = new HashMap<>(Attack.ATTACK_MAX_INSTANCES);


    public AttackSelectionDialog() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @param accessPoint Parameter 1.
     * @return A new instance of fragment AttackSelectionDialog.
     */
    public static AttackSelectionDialog newInstance(AccessPoint accessPoint) {
        AttackSelectionDialog fragment = new AttackSelectionDialog();
        String accessPointString = new Gson().toJson(accessPoint);
        Bundle args = new Bundle();
        args.putString(ARG_AP, accessPointString);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            String apString = getArguments().getString(ARG_AP);
            accessPoint = new Gson().fromJson(apString, AccessPoint.class);
        }

        guiHandler = new Handler() {

            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                Button button;
                try {
                    switch(msg.what) {
                        case GUI_DISABLE_BUTTON:
                            button = (Button) msg.obj;
                            button.setEnabled(false);
                            break;
                        case GUI_ENABLE_BUTTON:
                            button = (Button) msg.obj;
                            button.setEnabled(true);
                            break;
                        default:
                            break;
                    }
                } catch(Exception e) {e.printStackTrace();}
            }


        };
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        View view = inflater.inflate(R.layout.attack_selection_dialog, container, false);

        getDialog().setTitle("Attack Selection");

        btnFakeAuth = (Button) view.findViewById(R.id.btn_attack_fakeauth);
        btnDeAuth = (Button) view.findViewById(R.id.btn_attack_deauth);
        btnArpReplay = (Button) view.findViewById(R.id.btn_arp_replay_attack);
        btnBeaconFlood = (Button) view.findViewById(R.id.btn_beacon_flood_attack);
        btnAuthFlood = (Button) view.findViewById(R.id.btn_auth_flood_attack);
        btnCountermeasures = (Button) view.findViewById(R.id.btn_countermeasures_attack);
        btnWids = (Button) view.findViewById(R.id.btn_wids_attack);
        btnUpc = (Button) view.findViewById(R.id.btn_upc_attack);
        btnReaver = (Button) view.findViewById(R.id.btn_reaver_attack);


        btnFakeAuth.setTag(Attack.ATTACK_FAKE_AUTH);
        btnDeAuth.setTag(Attack.ATTACK_DE_AUTH);
        btnArpReplay.setTag(Attack.ATTACK_ARP_REPLAY);
        btnBeaconFlood.setTag(Attack.ATTACK_BEACON_FLOOD);
        btnAuthFlood.setTag(Attack.ATTACK_AUTH_FLOOD);
        btnCountermeasures.setTag(Attack.ATTACK_COUNTERMEASURES);
        btnWids.setTag(Attack.ATTACK_WIDS);
        btnUpc.setTag(Attack.ATTACK_UPC);
        btnReaver.setTag(Attack.ATTACK_REAVER);

        attackButtons.put(Attack.ATTACK_FAKE_AUTH, btnFakeAuth);
        attackButtons.put(Attack.ATTACK_DE_AUTH, btnDeAuth);
        attackButtons.put(Attack.ATTACK_ARP_REPLAY, btnArpReplay);
        attackButtons.put(Attack.ATTACK_BEACON_FLOOD, btnBeaconFlood);
        attackButtons.put(Attack.ATTACK_AUTH_FLOOD, btnAuthFlood);
        attackButtons.put(Attack.ATTACK_COUNTERMEASURES, btnCountermeasures);
        attackButtons.put(Attack.ATTACK_WIDS, btnWids);
        attackButtons.put(Attack.ATTACK_UPC, btnUpc);
        attackButtons.put(Attack.ATTACK_REAVER, btnReaver);

        for(Button attackBtn : attackButtons.values()) {
            attackBtn.setOnClickListener(this);
        }

        return view;
    }

    @Override
    public void onResume() {
        super.onResume();
        evaluateButtons();
        Attack.setObserver(this);
        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_INSTANCE_REQUEST");
        MyApplication.getAppContext().sendBroadcast(intent);
    }

    @Override
    public void onPause() {
        super.onPause();
        Attack.deleteObserver();
    }

    @Override
    public void onAttackInstanceUpdate(HashMap<String, Integer> remainingInstances) {
        this.remainingInstances = (HashMap) remainingInstances.clone();
        evaluateButtons();

    }

    private void evaluateButtons() {

        Iterator it = remainingInstances.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry pair = (Map.Entry)it.next();
            Message message = new Message();
            message.obj = attackButtons.get(pair.getKey());
            if((int) pair.getValue() <= 0)
                message.what = GUI_DISABLE_BUTTON;
            else
                message.what = GUI_ENABLE_BUTTON;
            guiHandler.sendMessage(message);
            //it.remove();
        }
    }

    private void showDeauthDialog(AccessPoint ap) {
        deauthDialog = DeauthDialog.newInstance(ap);
        deauthDialog.show(getFragmentManager(), "");
    }

    private void showFakeAuthDialog(AccessPoint ap) {
        fakeAuthDialog = FakeAuthDialog.newInstance(ap);
        fakeAuthDialog.show(getFragmentManager(), "");
    }

    private void showArpReplayDialog(AccessPoint ap) {
        arpReplayDialog = ArpReplayDialog.newInstance(ap);
        arpReplayDialog.show(getFragmentManager(), "");
    }

    private void showBeaconFloodDialog() {
        beaconFloodDialog = BeaconFloodDialog.newInstance();
        beaconFloodDialog.show(getFragmentManager(), "");
    }

    private void showAuthFloodDialog(AccessPoint ap) {
        authFloodDialog = AuthFloodDialog.newInstance(ap);
        authFloodDialog.show(getFragmentManager(), "");
    }

    private void showCountermeasuresDialog(AccessPoint ap) {
        countermeasuresDialog = CountermeasuresDialog.newInstance(ap);
        countermeasuresDialog.show(getFragmentManager(), "");
    }

    private void showWidsDialog(AccessPoint ap) {
        widsDialog = WidsDialog.newInstance(ap);
        widsDialog.show(getFragmentManager(), "");
    }
    private void showWpsDialog(AccessPoint ap) {
        wpsDialog = WpsDialog.newInstance(ap);
        wpsDialog.show(getFragmentManager(), "");
    }

    private void showUpcAttack(AccessPoint ap) {
        if(ap.getSsid() == null || !ap.getSsid().startsWith("UPC")) {
            MyApplication.toast("Only SSID starting with \"UPC\" are valid for this Attack.");
            return;
        }

        Attack attack = new UpcAttack(ap);
        String ob = new Gson().toJson(attack);
        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
        intent.putExtra("ATTACK", ob);
        intent.putExtra("ATTACK_TYPE", Attack.ATTACK_UPC);
        MyApplication.getAppContext().sendBroadcast(intent);
        dismiss();
    }

    @Override
    public void onClick(View v) {
        String attack = (String) v.getTag();
        if(!MyApplication.isInjectionAvailable() || !MyApplication.isRawproxyreverseAvailable()) {
            switch (attack) {
                case Attack.ATTACK_FAKE_AUTH:
                case Attack.ATTACK_DE_AUTH:
                case Attack.ATTACK_ARP_REPLAY:
                case Attack.ATTACK_BEACON_FLOOD:
                case Attack.ATTACK_AUTH_FLOOD:
                case Attack.ATTACK_COUNTERMEASURES:
                case Attack.ATTACK_WIDS:
                case Attack.ATTACK_REAVER:
                    if (!MyApplication.isInjectionAvailable())
                        MyApplication.toast("No injection support available, sorry.");
                    else if (!MyApplication.isRawproxyreverseAvailable())
                        MyApplication.toast("Please install \"Tools\" -> rawproxyreverse first.");
                    else
                        MyApplication.toast("Unknown error!");

                    break;
                case Attack.ATTACK_UPC:
                    showUpcAttack(accessPoint);
                    break;
                default:
                    break;
            }
        } else {
            switch (attack) {
                case Attack.ATTACK_FAKE_AUTH:
                    showFakeAuthDialog(accessPoint);
                    break;
                case Attack.ATTACK_DE_AUTH:
                    if(accessPoint.getStations().isEmpty()) {
                        MyApplication.toast("No clients available!");
                    } else {
                        showDeauthDialog(accessPoint);
                    }
                    break;
                case Attack.ATTACK_ARP_REPLAY:
                    if(accessPoint.getStations().isEmpty()) {
                        MyApplication.toast("No clients available!");
                    } else {
                        showArpReplayDialog(accessPoint);
                    }
                    break;
                case Attack.ATTACK_BEACON_FLOOD:
                    showBeaconFloodDialog();
                    break;
                case Attack.ATTACK_AUTH_FLOOD:
                    showAuthFloodDialog(accessPoint);
                    break;
                case Attack.ATTACK_COUNTERMEASURES:
                    showCountermeasuresDialog(accessPoint);
                    break;
                case Attack.ATTACK_WIDS:
                    showWidsDialog(accessPoint);
                    break;
                case Attack.ATTACK_REAVER:
                    showWpsDialog(accessPoint);
                    break;
                case Attack.ATTACK_UPC:
                    showUpcAttack(accessPoint);
                    break;
                default:
                    MyApplication.toast("Unknown error!");
                    break;
            }
        }
    }
}
