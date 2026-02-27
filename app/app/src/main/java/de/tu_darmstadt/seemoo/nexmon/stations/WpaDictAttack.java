package de.tu_darmstadt.seemoo.nexmon.stations;

/**
 * Created by fabian on 9/30/16.
 */
public class WpaDictAttack extends Attack {

    public native void cowpatty(String essid, String dictOrHashFile, boolean isHashFile, String pcapFile, int id);
    public native void stopcowpatty(boolean stopAttack);

    private String dictOrHashFile;
    private String pcapFile;
    private boolean isHashFile;

    public WpaDictAttack(AccessPoint ap, String dictOrHashFile, boolean isHashFile, String pcapFile) {
        super(ap);
        this.dictOrHashFile = dictOrHashFile;
        this.isHashFile = isHashFile;
        this.pcapFile = pcapFile;
    }

    @Override
    public String getName() {
        return "WPA/WPA2 Dictionary Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_WPA_DICT;
    }

    @Override
    public boolean needsMonitorMode() {
        return false;
    }

    @Override
    public int getMaxInstances() {
        return 1;
    }

    @Override
    public void stopAttack() {
        super.stopAttack();
        stopcowpatty(true);
    }

    @Override
    public void doAttack() {
        cowpatty(getAp().getSsid(), dictOrHashFile, isHashFile, pcapFile, getGuid());
    }

    public void cowpattyUpdate(String updateText, int updateReason, int id) {
        updateAttackText(updateText, updateReason, id);
    }

}
