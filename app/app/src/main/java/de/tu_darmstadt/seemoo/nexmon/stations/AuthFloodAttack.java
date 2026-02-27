package de.tu_darmstadt.seemoo.nexmon.stations;

/**
 * Created by fabian on 9/22/16.
 */
public class AuthFloodAttack extends Attack {

    private static final String DELIMITER = "***************";


    public native void authflood(String interfaceName, String apMac, boolean intelligent, int id);

    public native void stopauthflood(boolean stop_attack);

    private boolean intelligent;

    public AuthFloodAttack(AccessPoint ap, boolean intelligent) {
        super(ap);
        this.intelligent = intelligent;
    }

    @Override
    public String getName() {
        return "Auth Flood Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_AUTH_FLOOD;
    }

    @Override
    public boolean needsMonitorMode() {
        return true;
    }

    @Override
    public void stopAttack() {
        super.stopAttack();
        stopauthflood(true);
    }

    @Override
    public int getMaxInstances() {
        return 1;
    }

    @Override
    public void doAttack() {
        authflood("wlan0", getAp().getBssid(), intelligent, getGuid());
    }


    public void authFloodUpdate(String updateText, int updateReason, int id) {
        updateAttackText(updateText, updateReason, id);
    }

    public void authFloodStats(int packetRate, int packetAmount) {

        /*
        for(int i = 0; i < getRunningInfo().size(); i++) {
            if(getRunningInfo().get(i).message.contains(DELIMITER)) {
                getRunningInfo().remove(i);
            }
        }*/

        String info = "\n" + DELIMITER + "\n";
        info += "Packets received: " + packetAmount + "\n";
        info += "Packets sent per sec: " + packetRate + "\n";
        info += DELIMITER + "\n";

        updateAttackText(info, AttackInfoString.ATTACK_UPDATE_SUCCESS, getGuid());
    }
}
