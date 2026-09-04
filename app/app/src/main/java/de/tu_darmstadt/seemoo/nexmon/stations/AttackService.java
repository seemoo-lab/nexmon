package de.tu_darmstadt.seemoo.nexmon.stations;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.Icon;
import android.os.IBinder;

import com.google.gson.Gson;
import android.util.Log;
import java.lang.reflect.Type;

import java.util.HashMap;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.R;
import de.tu_darmstadt.seemoo.nexmon.gui.AttackInfoActivity;
import de.tu_darmstadt.seemoo.nexmon.net.MonitorModeService;

public class AttackService extends Service {

    private HashMap<Integer, Attack> attacks;

    private BroadcastReceiver receiver;




    @Override
    public void onCreate() {
        super.onCreate();
        attacks = new HashMap<>();
        receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if(intent.getAction().equals("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE"))
                    newAttack(intent);
                else if(intent.getAction().equals("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_STOP"))
                    stopAttack(intent);
                else if(intent.getAction().equals("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_UPDATE"))
                    updateAttack(intent);
                else if(intent.getAction().equals("de.tu_darmstadt.seemoo.nexmon.ATTACK_INFO_UPDATE"))
                    updateText(intent);


            }

            private void newAttack(Intent intent) {
                String attackType = intent.getStringExtra("ATTACK_TYPE");
                // An unknown/absent ATTACK_TYPE makes Attack.ATTACK_TYPE.get()
                // return null, which would crash Gson.fromJson(); reject it
                // instead of taking down the service.
                Type type = (attackType != null) ? Attack.ATTACK_TYPE.get(attackType) : null;
                if(type == null) {
                    Log.w("AttackService", "ignoring attack with unknown type: " + attackType);
                    return;
                }
                Attack attack = new Gson().fromJson(intent.getStringExtra("ATTACK"), type);
                if(attack == null) {
                    Log.w("AttackService", "ignoring malformed attack payload");
                    return;
                }
                if(!attacks.containsKey(attack.getGuid()))
                    startNewAttack(attack);
            }

            private void stopAttack(Intent intent) {
                int id = intent.getIntExtra("ATTACK_ID",-1);
                if(attacks.containsKey(id)) {
                    Attack attack = attacks.get(id);
                    attack.stopAttack();



                } else
                    MyApplication.getNotificationManager().cancel(id);
            }

            private void updateAttack(Intent intent) {
                if(intent != null && intent.hasExtra("id") && intent.hasExtra("text") && intent.hasExtra("status")) {
                    int attackId = intent.getIntExtra("id", -1);
                    String attackText = intent.getStringExtra("text");
                    int status = intent.getIntExtra("status", -1);

                    if(attacks.containsKey(attackId)) {
                        Attack attack = attacks.get(attackId);
                        showNotification(attack, attack.getTypeString(), attackText);

                        if(status == Attack.STATUS_FINISHED) {

                            attacks.remove(attackId);
                            broadcastInstances();
                            evaluateMonitorModeNeed();
                            if(attack.isCanceled) {
                                MyApplication.getNotificationManager().cancel(attackId);
                            }
                        }
                    }
                }
            }

            private void updateText(Intent intent) {
                int id = intent.getIntExtra(Attack.ATTACK_ID, -1);
                if(attacks.containsKey(id)) {
                    Attack attack = attacks.get(id);

                    if(intent.hasExtra(Attack.ATTACK_UPDATE_TEXT) && intent.hasExtra(Attack.ATTACK_UPDATE_REASON)) {
                        String updateText = intent.getStringExtra(Attack.ATTACK_UPDATE_TEXT);
                        if(updateText.contains(Attack.DELIMITER)) {
                            for(int i = 0; i < attack.getRunningInfo().size(); i++) {
                                if(attack.getRunningInfo().get(i).message.contains(Attack.DELIMITER)) {
                                    attack.getRunningInfo().remove(i);
                                }
                            }
                        }

                        int updateReason = intent.getIntExtra(Attack.ATTACK_UPDATE_REASON, AttackInfoString.ATTACK_UPDATE_RUNNING);
                        AttackInfoString attackInfoString = new AttackInfoString(updateReason, updateText);
                        attack.getRunningInfo().add(attackInfoString);
                    }

                    Intent sendIntent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_GET");
                    sendIntent.putExtra("ATTACK_TYPE", attack.getTypeString());
                    sendIntent.putExtra("ATTACK", new Gson().toJson(attack));

                    MyApplication.getAppContext().sendBroadcast(sendIntent);
                }
            }
        };



        IntentFilter filter = new IntentFilter("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE");
        IntentFilter filterTwo = new IntentFilter("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_STOP");
        IntentFilter filterThree = new IntentFilter("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_UPDATE");
        IntentFilter filerFour = new IntentFilter("de.tu_darmstadt.seemoo.nexmon.ATTACK_INFO_UPDATE");
        // Gate these control broadcasts (start/stop/update attacks) with the
        // app's signature-level permission so another installed app cannot
        // drive attack threads. Same-app senders hold the permission, so
        // functionality is unchanged.
        registerReceiver(receiver, filter, MyApplication.PERMISSION_INTERNAL_BROADCAST, null);
        registerReceiver(receiver, filterTwo, MyApplication.PERMISSION_INTERNAL_BROADCAST, null);
        registerReceiver(receiver, filterThree, MyApplication.PERMISSION_INTERNAL_BROADCAST, null);
        registerReceiver(receiver, filerFour, MyApplication.PERMISSION_INTERNAL_BROADCAST, null);
    }

    private void evaluateMonitorModeNeed() {
        Intent intent = new Intent(MonitorModeService.INTENT_RECEIVER);
        intent.putExtra(MonitorModeService.MONITOR_MODE_ID, MonitorModeService.MONITOR_MODE_ATTACKS);
        intent.putExtra(MonitorModeService.MONITOR_MODE_NEED, needsMonitorMode());

        MyApplication.getAppContext().sendBroadcast(intent);
    }



    private boolean needsMonitorMode() {
        for(Attack attack : attacks.values()) {
            if(attack.needsMonitorMode() && (attack.getStatus() != Attack.STATUS_FINISHED)) {
                return true;
            }
        }
        return false;
    }

    private void broadcastInstances() {
        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_INSTANCES");

        HashMap<String, Integer> attackInstances = getAmountOfAttackInstances();

        for(String attackType : attackInstances.keySet()) {
            intent.putExtra(attackType, attackInstances.get(attackType));
        }

        MyApplication.getAppContext().sendBroadcast(intent);
    }


    private HashMap<String, Integer> getAmountOfAttackInstances() {
        HashMap<String, Integer> attackInstances = new HashMap<>();
        for(Attack attack : attacks.values()) {
            if(attackInstances.containsKey(attack.getTypeString()))
                attackInstances.put(attack.getTypeString(), attackInstances.get(attack.getTypeString()).intValue() + 1);
            else
                attackInstances.put(attack.getTypeString(), 1);
        }
        return attackInstances;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        //LocalBroadcastManager.getInstance(MyApplication.getAppContext()).unregisterReceiver(receiver);
        unregisterReceiver(receiver);
        //unregisterReceiver(receiverTwo);
        //unregisterReceiver(receiverThree);
    }

    public void startNewAttack(Attack attack) {
        new Thread(attack).start();
        attacks.put(attack.getGuid(), attack);
        broadcastInstances();
        evaluateMonitorModeNeed();

        //showNotification(attack);
    }

    public void showNotification(Attack attack, String attackType, String text) {
        Intent intentInfo = new Intent(MyApplication.getAppContext(), AttackInfoActivity.class);
        intentInfo.putExtra("ATTACK_TYPE", attackType);
        intentInfo.putExtra("ATTACK", new Gson().toJson(attack, Attack.ATTACK_TYPE.get(attack.getTypeString())));
       // intentInfo.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        PendingIntent infoPendingIntent = PendingIntent.getActivity(MyApplication.getAppContext(), attack.getGuid(), intentInfo, PendingIntent.FLAG_UPDATE_CURRENT);
        Icon iconInfo = Icon.createWithResource(MyApplication.getAppContext(), R.drawable.ic_info_outline_white_24dp);

        Notification.Action actionInfo = new Notification.Action.Builder(iconInfo, "INFO", infoPendingIntent).build();

        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_STOP");
        intent.putExtra("ATTACK_ID", attack.getGuid());
        PendingIntent pendingIntent = PendingIntent.getBroadcast(MyApplication.getAppContext(), attack.getGuid(), intent, PendingIntent.FLAG_UPDATE_CURRENT);
        Icon icon = Icon.createWithResource(MyApplication.getAppContext(), R.drawable.ic_close_black_24dp);
        Notification.Action action = new Notification.Action.Builder(icon, "CANCEL", pendingIntent).build();
        Notification n  = new Notification.Builder(this)
                .setContentTitle(attack.getName() + ", ID: " + attack.getGuid())
                .setContentText(text)
                .setAutoCancel(false)
                .setSmallIcon(R.drawable.x_logo)
                .setOngoing(true)
                .addAction(action)
                .addAction(actionInfo)
                .build();

        MyApplication.getNotificationManager().notify(attack.getGuid(), n);
    }


    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }



}

