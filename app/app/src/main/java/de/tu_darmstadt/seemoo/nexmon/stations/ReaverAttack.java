package de.tu_darmstadt.seemoo.nexmon.stations;

/**
 * Created by fabian on 9/29/16.
 */
public class ReaverAttack extends Attack {

    public native void reaver(String interfaceName, String bssid, String essid, boolean useAutoSettings, boolean disableChanHopping, boolean usePixieDust, int id);
    public native void stopreaver(boolean stopAttack);

    private boolean usePixieDust;

    public ReaverAttack(AccessPoint ap, boolean usePixieDust) {
        super(ap);
        this.usePixieDust = usePixieDust;
    }

    @Override
    public String getName() {
        return "Reaver WPS Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_REAVER;
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
        reaver("wlan0", getAp().getBssid(), getAp().getSsid(), true, true, usePixieDust, getGuid());
    }

    public void reaverUpdate(String updateText, int updateReason, int id) {
        updateAttackText(updateText, updateReason, id);
    }

    @Override
    public void stopAttack() {
        super.stopAttack();
        stopreaver(true);
    }
}
