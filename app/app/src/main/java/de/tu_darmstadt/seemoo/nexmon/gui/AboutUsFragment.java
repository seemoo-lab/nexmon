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
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;

import de.tu_darmstadt.seemoo.nexmon.R;

/**
 * A simple {@link Fragment} subclass.
 */
public class AboutUsFragment extends Fragment {

    private ImageView imgTudLogo;
    private ImageView imgSeemooLogo;
    private ImageView imgNexmonLogo;
    private Button btnLicenses;

    private LicenseDialog licenseDialog;


    public AboutUsFragment() {
        // Required empty public constructor
    }

    /**
     * Use this factory method to create a new instance of
     * this fragment using the provided parameters.
     *
     * @return A new instance of fragment.
     */
    public static AboutUsFragment newInstance() {
        AboutUsFragment fragment = new AboutUsFragment();
        return fragment;
    }



    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_about_us, container, false);

        imgNexmonLogo = (ImageView) view.findViewById(R.id.imgNexmonLogo);
        imgSeemooLogo = (ImageView) view.findViewById(R.id.imgSeemooLogo);
        imgTudLogo = (ImageView) view.findViewById(R.id.imgTudLogo);
        btnLicenses = (Button) view.findViewById(R.id.btnLicenses);

        imgSeemooLogo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onClickSeemoo();
            }
        });

        imgNexmonLogo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onClickNexmon();
            }
        });

        imgTudLogo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onClickTuDarmstadt();
            }
        });

        btnLicenses.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onClickLicenses();
            }
        });

        return view;
    }

    @Override
    public void onPause() {
        super.onPause();

    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        try {
            getActivity().setTitle("Nexmon: About us");
        } catch(Exception e) {e.printStackTrace();}
    }

    public void onClickNexmon() {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        intent.setData(Uri.parse("https://nexmon.org"));
        startActivity(intent);
    }

    public void onClickSeemoo() {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        intent.setData(Uri.parse("https://seemoo.tu-darmstadt.de"));
        startActivity(intent);
    }

    public void onClickTuDarmstadt() {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        intent.setData(Uri.parse("https://www.tu-darmstadt.de"));
        startActivity(intent);
    }

    public void onClickLicenses() {
        licenseDialog = LicenseDialog.newInstance();
        licenseDialog.show(getFragmentManager(), "");
    }




}
