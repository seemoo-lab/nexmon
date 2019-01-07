package de.tu_darmstadt.seemoo.nexmon.stations;

/**
 * Created by fabian on 9/24/16.
 */
public class WidsAttack extends Attack {
    private static final String DELIMITER = "***************";

    boolean useChanHopping;
    boolean useZeroChaos;

    public native void wids(String interfaceName, String apEssid, boolean useChannelHopping, boolean useZeroChaos, int id);

    public native void stopwids(boolean stopAttack);

    public WidsAttack(AccessPoint ap, boolean useChanHopping, boolean useZeroChaos) {
        super(ap);
        this.useChanHopping = useChanHopping;
        this.useZeroChaos = useZeroChaos;
    }

    @Override
    public String getName() {
        return "WIDS Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_WIDS;
    }

    @Override
    public boolean needsMonitorMode() {
        return true;
    }

    @Override
    public int getMaxInstances() {
        return 1;
    }

    @Override
    public void doAttack() {
        wids("wlan0", getAp().getSsid(), useChanHopping, useZeroChaos, getGuid());
    }

    @Override
    public void stopAttack() {
        super.stopAttack();
        stopwids(true);
    }

    public void widsUpdate(String updateText, int updateReason, int id) {
        updateAttackText(updateText, updateReason, id);
    }

    public void widsStats(int packetRate, int packetAmount) {


        String info = "\n" + DELIMITER + "\n";
        info += "Packets received: " + packetAmount + "\n";
        info += "Packets sent per sec: " + packetRate + "\n";
        info += DELIMITER + "\n";

        updateAttackText(info, AttackInfoString.ATTACK_UPDATE_SUCCESS, getGuid());
    }
}
