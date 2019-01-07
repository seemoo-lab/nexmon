package de.tu_darmstadt.seemoo.nexmon;

/**
 * Created by fabian on 1/8/17.
 */

public class DissectionStrings {

    public final static String DISS_PROTOCOLS = "frame.protocols";
    public final static String DISS_FRAME_TYPE = "wlan.fc.type";
    public final static String DISS_FRAME_SUBTYPE = "wlan.fc.type_subtype";
    public final static String DISS_FRAME_CHECK = "wlan.fcs_good";
    public final static String DISS_BSSID = "wlan.bssid";
    public final static String DISS_SIGNAL_STRENGTH = "radiotap.dbm_antsignal";
    public final static String DISS_DISTRIBUTION_SYSTEM = "wlan.fc.ds";
    public final static String DISS_TKIP = "wlan_mgt.tag.oui.type";
    public final static String DISS_CCMP = "wlan_mgt.tag.number";
    public final static String DISS_WEP = "wlan_mgt.fixed.capabilities.privacy";
    public final static String DISS_CHANNEL = "wlan_mgt.ds.current_channel";
    public final static String DISS_SSID = "wlan_mgt.ssid";
    public final static String DISS_SRC_ADDR = "wlan.sa";
    public final static String DISS_DST_ADDR = "wlan.da";
    public final static String DISS_EAPOL_ACK = "eapol.keydes.key_info.key_ack";
    public final static String DISS_EAPOL_MIC = "eapol.keydes.key_info.key_mic";
    public final static String DISS_EAPOL_INSTALL = "eapol.keydes.key_info.install";
    public final static String DISS_EAPOL_LENGTH = "eapol.keydes.datalen";
    public final static String DISS_DATA_LENGTH = "data.len";
    public final static String DISS_SEQ = "wlan.seq";
    public final static String DISS_RECEIVER_ADDR = "wlan.ra";
    public final static String DISS_IP_SRC = "ip.src";
    public final static String DISS_IPV6_SRC = "ipv6.src";
    public final static String DISS_IP_DST = "ip.dst";
    public final static String DISS_IPV6_DST = "ipv6.dst";
    public final static String DISS_TIME = "frame.time";
    public final static String DISS_LENGTH = "frame.len";

}
