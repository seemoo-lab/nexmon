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
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.stations.Station;

public class APListArrayAdapter extends ArrayAdapter<APlistElement> {

    ArrayList<APlistElement> list;

    public APListArrayAdapter(Context context, ArrayList<APlistElement> list) {
        super(context, 0, list);
        this.list = list;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {

        View rowView = convertView;
        if (rowView == null) {
            LayoutInflater inflater = MyApplication.getLayoutInflater();
            rowView = inflater.inflate(de.tu_darmstadt.seemoo.nexmon.R.layout.list_ap, null);
            ViewHolder viewHolder = new ViewHolder();

            viewHolder.tvSsid = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tv_ssid);
            viewHolder.tvBssid = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tv_bssid);
            viewHolder.tvChannel = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tv_channel);
            viewHolder.tvBeacons = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tv_beacons);
            viewHolder.tvLastSeen = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tv_last_seen);
            viewHolder.tvStations = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tv_stations);
            viewHolder.tvEnc = (TextView) rowView.findViewById(de.tu_darmstadt.seemoo.nexmon.R.id.tv_enc);
            viewHolder.tvSignalStrength = (TextView) rowView.findViewById(R.id.tv_signal_strength);

            rowView.setTag(viewHolder);
        }

        ViewHolder holder = (ViewHolder) rowView.getTag();
        try {
            APlistElement ap = list.get(position);

            holder.tvSsid.setText(ap.ssid);
            holder.tvBssid.setText(ap.bssid);
            holder.tvChannel.setText("Ch.: " + ap.channel);
            holder.tvBeacons.setText("Beacons: " + ap.beacons);
            holder.tvLastSeen.setText("Last seen: " + (System.currentTimeMillis() - ap.lastSeen) / 1000 + "s");
            holder.tvEnc.setText(ap.encryption);
            holder.tvSignalStrength.setText(ap.signalStrength + " dBm");
            String stationString = "";
            for (Station station : ap.stations) {
                stationString += station.macAddress + "\u0009 " + station.dataFrames + " frames,\u0009 Last seen: " + (System.currentTimeMillis() - station.lastSeen) / 1000 + "s\n";
            }

            holder.tvStations.setText(stationString);

            if(ap.hasHandshake) {
                rowView.setBackgroundColor(Color.argb(128,60,179,113));
            } else
                rowView.setBackgroundColor(Color.TRANSPARENT);

        } catch (Exception e) {
            e.printStackTrace();
        }


        return rowView;
    }

    static class ViewHolder {
        public TextView tvSsid;
        public TextView tvBssid;
        public TextView tvChannel;
        public TextView tvBeacons;
        public TextView tvLastSeen;
        public TextView tvStations;
        public TextView tvEnc;
        public TextView tvSignalStrength;
    }


}
