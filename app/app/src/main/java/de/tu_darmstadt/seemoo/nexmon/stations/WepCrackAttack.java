package de.tu_darmstadt.seemoo.nexmon.stations;

/**
 * Created by fabian on 9/18/16.
 */
public class WepCrackAttack extends Attack {

    private boolean useEssid;
    private boolean useDecloak;
    private boolean useKorek;
    private String fileDir;

    public native void wepcrack(String bssid, String essid, boolean useEssid, String fileDir, boolean useDecloak, boolean useKorek, int id);

    public native void stopwepcrack(boolean stopAttack);

    public WepCrackAttack(AccessPoint ap, boolean useEssid, boolean useDecloak, boolean useKorek, String fileDir) {
        super(ap);
        this.useEssid = useEssid;
        this.useDecloak = useDecloak;
        this.useKorek = useKorek;
        this.fileDir = fileDir;
    }

    @Override
    public String getName() {
        return "Aircrack WEP Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_WEP_CRACK;
    }

    @Override
    public boolean needsMonitorMode() {
        return false;
    }

    @Override
    public void stopAttack() {
        super.stopAttack();
        stopwepcrack(true);
    }

    @Override
    public int getMaxInstances() {
        return 1;
    }

    @Override
    public void doAttack() {
        wepcrack(getAp().getBssid(), getAp().getSsid(), useEssid, fileDir, useDecloak, useKorek, getGuid());
    }


    public void wepCrackUpdate(String update, int updateReason, int id) {
        updateAttackText(update, updateReason, id);
    }
}
