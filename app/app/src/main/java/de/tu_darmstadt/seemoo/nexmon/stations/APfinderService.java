package de.tu_darmstadt.seemoo.nexmon.stations;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.util.Log;

import com.google.android.gms.analytics.HitBuilders;
import com.google.android.gms.analytics.Tracker;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import de.tu_darmstadt.seemoo.nexmon.net.IFrameReceiver;
import de.tu_darmstadt.seemoo.nexmon.net.MonitorModeService;
import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;
import de.tu_darmstadt.seemoo.nexmon.sharky.PcapFileWriter;

public class APfinderService extends Service implements IFrameReceiver {


    private final IBinder mBinder = new MyBinder();
    private HashMap<String, AccessPoint> accessPoints;

    private long startTime;

    private boolean isRunning = false;

    public boolean isRunning() {
        return isRunning;
    }


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        accessPoints = new HashMap<String, AccessPoint>();
        return Service.START_NOT_STICKY;
    }

    public void start() {
        accessPoints = new HashMap<String, AccessPoint>();
        MyApplication.getFrameReceiver().getObserver().addObserver(this);
        isRunning = true;
        sendMonitorModeBroadcast(true);
        startTime = System.currentTimeMillis();
    }

    private void sendMonitorModeBroadcast(boolean monitorModeNeeded) {
        Intent intent = new Intent(MonitorModeService.INTENT_RECEIVER);
        intent.putExtra(MonitorModeService.MONITOR_MODE_ID, MonitorModeService.MONITOR_MODE_AIRODUMP);
        intent.putExtra(MonitorModeService.MONITOR_MODE_NEED, monitorModeNeeded);

        MyApplication.getAppContext().sendBroadcast(intent);
    }

    public static boolean isBroadcastAddress(String macAddress) {
        String temp = (macAddress.split(":"))[0];
        int msOctet = MyApplication.hex2decimal(temp);
        msOctet = msOctet % 2;

        return msOctet != 0;
    }


    public void toggle() {
        if(isRunning)
            stop();
        else
            start();
    }

    public void stop() {
        MyApplication.getFrameReceiver().getObserver().removeObserver(this);
        isRunning = false;
        sendMonitorModeBroadcast(false);

        Tracker tracker = MyApplication.getDefaultTracker();

        tracker.send(new HitBuilders.TimingBuilder()
                .setCategory("Runtime")
                .setVariable("Airodump")
                .setValue(System.currentTimeMillis() - startTime)
                .build());

    }

    public AccessPoint getAccessPoint(String bssid) {
        return accessPoints.get(bssid);
    }

    public HashMap<String, AccessPoint> getAccessPoints() {
        return accessPoints;
    }

    @Override
    public void packetReceived(Packet packet) {

        String frameType = null;
        String frameSubType = null;
        String good = null;
        String bssid = null;
        boolean fromDS, toDS;

        while(!Packet.blocked.compareAndSet(false, true))
            try {
                Thread.sleep(5);
            } catch(Exception e) {e.printStackTrace();}

        try {

            if(packet.getField("frame.protocols").contains("eapol")) {
                Log.d("Handshake Debugger", "EAPOL frame received.");
                eapolPacket(packet);
            } else {

                frameType = packet.getField("wlan.fc.type");
                frameSubType = packet.getField("wlan.fc.type_subtype");
                good = packet.getField("wlan.fcs_good");
                bssid = packet.getField("wlan.bssid");

                String signalStrength = packet.getField("radiotap.dbm_antsignal");

                String dsValue = packet.getField("wlan.fc.ds");

                if (dsValue.equals("0x01")) {
                    toDS = true;
                    fromDS = false;
                } else if (dsValue.equals("0x02")) {
                    toDS = false;
                    fromDS = true;
                } else if (dsValue.equals("0x03")) {
                    toDS = true;
                    fromDS = true;
                } else {
                    toDS = false;
                    fromDS = false;
                }

                if (frameSubType != null && frameSubType.equals("0x08") && bssid != null && !bssid.equals("") && !bssid.equals("ff:ff:ff:ff:ff:ff")) {

                    try {

                        String fields[] = packet.getField("wlan_mgt.tag.oui.type").split(",");
                        String tags[] = packet.getField("wlan_mgt.tag.number").split(",");

                        String enc = "";
                        for (String field : fields) {
                            field = field.trim();
                            if (field.equals("1") && !enc.contains("WPA "))
                                enc += "WPA ";
                            else if (field.equals("4") && !enc.contains("WPS "))
                                enc += "WPS ";
                        }

                        for (String tag : tags) {
                            tag = tag.trim();
                            if (tag.equals("48") && !enc.contains("WPA2 ")) {
                                enc += "WPA2 ";
                                break;
                            }
                        }

                        if (!enc.contains("WPA")) {
                            if (packet.getField("wlan_mgt.fixed.capabilities.privacy").equals("1"))
                                enc += "WEP ";
                            else
                                enc += "OPEN ";
                        }

                        int channel = Integer.parseInt(packet.getField("wlan_mgt.ds.current_channel"));

                        String ssid = packet.getField("wlan_mgt.ssid");
                        long lastSeen = System.currentTimeMillis();

                        AccessPoint ap;
                        if (accessPoints.containsKey(bssid))
                            ap = accessPoints.get(bssid);
                        else {
                            ap = new AccessPoint(bssid);
                            ap.setBeacon(packet);
                        }
                        ap.setChannel(channel);
                        ap.setSsid(ssid);
                        ap.setLastSeen(lastSeen);
                        ap.setBeacons(ap.getBeacons() + 1);
                        ap.setEncryption(enc);
                        ap.setSignalStrength(signalStrength);
                        accessPoints.put(bssid, ap);

                        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.NEW_AP");
                        intent.putExtra("bssid", ap.getBssid());
                        sendBroadcast(intent);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }


                if (accessPoints.containsKey(bssid) && (toDS ^ fromDS)) {

                    String stationMac;
                    if (toDS && !fromDS)
                        stationMac = packet.getField("wlan.sa");
                    else
                        stationMac = packet.getField("wlan.da");

                    if (stationMac != null && !isBroadcastAddress(stationMac)) {
                        AccessPoint ap = accessPoints.get(bssid);
                        ap.setSignalStrength(signalStrength);

                        Station station;
                        if (ap.getStations().containsKey(stationMac))
                            station = ap.getStations().get(stationMac);
                        else {
                            station = new Station();
                            station.macAddress = stationMac;
                        }

                        station.dataFrames += 1;
                        station.lastSeen = System.currentTimeMillis();
                        ap.getStations().put(station.macAddress, station);

                        Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.NEW_AP");
                        intent.putExtra("bssid", ap.getBssid());
                        sendBroadcast(intent);

                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            packet.cleanDissection();
            Packet.blocked.set(false);
        }

    }

    private void eapolPacket(Packet packet) {
       /* Hashtable<String, ProtoNode> allFields = packet.getAllFields();
        Iterator<String> it = allFields.keySet().iterator();
        while(it.hasNext())
            Log.d("Handshake Debugger", it.next()); */
        int msgNumber = 0;
       // boolean secSet = packet.getField("hsPackets.keydes.key_info.secure").trim().equals("1");
        boolean ackSet = packet.getField("eapol.keydes.key_info.key_ack").trim().equals("1");
        boolean micSet = packet.getField("eapol.keydes.key_info.key_mic").trim().equals("1");
        boolean installSet = packet.getField("eapol.keydes.key_info.install").trim().equals("1");

        String bssid = packet.getField("wlan.bssid").trim();
        String station;
        if(ackSet && micSet && installSet) {
            msgNumber = 3;
        } else if(ackSet) {
            msgNumber = 1;
        } else if(micSet) {
            int dataLength = Integer.valueOf(packet.getField("eapol.keydes.datalen").trim());
            if(dataLength > 0) {
                msgNumber = 2;
            } else {
                msgNumber = 4;
            }
        }

        if(msgNumber == 1 || msgNumber == 3) {
            station = packet.getField("wlan.da").trim();
        } else {
            station = packet.getField("wlan.sa").trim();
        }


        if(msgNumber > 0 && accessPoints.containsKey(bssid)) {
            HashMap<String, Handshake> apHandshakes = accessPoints.get(bssid).getHandshakes();

            Handshake handshake;
            // If no previous handshake message is captured...
            if(!apHandshakes.containsKey(station)) {
                handshake = new Handshake();
                handshake.stationMac = station;
                apHandshakes.put(station, handshake);
            } else
                handshake = apHandshakes.get(station);

            handshake.addHandshakeMessage(msgNumber, packet);

            Intent intent = new Intent("de.tu_darmstadt.seemoo.nexmon.NEW_AP");
            intent.putExtra("bssid", bssid);
            sendBroadcast(intent);
        }
    }

    public ArrayList<Packet> getAllBeacons() {
        ArrayList<Packet> beacons = new ArrayList<>();
        for(AccessPoint ap : accessPoints.values()) {
            beacons.add(ap.getBeacon());
        }

        return beacons;
    }

    private List<Handshake> getAllHandshakes() {
        List<Handshake> handshakes = new ArrayList<>();

        for(AccessPoint ap : accessPoints.values()) {
            for(Handshake hs : ap.getHandshakes().values()) {
                handshakes.add(hs);
            }
        }

        return handshakes;
    }

    public ArrayList<Packet> getAllHandshakePackets() {
        ArrayList<Packet> hsPackets = new ArrayList<>();

        for(Handshake hs : getAllHandshakes()) {
            if(!hs.isComplete)
                continue;

            for(int i = 1; i <= 4; i++) {
                if(hs.hsPackets.containsKey(i)) {
                    hsPackets.add(hs.hsPackets.get(i));
                } else {
                    continue;
                }
            }
        }

        return hsPackets;
    }

    public void writeHandshakesToFile(String filename) {
        PcapFileWriter hsPcapWriter = new PcapFileWriter(filename);

        for(Handshake hs : getAllHandshakes()) {
            if(!hs.isComplete)
                continue;

            for(int i = 1; i <= 4; i++) {
                if(hs.hsPackets.containsKey(i)) {
                    hsPcapWriter.writePacket(hs.hsPackets.get(i));
                } else {
                    continue;
                }
            }
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }


    public class MyBinder extends Binder {
        public APfinderService getService() {
            return APfinderService.this;
        }
    }



}
