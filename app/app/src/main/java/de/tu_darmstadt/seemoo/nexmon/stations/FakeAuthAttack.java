package de.tu_darmstadt.seemoo.nexmon.stations;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;

/**
 * Created by fabian on 9/1/16.
 */
public class FakeAuthAttack extends Attack {

    private int reassocTime;
    private int keepaliveTime;
    private int packetTime;
    private boolean useCustomStationMac;
    private String stationMac;

    public native void fakeauth(String bssid, String essid, boolean useCustomStationMac, String stationMac, String interfaceName, int reassocTiming, int keepaliveTiming, int packetTiming, int id);
    public native void stopfakeauth(boolean running);

    public FakeAuthAttack(AccessPoint ap, int reassocTime, int keepaliveTime, int packetTime, boolean useCustomStationMac, String stationMac) {
        super(ap);
        this.reassocTime = reassocTime;
        this.keepaliveTime = keepaliveTime;
        this.packetTime = packetTime;
        this.useCustomStationMac = useCustomStationMac;
        this.stationMac = stationMac;
    }

    @Override
    public String getName() {
        return "FakeAuth Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_FAKE_AUTH;
    }

    @Override
    public boolean needsMonitorMode() {
        return true;
    }

    @Override
    public void stopAttack() {
        super.stopAttack();
        stopfakeauth(true);
    }

    @Override
    public void doAttack() {
        fakeauth(getAp().getBssid(), getAp().getSsid(), useCustomStationMac, stationMac, MyApplication.WLAN_INTERFACE, reassocTime, keepaliveTime, packetTime, getGuid());
    }


    public void fakeauthUpdate(String updateText, int updateReason, int id) {

        updateAttackText(updateText, updateReason, id);


    }

    @Override
    public int getMaxInstances() {
        return 1;
    }
}
