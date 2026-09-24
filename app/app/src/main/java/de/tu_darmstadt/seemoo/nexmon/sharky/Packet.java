/*
 * Nexmon PenTestSuite
 * Copyright (C) 2016 Fabian Knapp
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package de.tu_darmstadt.seemoo.nexmon.sharky;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.Nullable;
import android.util.Log;

import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.concurrent.atomic.AtomicBoolean;

import de.tu_darmstadt.seemoo.nexmon.gui.MyActivity;

// To store raw packet information.  Can also piggyback a dissection pointer.
public class Packet implements Parcelable {
    public static final Parcelable.Creator<Packet> CREATOR
            = new Parcelable.Creator<Packet>() {
        public Packet createFromParcel(Parcel in) {
            return new Packet(in);
        }

        public Packet[] newArray(int size) {
            return new Packet[size];
        }
    };
    public int _number;
    public LinkType _encap;
    public int _headerLen;
    public int _dataLen;
    public byte[] _rawHeader;
    public byte[] _rawData;
    public int _lqi;  // only for ZigBee
    public int _band;  // only for ZigBee also since there is no radiotap header
    public int _dissection_ptr;
    private Hashtable<String, ProtoNode> table = null;
    public static AtomicBoolean blocked = new AtomicBoolean(false);

    public enum LinkType {
        IEEE_802_11(20, 105),
        IEEE_802_11_WLAN_RADIOTAP(23, 127),
        IEEE_ETHERNET(1, 1);

        private int wtap;
        private int pcap;

        private LinkType(int wtap, int pcap) {
            this.wtap = wtap;
            this.pcap = pcap;
        }

        public int getWtapLinktype() {
            return wtap;
        }

        public int getPcapLinktype() {
            return pcap;
        }

        @Nullable
        public static LinkType getLinktypeFromPcapValue(int pcapLinktype) {
            for (LinkType l : LinkType.values()) {
                if (l.getPcapLinktype() == pcapLinktype) {
                    return l;
                }
            }

            return null;
        }

        @Nullable
        public static LinkType getLinktypeFromWtapValue(int wtapLinktype) {
            for (LinkType l : LinkType.values()) {
                if (l.getWtapLinktype() == wtapLinktype) {
                    return l;
                }
            }

            return null;
        }
    }

//    public final static int WTAP_ENCAP_IEEE_802_11 = 20;
//    public final static int WTAP_ENCAP_IEEE_802_11_WLAN_RADIOTAP = 23;
//    public final static int WTAP_ENCAP_ETHERNET = 1;
//    public final static int PCAP_LINKTYPE_IEEE802_11 = 105;
//    public final static int PCAP_LINKTYPE_RADIOTAP = 127;
//    public final static int PCAP_LINKTYPE_ETHERNET = 1;


    public Packet(LinkType encap) {
        _rawHeader = null;
        _rawData = null;
        _encap = encap;
        _lqi = -1;
        _dissection_ptr = -1;
        _band = -1;
    }

    private Packet(Parcel in) {
        _encap = LinkType.getLinktypeFromWtapValue(in.readInt());
        _headerLen = in.readInt();
        _dataLen = in.readInt();

        _rawHeader = new byte[_headerLen];
        _rawData = new byte[_dataLen];

        in.readByteArray(_rawHeader);
        in.readByteArray(_rawData);
        _lqi = in.readInt();
        _band = in.readInt();
        _dissection_ptr = in.readInt();
    }

    public int describeContents() {
        return this.hashCode();
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(_encap.getWtapLinktype());
        out.writeInt(_headerLen);
        out.writeInt(_dataLen);
        out.writeByteArray(_rawHeader);
        out.writeByteArray(_rawData);
        out.writeInt(_lqi);
        out.writeInt(_band);
        out.writeInt(-1);    // Cannot pass the dissection pointer, otherwise the GC will
        // try to free the object more than once.  Once per copied object.
    }

    // Dissects the packet, wireshark-style.  Saves the pointer to
    // the dissection for the ability to pull fields.
    public synchronized boolean dissect() {

        if (_rawHeader == null || _rawData == null)
            return false;  // can't dissect something without the data

        if (_dissection_ptr != -1)  // packet is already dissected
            return true;

        //Log.d("PKT", _rawHeader + " " + _rawData + " " + _encap);
        _dissection_ptr = dissectPacket(_rawHeader, _rawData, _encap.getWtapLinktype());


        if (_dissection_ptr == -1)
            return false;

        return true;
    }

    public boolean isBeacon() {
        int rtap = getRtapLength();
        return (_rawData.length > rtap && _rawData[rtap] == -128);
    }

    public String getSSIDfromBeacon() {
        int rtap = getRtapLength();
        if(_rawData.length >= (rtap + 38)) {
            int ssidLength = _rawData[rtap + 37];
            byte[] ssidByte = new byte[ssidLength];

            for (int i = rtap + 38; i < (rtap + 38 + ssidLength); i++) {
                ssidByte[i - (rtap + 38)] = _rawData[i];
            }

            return new String(ssidByte);
        } else
            return "";

    }

    public String getBSSIDfromBeacon() {
        int rtap = getRtapLength();
        if(_rawData.length >= (rtap + 22)) {
            byte[] bssidByte = new byte[6];
            for (int i = rtap + 16; i <= (rtap + 21); i++) {
                bssidByte[i - (rtap + 16)] = _rawData[i];
            }

            String bssidString = MyActivity.bytesToHex(bssidByte);
            bssidString = bssidString.trim();
            bssidString = bssidString.replaceAll(" ", ":");

            return bssidString;
        } else
            return "";
    }

    public int getRtapLength() {
        int len = ((_rawData[3] & 0xff) << 8) | (_rawData[2] & 0xff);
        return len;
    }

    // Attempt to pull a field from the dissection
    public synchronized String getField(String f) {
        if(!blocked.get())
            throw new RuntimeException("Forgot to set blocked in class Packet during dissection!");
        String result;

        if (_dissection_ptr == -1)
            if (!dissect())
                return null;


        result = wiresharkGet(_dissection_ptr, f);

        return result;
    }

    public synchronized void cleanDissection() {
        if (_dissection_ptr != -1)
            dissectCleanup(_dissection_ptr);
        _dissection_ptr = -1;
    }

	/*
    // On garbage collection (this raw data is no longer used), make sure to
	// cleanup the dissection memory.
	protected void finalize() throws Throwable {
	    try {
	    	
	    	if(_dissection_ptr!=-1)
	    		dissectCleanup(_dissection_ptr);

	    } finally {
	        super.finalize();
	    }
	}*/


    public ArrayList<ProtoNode> getParents() {

        ArrayList<ProtoNode> result = new ArrayList<ProtoNode>();

        if (table == null)
            table = getAllFields();


        Enumeration<ProtoNode> protoNodes = table.elements();
        ProtoNode tmp;
        while (protoNodes.hasMoreElements()) {
            tmp = protoNodes.nextElement();
            if (tmp.getParent().equals("null"))
                result.add(tmp);
        }

        return result;
    }

    public ArrayList<ProtoNode> getChilds(String key) {

        ArrayList<ProtoNode> result = new ArrayList<ProtoNode>();

        if (table == null)
            table = getAllFields();

        Enumeration<ProtoNode> protoNodes = table.elements();
        ProtoNode tmp;
        while (protoNodes.hasMoreElements()) {
            tmp = protoNodes.nextElement();
            if (tmp.getParent().equals(key))
                result.add(tmp);
        }

        return result;
    }


    public synchronized Hashtable<String, ProtoNode> getAllFields() {
            if (!dissect())  // will only dissect if needed
                return null;

            String last_tag = "";

            // First dissect the entire packet, getting all fields

            String fields[] = wiresharkGetAll(_dissection_ptr);
            Hashtable<String, ProtoNode> pkt_fields;
            pkt_fields = new Hashtable<String, ProtoNode>();

            for (int i = 0; i < fields.length; i++) {
                String spl[] = fields[i].split("---", 5); // spl[0]: desc, spl[1]: value

                if (pkt_fields.containsKey(spl[0])) {
                    ProtoNode l = pkt_fields.get(spl[0]);
                    l.addValue(spl[4]);
                } else {
                    ProtoNode node = new ProtoNode(spl[0], i);
                    node.setParent(spl[1]);

                    if (spl[2].trim().equals("(null)"))
                        node.setDescription(spl[3]);
                    else {
                        spl[2] = spl[2].replaceFirst("Frame 0", "Frame " + _number);
                        node.setDescription(spl[2]);
                    }


                    node.addValue(spl[4]);
                    pkt_fields.put(spl[0], node);
                }
            }

            return pkt_fields;

    }

    public void nativeCrashed() {
        Log.e("Packet", "crashed");
        Log.d("Packet", "(JNIDEBUG) In nativeCrashed(): " + Integer.toString(_dissection_ptr) + ", writing crashed packet to /sdcard/crash.pcap");
        try {
            DataOutputStream os = new DataOutputStream(new FileOutputStream("/sdcard/crash.pcap"));
            byte pcap_header[] = {(byte) 0xd4, (byte) 0xc3, (byte) 0xb2, (byte) 0xa1,        // magic number
                    (byte) 0x02, (byte) 0x00, (byte) 0x04, (byte) 0x00,    // version numbers
                    (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,    // thiszone
                    (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,    // sigfigs
                    (byte) 0xff, (byte) 0xff, (byte) 0x00, (byte) 0x00,    // snaplen
                    (byte) 0x7f, (byte) 0x00, (byte) 0x00, (byte) 0x00};    // wifi

            os.write(pcap_header);
            os.write(_rawHeader);
            os.write(_rawData);
            os.close();
            Log.d("Packet", "Finished writing crashed packet");

        } catch (Exception e) {
            Log.e("nativeCrashed", "Exception trying to write crashed packet", e);
        }
        new RuntimeException("gcrashed here (native trace should follow after the Java trace)").printStackTrace();
    }


    // TODO: instead, create a class where all of the wireshark functions are static
    public native static synchronized int dissectPacket(byte[] header, byte[] data, int encap);

    public native static synchronized void dissectCleanup(int dissect_ptr);

    public native static synchronized String wiresharkGet(int dissect_ptr, String param);

    public native static synchronized String[] wiresharkGetAll(int dissect_ptr);

    public native void decrypt(String bssid, String essid, String passphrase, String fileDir, int encryption);

}
