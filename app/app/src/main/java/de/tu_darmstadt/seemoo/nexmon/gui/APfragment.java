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
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.commands.CurrentChannelCommand;
import de.tu_darmstadt.seemoo.nexmon.commands.CurrentChannelListener;
import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;
import de.tu_darmstadt.seemoo.nexmon.stations.APfinderService;
import de.tu_darmstadt.seemoo.nexmon.stations.AccessPoint;
import de.tu_darmstadt.seemoo.nexmon.stations.Handshake;
import de.tu_darmstadt.seemoo.nexmon.stations.Station;


public class APfragment extends Fragment implements CurrentChannelListener {

    public static final int UPDATE_LIST = 30;
    public static final int UPDATE_LIST_COMPLETE = 31;
    public static final int CLEAR_LIST = 32;

    private static final int MAX_AP_SHOWN = 100;

    ArrayAdapter<APlistElement> adapter;
    ArrayList<APlistElement> apList = new ArrayList<APlistElement>();
    Handler guiHandler;
    ListView lvAp;
    TextView tvAirodumpInfo;
    APfinderService aPfinderService;
    boolean running = true;
    AttackSelectionDialog attackSelectionDialog;
    CurrentChannelCommand currentChannelCommand;

    ChannelSelectDialog channelSelectDialog;

    SavePacketDialog savePacketDialog;

    private BroadcastReceiver APfinderServiceReceiver;

    private ServiceConnection serviceConnection = new ServiceConnection() {

        @Override
        public void onServiceDisconnected(ComponentName name) {
            aPfinderService = null;

        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            APfinderService.MyBinder binder = (APfinderService.MyBinder) service;
            aPfinderService = binder.getService();

            HashMap<String, AccessPoint> map = aPfinderService.getAccessPoints();
            for(AccessPoint ap : map.values()) {
                Message msg = new Message();
                msg.what = UPDATE_LIST;
                msg.obj = ap;
                guiHandler.sendMessage(msg);
            }
        }
    };


    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_toggle_finder:
                toggleApFinder();
                break;
            case R.id.action_save_handshakes:
                saveHandshakes(aPfinderService.getAllHandshakePackets(), aPfinderService.getAllBeacons());
                break;
            case R.id.action_change_channel:
                channelSelectDialog = ChannelSelectDialog.newInstance();
                channelSelectDialog.show(getFragmentManager(), "");
                break;
            default:
                break;
        }
        return true;
    }

    public APfragment() {

      //  bssidToAccessPoint = new HashMap<String, AccessPoint>();
        adapter = new APListArrayAdapter(MyApplication.getAppContext(), apList);

        guiHandler = new Handler() {

            @SuppressWarnings("unchecked")
            public void handleMessage(Message msg) {
                super.handleMessage(msg);

                switch (msg.what) {
                    case UPDATE_LIST:
                        AccessPoint element = (AccessPoint) msg.obj;
                        boolean contains = false;
                        for (APlistElement el : apList) {
                            try {
                                if (element.getBssid().equals(el.bssid)) {
                                    contains = true;
                                    el.ssid = element.getSsid();
                                    el.beacons = element.getBeacons();
                                    el.channel = element.getChannel();
                                    el.cipher = element.getCipher() + "";
                                    el.lastSeen = element.getLastSeen();
                                    el.stations = new ArrayList<Station>(element.getStations().values());
                                    el.encryption = element.getEncryption();
                                    el.signalStrength = element.getSignalStrength();

                                    Iterator<Handshake> it = element.getHandshakes().values().iterator();
                                    while(it.hasNext()) {
                                        if(it.next().isComplete) {
                                            el.hasHandshake = true;
                                            break;
                                        }
                                    }
                                    break;
                                }
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                        }

                        if (!contains) {
                            APlistElement apListElement = new APlistElement();
                            apListElement.bssid = element.getBssid();
                            apListElement.ssid = element.getSsid();
                            apListElement.beacons = element.getBeacons();
                            apListElement.channel = element.getChannel();
                            apListElement.cipher = element.getCipher() + "";
                            apListElement.lastSeen = element.getLastSeen();
                            apListElement.stations = new ArrayList<Station>(element.getStations().values());
                            apListElement.encryption = element.getEncryption();
                            apListElement.signalStrength = element.getSignalStrength();
                            apList.add(apListElement);
                        }

                        adapter.notifyDataSetChanged();

                        if(adapter.getCount() > MAX_AP_SHOWN)
                            guiHandler.sendEmptyMessage(CLEAR_LIST);

                        break;
                    case UPDATE_LIST_COMPLETE:
                        adapter.notifyDataSetChanged();
                        if(!apList.isEmpty())
                            tvAirodumpInfo.setVisibility(View.GONE);
                        break;
                    case CLEAR_LIST:
                        apList.clear();
                        adapter.notifyDataSetChanged();
                        break;

                }
            }
        };


    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);


    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        inflater.inflate(R.menu.ap_finder_menu, menu);
        super.onCreateOptionsMenu(menu, inflater);
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        try {
            setupMenu(menu);
        } catch (Exception e) {
            // TODO: handle exception
        }
    }



    private void setupMenu(Menu menu) {
        if (aPfinderService.isRunning()) {
            menu.findItem(R.id.action_toggle_finder).setTitle("Stop");
        } else {
            menu.findItem(R.id.action_toggle_finder).setTitle("Start");
        }

        MenuItem channelItem = menu.findItem(R.id.action_change_channel);
    }

    @Override
    public void onPause() {
        super.onPause();
        running = false;
        getActivity().unregisterReceiver(APfinderServiceReceiver);
        if(currentChannelCommand != null) {
            currentChannelCommand.removeListener();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        running = true;
        Intent intent = new Intent(getActivity().getApplicationContext(), APfinderService.class);

        APfinderServiceReceiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {
                String bssid = intent.getStringExtra("bssid");
                AccessPoint ap = aPfinderService.getAccessPoint(bssid);
                Message msg = new Message();
                msg.obj = ap;
                msg.what = UPDATE_LIST;
                guiHandler.sendMessage(msg);


            }
        };
        MyApplication.getAppContext().bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
        IntentFilter intentFilter = new IntentFilter("de.tu_darmstadt.seemoo.nexmon.NEW_AP");
        getActivity().registerReceiver(APfinderServiceReceiver, intentFilter);
        contUpdate();
    }

    private void toggleApFinder() {
        if (!aPfinderService.isRunning()) {
            guiHandler.sendEmptyMessage(CLEAR_LIST);
            aPfinderService.start();
            tvAirodumpInfo.setVisibility(View.GONE);
        } else
            aPfinderService.stop();
    }

    private void contUpdate() {
        new Thread(new Runnable() {

            @Override
            public void run() {
                while (running) {
                    guiHandler.sendEmptyMessage(UPDATE_LIST_COMPLETE);
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }).start();


    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        //running = false;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        if (container != null) {
            container.removeAllViews();
        }
        View view = inflater.inflate(R.layout.fragment_ap, container, false);
        lvAp = (ListView) view.findViewById(R.id.lv_ap);
        tvAirodumpInfo = (TextView) view.findViewById(R.id.tv_airodump_info);



        lvAp.setAdapter(adapter);

        lvAp.setOnItemClickListener(new OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> parent, View view,
                                    int position, long id) {
                APlistElement element = (APlistElement) parent.getAdapter().getItem(position);
                AccessPoint ap = aPfinderService.getAccessPoint(element.bssid);

                attackSelectionDialog = AttackSelectionDialog.newInstance(ap);
                attackSelectionDialog.show(getFragmentManager(), "");
            }
        });

        if(!apList.isEmpty())
            tvAirodumpInfo.setVisibility(View.GONE);



        return view;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        try {
            getActivity().setTitle("Nexmon: Airodump");
        } catch(Exception e) {e.printStackTrace();}

    }


    private void saveHandshakes(ArrayList<Packet> hsPackets, ArrayList<Packet> beacons) {
        savePacketDialog = SavePacketDialog.newInstance(hsPackets, beacons);
        savePacketDialog.show(getFragmentManager(), "");
    }


    @Override
    public void onCurrentChannelInfo(int channel) {

    }

    @Override
    public void onCurrentChannelError(String error) {

    }
}
