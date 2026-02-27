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
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.ListPreference;
import android.preference.PreferenceFragment;
import android.preference.SwitchPreference;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.stericson.RootShell.execution.Command;
import com.stericson.RootTools.RootTools;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.commands.CurrentChannelCommand;
import de.tu_darmstadt.seemoo.nexmon.commands.CurrentChannelListener;
import de.tu_darmstadt.seemoo.nexmon.net.MonitorModeService;


/**
 * A simple {@link Fragment} subclass.
 */
public class SettingsFragment extends PreferenceFragment implements SharedPreferences.OnSharedPreferenceChangeListener, CurrentChannelListener {

    CurrentChannelCommand currentChannelCommand;
    SwitchPreference switchPreferenceWlan;
    SwitchPreference switchSurveyNotification;
    ListPreference listPreferenceChannel;

    private static final int GUI_UPDATE_WLAN_SWITCH = 31;
    private static final int GUI_UPDATE_WLAN_CHANNEL = 32;
    private static final int ROOT_GRANTED = 41;
    private static final int ROOT_DENIED = 42;
    private static final int COMMAND_WLAN_ACTIVE = 51;
    private static final int COMMAND_PULLUP_WLAN = 52;
    private static final int COMMAND_PULLDOWN_WLAN = 53;
    private static final int COMMAND_SET_CHANNEL = 54;

    Handler guiHandler;

    public SettingsFragment() {

    }


    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Obtain the shared Tracker instance.
        MyApplication application = (MyApplication) getActivity().getApplication();

        addPreferencesFromResource(R.xml.pentest_preferences);

        switchPreferenceWlan = (SwitchPreference) findPreference("switch_wlan_preference");
        listPreferenceChannel = (ListPreference) findPreference("list_wlan_channel");
        switchSurveyNotification = (SwitchPreference) findPreference("switch_survey_notification");

        setupHandler();
    }

    private void setupHandler() {
        guiHandler = new Handler() {

            @SuppressWarnings("unchecked")
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                try {
                    switch (msg.what) {

                        case GUI_UPDATE_WLAN_CHANNEL:
                            listPreferenceChannel.setValueIndex((int) msg.obj);
                            break;
                        case GUI_UPDATE_WLAN_SWITCH:
                            setWlan((boolean) msg.obj);
                            break;
                        case ROOT_DENIED:
                            MyApplication.toast("We need root!");
                            break;
                        case ROOT_GRANTED:
                            checkForCurrentChannel();
                            break;
                    }
                } catch(Exception e) {
                    e.printStackTrace();
                }
            }
        };
    }

    private void setWlan(boolean active) {
        getPreferenceManager().getSharedPreferences().edit().putBoolean("switch_wlan_preference", active).commit();
        listPreferenceChannel.setEnabled(active);
        switchPreferenceWlan.setEnabled(true);
    }




    private void checkForCurrentChannel() {
        // Aktuell aktiven channel ermitteln und in den preferences setzen
        currentChannelCommand = new CurrentChannelCommand(this);
        try {
            RootTools.getShell(true).add(currentChannelCommand);
        } catch(Exception e) {e.printStackTrace();}
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        if (container != null) {
            container.removeAllViews();
        }
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        getPreferenceManager().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);

        // Disable: We dont know the status yet.
        listPreferenceChannel.setEnabled(false);
        switchPreferenceWlan.setEnabled(false);

        // WLAN status abfragen
        isWlanActive();

        requestRoot();
    }

    private void requestRoot() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if(RootTools.isAccessGiven())
                    guiHandler.sendEmptyMessage(ROOT_GRANTED);
                else
                    guiHandler.sendEmptyMessage(ROOT_DENIED);
            }
        }).start();
    }

    private void isWlanActive() {
        Command command = new Command(COMMAND_WLAN_ACTIVE, "ifconfig | grep " + MyApplication.WLAN_INTERFACE) {

            private boolean wlan0Active = false;

            @Override
            public void commandOutput(int id, String line) {
                if(id == COMMAND_WLAN_ACTIVE)
                    checkLine(line);

                super.commandOutput(id, line);
            }

            private void checkLine(String line) {
                if (line != null && line.contains(MyApplication.WLAN_INTERFACE)) {
                    wlan0Active = true;
                }
            }

            @Override
            public void commandCompleted(int id, int exitcode) {
                if(id == COMMAND_WLAN_ACTIVE) {
                    Message msg = guiHandler.obtainMessage();
                    msg.what = GUI_UPDATE_WLAN_SWITCH;
                    msg.obj = wlan0Active;
                    guiHandler.sendMessage(msg);
                }
                super.commandCompleted(id, exitcode);
            }
        };

        try {
            RootTools.getShell(false).add(command);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        getPreferenceManager().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
        currentChannelCommand.removeListener();
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String s) {

        switch (s) {
            case "switch_wlan_preference":
                boolean isActive = sharedPreferences.getBoolean(s, true);
                if (isActive) {
                    pullUpWlan();
                } else {
                    pullDownWlan();
                }

                listPreferenceChannel.setEnabled(isActive);
                switchPreferenceWlan.setChecked(isActive);
                break;
            case "list_wlan_channel":
                String channel = sharedPreferences.getString(s, "0");
                int channelInt;
                try {
                    channelInt = Integer.parseInt(channel);
                } catch(Exception e) {
                    e.printStackTrace();
                    channelInt = 1;
                }

                setWlanChannel(channelInt);

                break;
            case "switch_survey_notification":
                //MyApplication.showSurveyNotification();
                break;

            default:
                break;
        }
    }

    private void pullUpWlan() {
        final Command command = new Command(COMMAND_PULLUP_WLAN, "ifconfig " + MyApplication.WLAN_INTERFACE + " up") {
            @Override
            public void commandCompleted(int id, int exitcode) {
                if(id == COMMAND_PULLUP_WLAN) {

                    // Trigger MonitorModeService Update.
                    Intent intent = new Intent(MonitorModeService.INTENT_RECEIVER);
                    MyApplication.getAppContext().sendBroadcast(intent);

                    Message msg = guiHandler.obtainMessage();
                    msg.what = GUI_UPDATE_WLAN_CHANNEL;
                    msg.obj = 0;
                    guiHandler.sendMessage(msg);
                }

                super.commandCompleted(id, exitcode);
            }
        };

        new Thread(new Runnable() {
            @Override
            public void run() {
                if(RootTools.isAccessGiven()) {
                    try {
                        RootTools.getShell(true).add(command);
                    } catch(Exception e) {e.printStackTrace();}
                } else {
                    guiHandler.sendEmptyMessage(ROOT_DENIED);
                }
            }
        }).start();
    }

    private void pullDownWlan() {
        final Command command = new Command(COMMAND_PULLDOWN_WLAN, "ifconfig " + MyApplication.WLAN_INTERFACE + " down");

        new Thread(new Runnable() {
            @Override
            public void run() {
                if(RootTools.isAccessGiven()) {
                    try {
                        RootTools.getShell(true).add(command);
                    } catch(Exception e) {e.printStackTrace();}
                } else
                    guiHandler.sendEmptyMessage(ROOT_DENIED);
            }
        }).start();
    }

    private void setWlanChannel(int channel) {
        final Command command = new Command(COMMAND_SET_CHANNEL, "nexutil -i -s 30 -v " + channel);

        new Thread(new Runnable() {
            @Override
            public void run() {
                if(RootTools.isAccessGiven()) {
                    try {
                        RootTools.getShell(true).add(command);
                    } catch(Exception e) {e.printStackTrace();}
                } else
                    guiHandler.sendEmptyMessage(ROOT_DENIED);
            }
        }).start();
    }


    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        try {
            getActivity().setTitle("Nexmon: Preferences");
        } catch(Exception e) {e.printStackTrace();}
    }

    @Override
    public void onCurrentChannelInfo(int channel) {
        CharSequence[] values = listPreferenceChannel.getEntryValues();
        int position = 0;
        for (int i = 0; i < values.length; i++) {
            if (values[i].equals(String.valueOf(channel))) {
                position = i;
                break;
            }
        }

        listPreferenceChannel.setValueIndex(position);
    }

    @Override
    public void onCurrentChannelError(String error) {
        Log.e("SettingsFragment", error);
    }
}
