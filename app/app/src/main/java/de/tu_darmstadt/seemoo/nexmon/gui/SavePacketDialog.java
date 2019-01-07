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


import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;
import de.tu_darmstadt.seemoo.nexmon.sharky.PcapFileWriter;

/**
 * Created by fabian on 11/15/16.
 */

public class SavePacketDialog extends DialogFragment {

    protected final static String HS_PACKETS = "HS_PACKETS";
    protected final static String BEACON_PACKETS = "BEACON_PACKETS";

    ArrayList<Packet> beacons;

    ArrayList<Packet> hsPackets;
    EditText etFilename;

    public SavePacketDialog() {

    }

    public static SavePacketDialog newInstance(ArrayList<Packet> hsPackets, ArrayList<Packet> beacons) {
        SavePacketDialog fragment = new SavePacketDialog();
        Bundle args = new Bundle();
        args.putString(HS_PACKETS, new Gson().toJson(hsPackets));
        args.putString(BEACON_PACKETS, new Gson().toJson(beacons));

        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (getArguments() != null) {
            hsPackets = new Gson().fromJson(getArguments().getString(HS_PACKETS), new TypeToken<ArrayList<Packet>>(){}.getType());
            beacons = new Gson().fromJson(getArguments().getString(BEACON_PACKETS), new TypeToken<ArrayList<Packet>>(){}.getType());
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder b = new AlertDialog.Builder(getActivity())
                .setTitle("Save to:")
                .setPositiveButton("OK",
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                try {
                                    String fullPath = etFilename.getText().toString();
                                    int index = fullPath.lastIndexOf("/");
                                    String filePath = fullPath.substring(0, index);
                                    String fileName = fullPath.substring(index);
                                    PcapFileWriter writer = new PcapFileWriter(filePath, fileName);
                                    writer.writePackets(beacons);
                                    writer.writePackets(hsPackets);
                                } catch(Exception e) {e.printStackTrace();}

                            }
                        }
                )
                .setNegativeButton("Cancel",
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                dialog.dismiss();
                            }
                        }
                );

        LayoutInflater i = getActivity().getLayoutInflater();

        View v = i.inflate(R.layout.filename_dialog, null);
        etFilename = (EditText) v.findViewById(R.id.et_filename);

        b.setView(v);

        return b.create();
    }
}
