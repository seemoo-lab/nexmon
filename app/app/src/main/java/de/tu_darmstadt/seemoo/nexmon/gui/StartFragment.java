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
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.SpannableStringBuilder;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.roger.catloadinglibrary.CatLoadingView;
import com.stericson.RootTools.RootTools;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;

/**
 * A simple {@link Fragment} subclass.
 * Use the {@link StartFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class StartFragment extends TrackingFragment {

    TextView tvNexmonInfo;
    ImageView ivNexmon;
    Button btnRoot;

    Handler guiHandler;

    CatLoadingView loadingView;

    private static final int GUI_UPDATE_TEXT = 11;
    private static final int GUI_SHOW_LOADING = 12;
    private static final int GUI_DISMISS_LOADING = 13;
    private static final int ROOT_DENIED = 21;
    private static final int ROOT_GRANTED = 22;

    public StartFragment() {
        // Required empty public constructor
    }


    public static StartFragment newInstance() {
        StartFragment fragment = new StartFragment();
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setupHandler();
    }

    @Override
    public void onStart() {
        super.onStart();


    }

    @Override
    public String getTrackingName() {
        return "Screen: Start";
    }

    private void setupHandler() {
        guiHandler = new Handler() {

            @SuppressWarnings("unchecked")
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                try {
                    switch (msg.what) {
                        case GUI_SHOW_LOADING:
                            loadingView = new CatLoadingView();
                            loadingView.setCancelable(false);
                            loadingView.show(getFragmentManager(), "");
                            break;
                        case GUI_DISMISS_LOADING:
                            loadingView.dismiss();
                            break;
                        case GUI_UPDATE_TEXT:
                            tvNexmonInfo.setText((SpannableStringBuilder) msg.obj);
                            break;
                        case ROOT_DENIED:
                            tvNexmonInfo.setText(getRootDeniedText());
                            Intent intent = MyApplication.getPackManager().getLaunchIntentForPackage("eu.chainfire.supersu");
                            if (intent != null) {
                                // We found the activity now start the activity
                                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                startActivity(intent);
                            }
                            break;
                        case ROOT_GRANTED:
                            checkInstallation();
                            break;
                    }
                } catch(Exception e) {
                    e.printStackTrace();
                }
            }
        };
    }

    private SpannableStringBuilder getRootDeniedText() {
        SpannableStringBuilder infoString = new SpannableStringBuilder();
        infoString.append("\n\n\nSorry, we need root access to check the Nexmon installation status.", new ForegroundColorSpan(Color.RED), 0);
        return infoString;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        if (container != null) {
            container.removeAllViews();
        }

        View view = inflater.inflate(R.layout.fragment_blank, container, false);
        ivNexmon = (ImageView) view.findViewById(R.id.iv_nexmon);
        tvNexmonInfo = (TextView) view.findViewById(R.id.tv_nexmon_info);
        btnRoot = (Button) view.findViewById(R.id.btn_root);

        tvNexmonInfo.setText("");

        btnRoot.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        if(RootTools.isRootAvailable() && RootTools.isAccessGiven()) {
                            MyApplication.isRootGranted = true;
                            guiHandler.sendEmptyMessage(ROOT_GRANTED);
                            MyActivity.verifyStoragePermissions(getActivity());
                        } else {
                            guiHandler.sendEmptyMessage(ROOT_DENIED);
                            MyApplication.isRootGranted = false;
                        }
                    }
                }).start();
            }
        });

        ivNexmon.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onClickNexmon();
            }
        });

        try {
            getActivity().setTitle("Nexmon: Start");
        } catch(Exception e) {e.printStackTrace();}

        showInstallInfo();

        return view;
    }


    private void checkInstallation() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                guiHandler.sendEmptyMessage(GUI_SHOW_LOADING);
                MyApplication.evaluateAll();
                try {
                    Thread.sleep(3000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                showInstallInfo();
                guiHandler.sendEmptyMessage(GUI_DISMISS_LOADING);


            }
        }).start();
    }

    private void showInstallInfo() {
        Message msg = guiHandler.obtainMessage(GUI_UPDATE_TEXT, MyApplication.getInstallInfo());
        guiHandler.sendMessage(msg);
    }


    public void onClickNexmon() {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        intent.setData(Uri.parse("https://nexmon.org"));
        startActivity(intent);
    }
}
