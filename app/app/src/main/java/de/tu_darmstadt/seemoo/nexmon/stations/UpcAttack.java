package de.tu_darmstadt.seemoo.nexmon.stations;

/**
 * Created by fabian on 9/26/16.
 */
public class UpcAttack extends Attack {

    public native void upc(String essid, String prefixes, int id);

    public UpcAttack(AccessPoint ap) {
        super(ap);
    }

    @Override
    public String getName() {
        return "Unity Media Router Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_UPC;
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
    public void doAttack() {
        upc(getAp().getSsid(), "SAAP,SAPP,SBAP", getGuid());
    }

    public void upcUpdate(String update, int updateReason, int id) {
        updateAttackText(update, updateReason, id);
    }
}
