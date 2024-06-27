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
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.roger.catloadinglibrary.CatLoadingView;
import com.stericson.RootTools.RootTools;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.utils.Assets;
import de.tu_darmstadt.seemoo.nexmon.utils.Dhdutil;
import de.tu_darmstadt.seemoo.nexmon.utils.FirmwareUtil;
import de.tu_darmstadt.seemoo.nexmon.utils.Nexutil;
import eu.chainfire.libsuperuser.Shell;

/**
 * A simple {@link Fragment} subclass.
 * Use the {@link StartFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class StartFragment extends Fragment {

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
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onStop() {
        super.onStop();
        ((MyActivity) getActivity()).removePermissionListener();
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
                  String x = "isbroadcomchip: " + FirmwareUtil.getInstance().isBroadcomChip() + "\n";
                //x += " capabilities: " + FirmwareUtil.getInstance().getCapabilities();
                //x += " magic: " + String.format("%08x", Nexutil.getInstance().getIntIoctl(0)) + "\n";
                //x += " magic: " + String.format("%08x", 0x11223344) + "\n";
                //x += " test: " + String.format("%08x", (new Nexutil()).get(0).executeInt());
                  x += " ver:" + (new Nexutil()).getIovar("ver", 256);
                  Toast.makeText(getContext(), x, Toast.LENGTH_SHORT).show();
                //Toast.makeText(getContext(), Dhdutil.getInstance().dumpConsole(), Toast.LENGTH_LONG).show();
                //Log.d("DHDUTIL", Dhdutil.getInstance().dumpConsole());
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

        return view;
    }

    public void onClickNexmon() {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        intent.setData(Uri.parse("https://nexmon.org"));
        startActivity(intent);
    }
}
