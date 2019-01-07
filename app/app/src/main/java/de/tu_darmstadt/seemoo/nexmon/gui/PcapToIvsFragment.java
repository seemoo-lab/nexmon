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
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;

import com.nononsenseapps.filepicker.FilePickerActivity;
import com.roger.catloadinglibrary.CatLoadingView;

import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.sharky.IvsTools;

/**
 * Created by fabian on 9/26/16.
 */
public class PcapToIvsFragment extends TrackingFragment {

    private static final int HANDLER_SHOW_LOADING = 1;
    private static final int HANDLER_DISMISS_LOADING = 2;

    private static final int REQ_CODE_SRC1 = 10;
    private static final int REQ_CODE_DST_PATH = 51;


    private static EditText etSrc1;
    private static EditText etDst;

    private static Button btnSelect1;
    private static Button btnSelectDst;
    private static Button btnConcat;

    private CatLoadingView loadingView;

    private String src1;
    private String dst;



    private Handler guiHandler;

    public PcapToIvsFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @return A new instance of fragment AttackInfoFragment.
     */
    // TODO: Rename and change types and number of parameters
    public static PcapToIvsFragment newInstance() {
        PcapToIvsFragment fragment = new PcapToIvsFragment();
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setupGuiHandler();



    }

    private void setupGuiHandler() {
        guiHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                try {
                    switch(msg.what) {
                        case HANDLER_SHOW_LOADING:
                            loadingView = new CatLoadingView();
                            loadingView.setCancelable(false);
                            loadingView.show(getFragmentManager(), "");
                            break;
                        case HANDLER_DISMISS_LOADING:
                            loadingView.dismiss();
                            break;
                        default:
                            break;
                    }
                } catch(Exception e) {e.printStackTrace();}
            }
        };
    }



    @Override
    public void onResume() {
        super.onResume();

    }

    @Override
    public String getTrackingName() {
        return "Screen: PCAP to IVS";
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        if (container != null) {
            container.removeAllViews();
        }
        View view = inflater.inflate(R.layout.fragment_pcap_to_ivs, container, false);

        etSrc1 = (EditText) view.findViewById(R.id.et_pcap_ivs_src);
        etDst = (EditText) view.findViewById(R.id.et_pcap_ivs_dst);
        btnSelect1 = (Button) view.findViewById(R.id.btn_pcap_ivs_src);
        btnSelectDst = (Button) view.findViewById(R.id.btn_pcap_ivs_dst);
        btnConcat = (Button) view.findViewById(R.id.btn_pcap_ivs_convert);

        btnSelect1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                selectFile(REQ_CODE_SRC1);
            }
        });

        btnSelectDst.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                selectNewFile(REQ_CODE_DST_PATH);
            }
        });


        btnConcat.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dst = etDst.getText().toString();
                if(src1 != null && !src1.equals("") && dst != null && !dst.equals("")) {
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            guiHandler.sendEmptyMessage(HANDLER_SHOW_LOADING);
                            try {
                                IvsTools ivsTools = new IvsTools();
                                ivsTools.convert(src1, dst);
                            } catch(Exception e) {e.printStackTrace();}
                            guiHandler.sendEmptyMessage(HANDLER_DISMISS_LOADING);
                        }
                    }).start();

                }
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

    private void selectNewFile(int requestCode) {

        Intent i = new Intent(Intent.ACTION_GET_CONTENT);

        i.putExtra(FilePickerActivity.EXTRA_ALLOW_MULTIPLE, false);
        i.putExtra(FilePickerActivity.EXTRA_ALLOW_CREATE_DIR, true);
        i.putExtra(FilePickerActivity.EXTRA_MODE, FilePickerActivity.MODE_NEW_FILE);
        i.putExtra(FilePickerActivity.EXTRA_START_PATH, Environment.getExternalStorageDirectory().getPath());

        startActivityForResult(i, requestCode);

    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, final Intent data) {
        if (requestCode == REQ_CODE_SRC1 && data != null && data.getData() != null) {
            src1 = data.getData().getPath();
            etSrc1.setText(src1);
        } else if(requestCode == REQ_CODE_DST_PATH && data != null) {
            String result = data.getData().getPath();
            etDst.setText(result);
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        try {
            getActivity().setTitle("Nexmon: PCAP to IV");
        } catch(Exception e) {e.printStackTrace();}
    }
}
