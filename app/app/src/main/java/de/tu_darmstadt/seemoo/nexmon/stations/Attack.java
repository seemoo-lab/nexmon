package de.tu_darmstadt.seemoo.nexmon.stations;

import android.content.Intent;
import android.graphics.Color;
import android.text.SpannableStringBuilder;

import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.HashMap;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;

/**
 * Created by fabian on 9/1/16.
 */
public abstract class Attack implements Runnable {
    public static final int STATUS_NOT_STARTED = 59;
    public static final int STATUS_RUNNING = 60;
    public static final int STATUS_CANCELING = 61;
    public static final int STATUS_FINISHED = 62;

    public static final String ATTACK_FAKE_AUTH = "FakeAuth";
    public static final String ATTACK_DE_AUTH = "DeAuth";
    public static final String ATTACK_ARP_REPLAY = "ArpReplay";
    public static final String ATTACK_WEP_CRACK = "WepCrack";
    public static final String ATTACK_BEACON_FLOOD = "BeaconFlood";
    public static final String ATTACK_AUTH_FLOOD = "AuthFlood";
    public static final String ATTACK_COUNTERMEASURES = "Countermeasures";
    public static final String ATTACK_WIDS = "Wids";
    public static final String ATTACK_UPC = "Upc";
    public static final String ATTACK_REAVER = "Reaver";
    public static final String ATTACK_WPA_DICT = "WpaDict";

    public static final String ATTACK_ID = "attack_id";
    public static final String ATTACK_UPDATE_REASON = "attack_update_reason";
    public static final String ATTACK_UPDATE_TEXT = "attack_update_text";



    public static final HashMap<String, Type> ATTACK_TYPE = new HashMap<String, Type>() {{
        put(ATTACK_FAKE_AUTH, FakeAuthAttack.class);
        put(ATTACK_DE_AUTH, DeAuthAttack.class);
        put(ATTACK_ARP_REPLAY, ArpReplayAttack.class);
        put(ATTACK_WEP_CRACK, WepCrackAttack.class);
        put(ATTACK_BEACON_FLOOD, BeaconFloodAttack.class);
        put(ATTACK_AUTH_FLOOD, AuthFloodAttack.class);
        put(ATTACK_COUNTERMEASURES, CountermeasuresAttack.class);
        put(ATTACK_WIDS, WidsAttack.class);
        put(ATTACK_UPC, UpcAttack.class);
        put(ATTACK_REAVER, ReaverAttack.class);
        put(ATTACK_WPA_DICT, WpaDictAttack.class);

    }};

    public static HashMap<String, Integer> ATTACK_MAX_INSTANCES = new HashMap<String, Integer>() {{
        put(ATTACK_FAKE_AUTH, 1);
        put(ATTACK_DE_AUTH, 1);
        put(ATTACK_ARP_REPLAY, 1);
        put(ATTACK_WEP_CRACK, 1);
        put(ATTACK_BEACON_FLOOD, 1);
        put(ATTACK_AUTH_FLOOD, 1);
        put(ATTACK_COUNTERMEASURES, 1);
        put(ATTACK_WIDS, 1);
        put(ATTACK_UPC, 1);
        put(ATTACK_REAVER, 1);
        put(ATTACK_WPA_DICT, 1);
    }};
    public static final String DELIMITER = "***************";

    public static HashMap<String, Integer> attackRemainingInstances = new HashMap<>(ATTACK_MAX_INSTANCES);

    protected static int attackCounter = 0;

    private int guid;

    public long getStartTime() {
        return startTime;
    }

    private long startTime;

    public long getFinishTime() {
        return finishTime;
    }

    private long finishTime;

    public long getCancelTime() {
        return cancelTime;
    }

    private long cancelTime;

    protected int status = STATUS_NOT_STARTED;

    protected boolean isCanceled = false;

    protected ArrayList<AttackInfoString> attackInfo;


    private static IAttackInstanceUpdate attackInstanceUpdateObserver;

    private AccessPoint ap;
    public AccessPoint getAp() {
        return ap;
    }
    protected static void updateObserver() {
        if(attackInstanceUpdateObserver != null)
            attackInstanceUpdateObserver.onAttackInstanceUpdate(attackRemainingInstances);
    }

    protected static void updateAmountOfInstances(Intent intent) {
        int runningInstances;
        int remainingInstances;

        for(String attackString : ATTACK_MAX_INSTANCES.keySet()) {
            if(intent.hasExtra(attackString)) {
                runningInstances = intent.getIntExtra(attackString, 0);
                remainingInstances = ATTACK_MAX_INSTANCES.get(attackString) - runningInstances;
                attackRemainingInstances.put(attackString, remainingInstances);
            } else
                attackRemainingInstances.put(attackString, ATTACK_MAX_INSTANCES.get(attackString));

        }
    }

    public Attack(AccessPoint ap) {
        attackInfo = new ArrayList<>();
        guid = attackCounter++;
        this.ap = ap;
    }

    public abstract String getName();

    public abstract String getTypeString();

    public abstract boolean needsMonitorMode();

    public void stopAttack() {
        cancelTime = System.currentTimeMillis();
        isCanceled = true;
        status = STATUS_CANCELING;
        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_UPDATE");
        intent.putExtra("id", getGuid());
        intent.putExtra("text", "Attack is canceling...");
        intent.putExtra("status", STATUS_CANCELING);
        MyApplication.getAppContext().sendBroadcast(intent);
    }

    public abstract int getMaxInstances();

    public abstract void doAttack();

    @Override
    public void run() {
        status = STATUS_RUNNING;
        startTime = System.currentTimeMillis();
        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_UPDATE");
        intent.putExtra("id", getGuid());
        intent.putExtra("text", "Attack is running...");
        intent.putExtra("status", STATUS_RUNNING);
        MyApplication.getAppContext().sendBroadcast(intent);

        doAttack();

        finishTime = System.currentTimeMillis();
        status = STATUS_FINISHED;
        intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_SERVICE_UPDATE");
        intent.putExtra("id", getGuid());
        intent.putExtra("text", "Attack is finished!");
        intent.putExtra("status", STATUS_FINISHED);
        MyApplication.getAppContext().sendBroadcast(intent);
    }

    public int getGuid() {
        return guid;
    }

    public ArrayList<AttackInfoString> getRunningInfo() {
        return attackInfo;
    }

    public SpannableStringBuilder getRunningInfoString() {
        SpannableStringBuilder builder = new SpannableStringBuilder();
        builder.append("");
        for (AttackInfoString infoString : attackInfo) {
            MyApplication.appendWithColor(builder, infoString.message, getColor(infoString.messageType));
        }

        return builder;
    }

    private int getColor(int updateCause) {
        int color = Color.BLACK;
        switch (updateCause) {
            case AttackInfoString.ATTACK_UPDATE_ERROR:
                color = Color.RED;
                break;
            case AttackInfoString.ATTACK_UPDATE_RUNNING:
                color = Color.LTGRAY;
                break;
            case AttackInfoString.ATTACK_UPDATE_SUCCESS:
                color = Color.GREEN;
                break;
            default:
                break;
        }
        return color;
    }

    public int getStatus() {
        return status;
    }

    public static void setObserver(IAttackInstanceUpdate observer) {
        attackInstanceUpdateObserver = observer;
        updateObserver();
    }

    public static void deleteObserver() {
        attackInstanceUpdateObserver = null;
    }

    protected void updateAttackText(String updateText, int updateReason, int attackId) {
        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.ATTACK_INFO_UPDATE");
        intent.putExtra(ATTACK_ID, attackId);
        intent.putExtra(ATTACK_UPDATE_REASON, updateReason);
        intent.putExtra(ATTACK_UPDATE_TEXT, updateText);

        MyApplication.getAppContext().sendBroadcast(intent);
    }

    public interface IAttackInstanceUpdate {
        void onAttackInstanceUpdate(HashMap<String, Integer> remainingInstances);
    }
}
