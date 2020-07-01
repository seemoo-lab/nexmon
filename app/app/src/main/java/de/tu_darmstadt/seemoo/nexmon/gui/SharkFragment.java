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
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.nononsenseapps.filepicker.FilePickerActivity;
import com.roger.catloadinglibrary.CatLoadingView;

import java.io.File;
import java.util.ArrayList;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;
import de.tu_darmstadt.seemoo.nexmon.sharky.PcapFileReader;
import de.tu_darmstadt.seemoo.nexmon.sharky.SharkListElement;
import de.tu_darmstadt.seemoo.nexmon.sharky.WiresharkService;


public class SharkFragment extends Fragment {

    public static final int PACKET_AMOUNT_TO_SHOW = 500;
    private static final int UPDATE_LISTVIEW_STATIC = 14;
    private static final int UPDATE_LISTVIEW_STATIC_TEST = 15;

    private static final int UPDATE_LISTVIEW = 1;
    private static final int LOADED_FROM_FILE = 2;
    private static final int SHOW_LOADING = 3;
    private static final int CLOSE_CONTROL_VIEW = 4;
    private static final int SHOW_CONTROL_VIEW = 5;
    private static final int LAST_PAGE_TO_SHOW = 6;
    private static final int NOT_LAST_PAGE_TO_SHOW = 7;
    private static final int FIRST_PAGE_TO_SHOW = 8;
    private static final int NOT_FIRST_PAGE_TO_SHOW = 9;
    private static final int TEXTVIEW_PACKETS_SHOWING = 10;
    private static final int CLEAR_LIST = 11;
    private static final int CHOOSE_FILE = 12;
    private static final int DISMISS_LOADING = 13;
    private static final int DISMISS_INFO_TEXT = 16;
    ListView lvShark;
    RelativeLayout llControl;
    ImageView bBack;
    ImageView bForward;
    TextView tvPacketsShowing;
    TextView tvWiresharkInfo;
    ArrayAdapter<SharkListElement> adapter;
    ArrayList<SharkListElement> sharkList;
    ArrayList<SharkListElement> sharkListUpdater = new ArrayList<SharkListElement>();
    Handler guiHandler;
    CatLoadingView loadingView;
    PcapFileReader reader;

    private ChannelSelectDialog channelSelectDialog;

    PacketDialog packetDialog;
    FilenameDialog filenameDialog;
    String tmpFile = "";
    int packetStart = 1;
    private WiresharkService wiresharkService;
    private BroadcastReceiver frameServiceReceiver;
    private boolean isStaticUpdate = false;

    private ServiceConnection serviceConnection = new ServiceConnection() {

        @Override
        public void onServiceDisconnected(ComponentName name) {
            wiresharkService = null;

        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            WiresharkService.MyBinder binder = (WiresharkService.MyBinder) service;
            wiresharkService = binder.getService();
            //sharkList = wiresharkService.getList();
            if (wiresharkService.isCapturing())
                guiHandler.sendEmptyMessage(UPDATE_LISTVIEW);
            else
                updateListStatic();

        }
    };


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setRetainInstance(true);
        setHasOptionsMenu(true);


    }


    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        inflater.inflate(R.menu.shark_menu, menu);
        super.onCreateOptionsMenu(menu, inflater);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);


        outState.putInt("packetStart", packetStart);
        outState.putString("tmpFile", tmpFile);
        if(wiresharkService != null)
            outState.putBoolean("wasCapturing", wiresharkService.isCapturing());
        if (reader != null) {
            outState.putString("reader", reader.getFileName());
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        Intent intent = new Intent(getActivity().getApplicationContext(), WiresharkService.class);

        frameServiceReceiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {
                guiHandler.sendEmptyMessage(UPDATE_LISTVIEW);
            }
        };
        MyApplication.getAppContext().bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
        IntentFilter intentFilter = new IntentFilter("de.tu_darmstadt.seemoo.nexmon.NEW_FRAME");
        getActivity().registerReceiver(frameServiceReceiver, intentFilter);

        if(!sharkList.isEmpty())
            tvWiresharkInfo.setVisibility(View.GONE);

    }

    @Override
    public void onPause() {
        super.onPause();
        getActivity().unregisterReceiver(frameServiceReceiver);
        MyApplication.getAppContext().unbindService(serviceConnection);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        if (container != null) {
            container.removeAllViews();
        }
        View view = inflater.inflate(R.layout.fragment_shark, container, false);

        tvWiresharkInfo = (TextView) view.findViewById(R.id.tv_wireshark_info);
        lvShark = (ListView) view.findViewById(R.id.lv_shark);
        llControl = (RelativeLayout) view.findViewById(R.id.rl_control);
        bBack = (ImageView) view.findViewById(R.id.b_back);
        bForward = (ImageView) view.findViewById(R.id.b_forward);
        tvPacketsShowing = (TextView) view.findViewById(R.id.tv_packets_showing);

        sharkList = new ArrayList<>();
        adapter = new SharkListArrayAdapter(MyApplication.getAppContext(), sharkList);

        lvShark.setAdapter(adapter);
        lvShark.setTranscriptMode(ListView.TRANSCRIPT_MODE_NORMAL);
        return view;
    }


    @Override
    public void onActivityResult(int requestCode, int resultCode, final Intent data) {
        if (requestCode == CHOOSE_FILE && data != null && data.getData() != null) {
            loadFromFile(data.getData().getPath(), true);
        }
    }


    private void loadFromFile(String filePath, boolean firstPage) {
        try {
            MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).edit().putString("reader", filePath).commit();
            reader = new PcapFileReader(filePath);
            packetStart = 1;
            updateListStatic();
        } catch(Exception e) {e.printStackTrace();
            guiHandler.sendEmptyMessage(DISMISS_LOADING);
            showToast(e.getMessage());
        }

    }

    private void updateListStatic() {
        new Thread(new Runnable() {

            @Override
            public void run() {
                String filePath = MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).getString("reader", "");
                if (filePath != null && !filePath.equals("") && new File(filePath).exists() && !isStaticUpdate) {

                    try {


                        isStaticUpdate = true;
                        guiHandler.sendEmptyMessage(SHOW_LOADING);


                        reader = new PcapFileReader(filePath);

                        ArrayList<Packet> packetList = reader.getPackets(packetStart, PACKET_AMOUNT_TO_SHOW);
                        guiHandler.sendEmptyMessage(CLEAR_LIST);

                        sharkListUpdater.clear();

                        for (Packet packet : packetList) {
                                sharkListUpdater.add(SharkListElement.getSharkListElementSmall(packet));
                                packet.cleanDissection();
                        }


                        guiHandler.sendEmptyMessage(UPDATE_LISTVIEW_STATIC);

                        packetList = null;
                        int end;
                        int size = reader.getPositions().size();

                        if (size > PACKET_AMOUNT_TO_SHOW)
                            guiHandler.sendEmptyMessage(SHOW_CONTROL_VIEW);
                        else
                            guiHandler.sendEmptyMessage(CLOSE_CONTROL_VIEW);

                        guiHandler.sendEmptyMessage(LOADED_FROM_FILE);

                        if (size < packetStart + PACKET_AMOUNT_TO_SHOW) {
                            end = size;
                            guiHandler.sendEmptyMessage(LAST_PAGE_TO_SHOW);
                        } else {
                            guiHandler.sendEmptyMessage(NOT_LAST_PAGE_TO_SHOW);
                            end = packetStart + PACKET_AMOUNT_TO_SHOW - 1;
                        }

                        if (packetStart <= PACKET_AMOUNT_TO_SHOW)
                            guiHandler.sendEmptyMessage(FIRST_PAGE_TO_SHOW);
                        else
                            guiHandler.sendEmptyMessage(NOT_FIRST_PAGE_TO_SHOW);

                        Message msg = new Message();
                        msg.what = TEXTVIEW_PACKETS_SHOWING;
                        msg.obj = packetStart + " - " + end;
                        guiHandler.sendMessage(msg);
                    } catch(Exception e) {
                        e.printStackTrace();
                        guiHandler.sendEmptyMessage(CLEAR_LIST);
                        guiHandler.sendEmptyMessage(CLOSE_CONTROL_VIEW);
                        guiHandler.sendEmptyMessage(DISMISS_LOADING);
                    } finally {
                        isStaticUpdate = false;
                        guiHandler.sendEmptyMessage(DISMISS_INFO_TEXT);
                    }
                }


            }
        }).start();
    }


    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);


        if (savedInstanceState != null && savedInstanceState.containsKey("wasCapturing") && !savedInstanceState.getBoolean("wasCapturing")) {
            if (savedInstanceState.containsKey("tmpFile"))
                tmpFile = savedInstanceState.getString("tmpFile");
            if (savedInstanceState.containsKey("packetStart"))
                packetStart = savedInstanceState.getInt("packetStart");

            updateListStatic();
        }


        guiHandler = new Handler() {

            @SuppressWarnings("unchecked")
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                try {


                    switch (msg.what) {
                        case UPDATE_LISTVIEW_STATIC:
                            //sharkList = wiresharkService.getList();
                            //sharkList.add(wiresharkService.getList().get(wiresharkService.getList().size()-1));
                            sharkList.clear();
                            sharkList.addAll(sharkListUpdater);
                            adapter.notifyDataSetChanged();
                            break;
                        case UPDATE_LISTVIEW_STATIC_TEST:
                            updateListStatic();
                            break;
                        case UPDATE_LISTVIEW:
                            //sharkList = wiresharkService.getList();
                            //sharkList.add(wiresharkService.getList().get(wiresharkService.getList().size()-1));
                            sharkList.clear();
                            sharkList.addAll(wiresharkService.getList());
                            adapter.notifyDataSetChanged();
                            tvWiresharkInfo.setVisibility(View.GONE);
                            break;
                        case LOADED_FROM_FILE:
                            adapter.notifyDataSetChanged();
                            try {
                                loadingView.dismiss();
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                            break;
                        case SHOW_LOADING:
                            loadingView = new CatLoadingView();
                            loadingView.setCancelable(false);
                            loadingView.show(getFragmentManager(), "");
                            break;
                        case CLOSE_CONTROL_VIEW:
                            llControl.setVisibility(View.GONE);
                            break;
                        case SHOW_CONTROL_VIEW:
                            llControl.setVisibility(View.VISIBLE);
                            break;
                        case FIRST_PAGE_TO_SHOW:
                            bBack.setClickable(false);
                            bBack.setVisibility(View.INVISIBLE);
                            break;
                        case NOT_FIRST_PAGE_TO_SHOW:
                            bBack.setClickable(true);
                            bBack.setVisibility(View.VISIBLE);
                            break;
                        case LAST_PAGE_TO_SHOW:
                            bForward.setClickable(false);
                            bForward.setVisibility(View.INVISIBLE);
                            break;
                        case NOT_LAST_PAGE_TO_SHOW:
                            bForward.setClickable(true);
                            bForward.setVisibility(View.VISIBLE);
                            break;
                        case TEXTVIEW_PACKETS_SHOWING:
                            try {
                                tvPacketsShowing.setText((CharSequence) msg.obj);
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                            break;
                        case CLEAR_LIST:
                            sharkList.clear();
                            adapter.notifyDataSetChanged();
                            break;
                        case DISMISS_LOADING:
                            loadingView.dismiss();
                            break;
                        case DISMISS_INFO_TEXT:
                            tvWiresharkInfo.setVisibility(View.GONE);
                            break;
                    }
                } catch(Exception e) {
                    e.printStackTrace();
                }
            }
        };

        bBack.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                packetStart -= PACKET_AMOUNT_TO_SHOW;
                updateListStatic();
            }
        });

        bForward.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                packetStart += PACKET_AMOUNT_TO_SHOW;
                updateListStatic();
            }
        });

        lvShark.setOnItemClickListener(new OnItemClickListener() {

            @Override
            public void onItemClick(AdapterView<?> parent, View view,
                                    int position, long id) {
                SharkListElement element = (SharkListElement) parent.getAdapter().getItem(position);
                if (wiresharkService.isCapturing()) {
                    reader = new PcapFileReader(MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).getString("reader", ""));
                    //reader = new PcapFileReader("/sdcard/" + wiresharkService.getFileName());
                }
                try {
                    Packet packet = reader.getPackets(element.getNumber(), 1).get(0);
                    showPacketDialog(packet);
                } catch(Exception e) {
                    e.printStackTrace();
                    showToast(e.getMessage());
                }

            }
        });

        try {
            getActivity().setTitle("Nexmon: Wireshark");
        } catch(Exception e) {e.printStackTrace();}

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
        if (wiresharkService.isCapturing())
            menu.findItem(R.id.action_toggle_capturing).setTitle("Stop live capturing");
        else
            menu.findItem(R.id.action_toggle_capturing).setTitle("Start live capturing");

    }

    private void selectFile(int requestCode) {
        Intent i = new Intent(Intent.ACTION_GET_CONTENT);

        i.putExtra(FilePickerActivity.EXTRA_ALLOW_MULTIPLE, false);
        i.putExtra(FilePickerActivity.EXTRA_ALLOW_CREATE_DIR, true);
        i.putExtra(FilePickerActivity.EXTRA_MODE, FilePickerActivity.MODE_FILE);
        i.putExtra(FilePickerActivity.EXTRA_START_PATH, Environment.getExternalStorageDirectory().getPath());
        startActivityForResult(i, requestCode);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_change_channel_shark:
                channelSelectDialog = ChannelSelectDialog.newInstance();
                channelSelectDialog.show(getFragmentManager(), "");
                break;
            case R.id.action_load_file:
                wiresharkService.stopLiveCapturing();
                selectFile(CHOOSE_FILE);
                tvWiresharkInfo.setVisibility(View.GONE);
                break;
            case R.id.action_save_file:
                if (wiresharkService.isCapturing())
                    showToast("Stop live capturing first!");
                else if (sharkList.isEmpty())
                    showToast("Nothing to save!");
                else
                    showSaveFileDialog();

                break;
            case R.id.action_toggle_capturing:
                tvWiresharkInfo.setVisibility(View.GONE);
                wiresharkService.toggleLiveCapturing();
                if (!wiresharkService.isCapturing()) {
                    String fileP = MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).getString("reader", "");
                    loadFromFile(fileP, true);
                } else {
                    guiHandler.sendEmptyMessage(CLOSE_CONTROL_VIEW);
                    guiHandler.sendEmptyMessage(CLEAR_LIST);
                }
                break;
            case R.id.action_clear_list:
                try {

                    if (wiresharkService.isCapturing())
                        wiresharkService.clearList();
                    else {
                        sharkListUpdater.clear();
                        MyApplication.deleteTempFile();
                        reader = null;
                        guiHandler.sendEmptyMessage(UPDATE_LISTVIEW_STATIC);
                        tmpFile = "";
                        guiHandler.sendEmptyMessage(CLOSE_CONTROL_VIEW);
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            default:
                break;
        }
        return true;
    }


    private void showPacketDialog(Packet packet) {
        packetDialog = new PacketDialog(packet);
        packetDialog.show(getFragmentManager(), "");
    }

    private void showSaveFileDialog() {
        String fileP = MyApplication.getAppContext().getSharedPreferences("de.tu_darmstadt.seemoo.nexmon.SHARED_GLOBALS", Context.MODE_PRIVATE).getString("reader", "");
        filenameDialog = new FilenameDialog(fileP);
        filenameDialog.show(getFragmentManager(), "");
    }


    public void showToast(String text) {
        try {
            Toast.makeText(MyApplication.getAppContext(), text,
                    Toast.LENGTH_LONG).show();
        } catch(Exception e) {e.printStackTrace();}

    }



}
