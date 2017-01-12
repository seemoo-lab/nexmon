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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.sharky.SharkListElement;

public class SharkListArrayAdapter extends ArrayAdapter<SharkListElement> {

    ArrayList<SharkListElement> list;


    public SharkListArrayAdapter(Context context, ArrayList<SharkListElement> list) {
        super(context, 0, list);
        this.list = list;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {

        View rowView = convertView;
        if (rowView == null) {
            LayoutInflater inflater = MyApplication.getLayoutInflater();
            rowView = inflater.inflate(de.tu_darmstadt.seemoo.nexmon.R.layout.list_shark, null);
            ViewHolder viewHolder = new ViewHolder();

            viewHolder.tvTime = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tvTime);
            viewHolder.tvNumber = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tvNumber);
            viewHolder.tvSource = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tvSource);
            viewHolder.tvDestination = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tvDestination);
            viewHolder.tvProtocol = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tvProtocol);
            viewHolder.tvLength = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tvLength);


            rowView.setTag(viewHolder);
        }

        ViewHolder holder = (ViewHolder) rowView.getTag();
        try {
            SharkListElement sharkElement = list.get(position);

            holder.tvTime.setText(sharkElement.getTime());
            holder.tvNumber.setText(sharkElement.getNumber() + "");
            holder.tvSource.setText("src: " + sharkElement.getSource().toUpperCase());
            holder.tvDestination.setText("dst: " + sharkElement.getDest().toUpperCase());
            holder.tvProtocol.setText(sharkElement.getProtocol().toUpperCase());
            holder.tvLength.setText("length: " + sharkElement.getLength());
        } catch (Exception e) {
            e.printStackTrace();
        }


        return rowView;
    }

    static class ViewHolder {
        public TextView tvTime;
        public TextView tvNumber;
        public TextView tvSource;
        public TextView tvDestination;
        public TextView tvProtocol;
        public TextView tvLength;

    }

}
