package de.tu_darmstadt.seemoo.nexmon.stations;

/**
 * Created by fabian on 9/21/16.
 */
public class BeaconFloodAttack extends Attack {

    private static final String DELIMITER = "***************";


    public native void beaconflood(String interfaceName, int id);

    public native void stopbeaconflood(boolean stopAttack);

    public BeaconFloodAttack(AccessPoint ap) {
        super(ap);
    }

    @Override
    public String getName() {
        return "Beacon Flood Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_BEACON_FLOOD;
    }

    @Override
    public boolean needsMonitorMode() {
        return true;
    }

    @Override
    public void stopAttack() {
        super.stopAttack();
        stopbeaconflood(true);
    }

    @Override
    public int getMaxInstances() {
        return 1;
    }

    @Override
    public void doAttack() {
        beaconflood("wlan0", getGuid());
    }



    public void beaconFloodUpdate(String updateText, int updateReason, int id) {
        updateAttackText(updateText, updateReason, id);
    }

    public void beaconFloodStats(int packetRate, int packetAmount) {


        String info = "\n" + DELIMITER + "\n";
        info += "Packets received: " + packetAmount + "\n";
        info += "Packets sent per sec: " + packetRate + "\n";
        info += DELIMITER + "\n";
        updateAttackText(info, AttackInfoString.ATTACK_UPDATE_SUCCESS, getGuid());
    }

}
