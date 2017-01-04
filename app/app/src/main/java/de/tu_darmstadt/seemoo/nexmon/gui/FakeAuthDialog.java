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

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import com.google.gson.Gson;

import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.Attack;
import de.tu_darmstadt.seemoo.nexmon.stations.FakeAuthAttack;

/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link FakeAuthDialog.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link FakeAuthDialog#newInstance} factory method to
 * create an instance of this fragment.
 */
public class FakeAuthDialog extends AttackDialog {

    private TextView tvBssid;
    private TextView tvSsid;
    private Spinner spinnerAp;
    private EditText etKeepaliveTiming;
    private EditText etReassocTiming;
    private EditText etPacketTiming;
    private Button btnStartFakeAuth;
    private Attack attack;

    ArrayAdapter<String> stationsAdapter;


    public FakeAuthDialog() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @param ap AccessPoint to attack.
     * @return A new instance of fragment FakeAuthDialog.
     */
    // TODO: Rename and change types and number of parameters
    public static FakeAuthDialog newInstance(AccessPoint ap) {
        FakeAuthDialog fragment = new FakeAuthDialog();
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
        View view = inflater.inflate(R.layout.fragment_fake_auth_dialog, container, false);

        tvBssid = (TextView) view.findViewById(R.id.tv_bssid_ap);
        tvSsid = (TextView) view.findViewById(R.id.tv_ssid_ap);
        spinnerAp = (Spinner) view.findViewById(R.id.spinner_station_mac);
        etKeepaliveTiming = (EditText) view.findViewById(R.id.et_keepalive_timing);
        etReassocTiming = (EditText) view.findViewById(R.id.et_reassoc_timing);
        etPacketTiming = (EditText) view.findViewById(R.id.et_packet_timing);
        btnStartFakeAuth = (Button) view.findViewById(R.id.btn_start_fakeauth);

        tvBssid.setText("BSSID: " + ap.getBssid());
        tvSsid.setText("SSID: " + ap.getSsid());

        ArrayList<String> stations = new ArrayList<String>();
        stations.add("own");
        stations.addAll(ap.getStations().keySet());

        stationsAdapter = new ArrayAdapter<String>(getActivity(), android.R.layout.simple_spinner_item, stations);
        stationsAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerAp.setAdapter(stationsAdapter);

        btnStartFakeAuth.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String station = ((String) spinnerAp.getSelectedItem()).trim();

                boolean useCustomStation = !station.equals("own");
                int keepaliveTiming = Integer.valueOf(etKeepaliveTiming.getText().toString());
                int reassocTiming = Integer.valueOf(etReassocTiming.getText().toString());
                int packetTiming = Integer.valueOf(etPacketTiming.getText().toString());


                attack = new FakeAuthAttack(ap, reassocTiming, keepaliveTiming, packetTiming, useCustomStation, station);
                String ob = new Gson().toJson(attack);
                Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
                intent.putExtra("ATTACK", ob);
                intent.putExtra("ATTACK_TYPE", Attack.ATTACK_FAKE_AUTH);
                MyApplication.getAppContext().sendBroadcast(intent);
                dismiss();
                //new Thread(attack).start();
            }
        });
        return view;
    }



    @Override
    public void onAttach(Context context) {
        super.onAttach(context);

    }

    @Override
    public void onDetach() {
        super.onDetach();

    }


}
