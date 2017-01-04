package de.tu_darmstadt.seemoo.nexmon.stations;

/**
 * Created by fabian on 9/23/16.
 */
public class CountermeasuresAttack extends Attack {
    private static final String DELIMITER = "***************";

    int burstPause;
    int packetsPerBurst;
    int speed;
    boolean useQosExploit;

    public native void countermeasures(String interfaceName, String apMac, int burstPause, int packetsPerBurst, boolean useQosExplot, int speed, int id);
    public native void stopcountermeasures(boolean stopAttack);


    public CountermeasuresAttack(AccessPoint ap, int burstPause, int packetsPerBurst, int speed, boolean useQosExploit) {
        super(ap);
        this.burstPause = burstPause;
        this.packetsPerBurst = packetsPerBurst;
        this.speed = speed;
        this.useQosExploit = useQosExploit;
    }

    @Override
    public String getName() {
        return "Michael TKIP exploit";
    }

    @Override
    public String getTypeString() {
        return ATTACK_COUNTERMEASURES;
    }

    @Override
    public boolean needsMonitorMode() {
        return true;
    }

    @Override
    public void stopAttack() {
        super.stopAttack();
        stopcountermeasures(true);
    }

    @Override
    public int getMaxInstances() {
        return 1;
    }

    @Override
    public void doAttack() {
        countermeasures("wlan0", getAp().getBssid(), burstPause, packetsPerBurst, useQosExploit, speed, getGuid());
    }

    public void countermeasuresUpdate(String updateText, int updateReason, int id) {
        updateAttackText(updateText, updateReason, id);
    }

    public void countermeasuresStats(int packetRate, int packetAmount) {


        String info = "\n" + DELIMITER + "\n";
        info += "Packets received: " + packetAmount + "\n";
        info += "Packets sent per sec: " + packetRate + "\n";
        info += DELIMITER + "\n";
        updateAttackText(info, AttackInfoString.ATTACK_UPDATE_SUCCESS, getGuid());
    }


}
