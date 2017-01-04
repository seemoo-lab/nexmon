package de.tu_darmstadt.seemoo.nexmon.stations;

import java.util.HashMap;

import de.tu_darmstadt.seemoo.nexmon.sharky.Packet;

public class AccessPoint {

    private String bssid = "";
    private String ssid = "";
    private long lastSeen;
    private int beacons = 0;
    private HashMap<String, Station> stations = new HashMap<String, Station>();
    private int channel;
    private String encryption;
    private int cipher;
    private int auth;
    private String signalStrength;
    private HashMap<String, Handshake> handshakes = new HashMap<>();
    private Packet beacon;

    public AccessPoint(String bssid) {
        this.bssid = bssid;
    }

    public HashMap<String, Station> getStations() {
        return stations;
    }

    public void setStations(HashMap<String, Station> stations) {
        this.stations = stations;
    }

    public String getBssid() {
        return bssid;
    }

    public void setBssid(String bssid) {
        this.bssid = bssid;
    }

    public String getSsid() {
        return ssid;
    }

    public void setSsid(String ssid) {
        this.ssid = ssid;
        if(this.ssid == null)
            this.ssid = "";
    }

    public long getLastSeen() {
        return lastSeen;
    }

    public void setLastSeen(long lastSeen) {
        this.lastSeen = lastSeen;
    }

    public int getBeacons() {
        return beacons;
    }

    public void setBeacons(int beacons) {
        this.beacons = beacons;
    }

    public int getChannel() {
        return channel;
    }

    public void setChannel(int channel) {
        this.channel = channel;
    }

    public String getEncryption() {
        return encryption;
    }

    public void setEncryption(String encryption) {
        this.encryption = encryption;
    }

    public int getCipher() {
        return cipher;
    }

    public void setCipher(int cipher) {
        this.cipher = cipher;
    }

    public int getAuth() {
        return auth;
    }

    public void setAuth(int auth) {
        this.auth = auth;
    }

    public String getSignalStrength() {
        return signalStrength;
    }

    public void setSignalStrength(String signalStrength) {
        this.signalStrength = signalStrength;
    }

    public HashMap<String, Handshake> getHandshakes() {
        return handshakes;
    }

    public void setBeacon(Packet beacon) {
        this.beacon = beacon;
    }

    public Packet getBeacon() {
        return beacon;
    }
}
