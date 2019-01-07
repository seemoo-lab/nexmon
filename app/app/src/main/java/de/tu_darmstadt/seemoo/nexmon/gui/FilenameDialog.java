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
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

import java.io.IOException;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.sharky.PcapFileWriter;

public class FilenameDialog extends DialogFragment {

    String tmpFile;
    EditText etFilename;

    public FilenameDialog() {
    }

    public FilenameDialog(String tmpFile) {
        this.tmpFile = tmpFile;
    }


    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder b = new AlertDialog.Builder(getActivity())
                .setTitle("Save to:")
                .setPositiveButton("OK",
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                saveFile(etFilename.getText().toString());
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

    private void saveFile(final String fileName) {
        new Thread(new Runnable() {

            @Override
            public void run() {
                try {
                    PcapFileWriter.copy(tmpFile, fileName);
                    MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).edit().putString("reader", fileName).commit();


                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }).start();
    }

}
