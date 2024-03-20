/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * License:                                                                *
 *                                                                         *
 * Copyright (c) 2017 Secure Mobile Networking Lab (SEEMOO)                *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * 1. The above copyright notice and this permission notice shall be       *
 *    include in all copies or substantial portions of the Software.       *
 *                                                                         *
 * 2. Any use of the Software which results in an academic publication or  *
 *    other publication which includes a bibliography must include         *
 *    citations to the nexmon project a) and the paper cited under b):     *
 *                                                                         *
 *    a) "Matthias Schulz, Daniel Wegemer and Matthias Hollick. Nexmon:    *
 *        The C-based Firmware Patching Framework. https://nexmon.org"     *
 *                                                                         *
 *    b) "Matthias Schulz, Francesco Gringoli, Daniel Steinmetzer, Michael *
 *        Koch and Matthias Hollick. Massive Reactive Smartphone-Based     *
 *        Jamming using Arbitrary Waveforms and Adaptive Power Control.    *
 *        Proceedings of the 10th ACM Conference on Security and Privacy   *
 *        in Wireless and Mobile Networks (WiSec 2017), July 2017."        *
 *                                                                         *
 * 3. The Software is not used by, in cooperation with, or on behalf of    *
 *    any armed forces, intelligence agencies, reconnaissance agencies,    *
 *    defense agencies, offense agencies or any supplier, contractor, or   *
 *    research associated.                                                 *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 *                                                                         *
 **************************************************************************/

package de.tu_darmstadt.seemoo.nexmon.utils;

import android.content.Context;
import android.util.Base64;
import android.util.Log;
import android.widget.Toast;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

import de.tu_darmstadt.seemoo.nexmon.MyApplication;
import eu.chainfire.libsuperuser.Shell;

/**
 * Created by matthias on 24.05.17.
 */

public class Nexutil {
    protected static Nexutil instance;
    private static String nexutilPath;

    private StringBuilder args = new StringBuilder();

    public Nexutil() {
        this.instance = this;
        if (nexutilPath == null) {
            nexutilPath = Assets.getAssetsPath(MyApplication.getAppContext(), "nexutil");
        }
    }

    public Nexutil get(int cmd) {
        args.append(" -g" + cmd);
        return this;
    }

    public Nexutil value(byte[] value, int extraLength) {
        String valueEncoded = Base64.encodeToString(value, Base64.NO_WRAP);
        args.append(" -b -l" + (value.length + extraLength) + " -v\"" + valueEncoded + "\"");
        return this;
    }

    public Nexutil value(byte[] value) {
        return value(value, 0);
    }

    public Nexutil value(int value) {
        args.append(" -i -l4 -v" + value);
        return this;
    }

    public Nexutil value(String value, int extraLength) {
        byte[] valueBytes = value.getBytes();
        byte[] valueBytesTerminated = new byte[valueBytes.length + 1];
        System.arraycopy(valueBytes, 0, valueBytesTerminated, 0, valueBytes.length);
        return value(valueBytesTerminated, extraLength);
    }

    public Nexutil value(String value) {
        return value(value, 0);
    }

    public Nexutil value(String value, String value2) {
        byte[] valueBytes = value.getBytes();
        byte[] valueBytes2 = value2.getBytes();
        byte[] bytesTerminated = new byte[valueBytes.length + 1 + valueBytes2.length + 1];
        System.arraycopy(valueBytes, 0, bytesTerminated, 0, valueBytes.length);
        System.arraycopy(valueBytes2, 0, bytesTerminated, valueBytes.length + 1, valueBytes2.length);
        return value(bytesTerminated);
    }

    public Nexutil set(int cmd) {
        args.append(" -s");
        return this;
    }

    public Nexutil length(int length) {
        args.append(" -l");
        return this;
    }

    public int executeInt() {
        byte[] decoded = executeBytes();
        ByteBuffer decodedBuf = ByteBuffer.wrap(decoded);
        decodedBuf.order(ByteOrder.LITTLE_ENDIAN);
        return decodedBuf.getInt();
    }

    public String executeString() {
        List<String> out = Shell.SU.run(nexutilPath + args.toString() + " -r | strings");
        return out.toString();
    }

    public byte[] executeBytes() {
        List<String> out = Shell.SU.run(nexutilPath + " -R" + args.toString());
        byte[] decoded = Base64.decode(out.toString(), Base64.DEFAULT);
        return decoded;
    }

    public void execute() {
        Shell.SU.run(nexutilPath + args.toString());
    }

    public String getIovar(String name, int extraLength) {
        return get(WLC_GET_VAR).value(name, extraLength).executeString();
    }

    public void setIovar(String name, String value) {
        set(WLC_SET_VAR).value(name, value).execute();
    }

    public static Nexutil getInstance() {
        return instance == null ? new Nexutil() : instance;
    }

    public String getIoctl(int cmd, int length) {
        List<String> out = Shell.SU.run(nexutilPath + " -l" + length + " -g" + cmd);
        return out.toString();
    }

    public String getStringIoctl(int cmd, int length) {
        List<String> out = Shell.SU.run(nexutilPath + " -r -l" + length + " -g" + cmd + " | strings");
        return out.toString();
    }

    public String getIoctl(int cmd) {
        List<String> out = Shell.SU.run(nexutilPath + " -g" + cmd);
        return out.toString();
    }

    public int getIntIoctl(int cmd) {
        List<String> out = Shell.SU.run(nexutilPath + " -R -g" + cmd);
        byte[] decoded = Base64.decode(out.toString(), Base64.DEFAULT);
        ByteBuffer decodedBuf = ByteBuffer.wrap(decoded);
        decodedBuf.order(ByteOrder.LITTLE_ENDIAN);
        return decodedBuf.getInt();
    }

    public String setIoctl(int cmd) {
        List<String> out = Shell.SU.run(nexutilPath + " -s" + cmd);
        return out.toString();
    }

    public String setIoctl(int cmd, int value) {
        List<String> out = Shell.SU.run(nexutilPath + " -s" + cmd + " -l4 -i -v" + value);
        return out.toString();
    }

    public String setIoctl(int cmd, String value) {
        byte[] valueBytes = value.getBytes();
        byte[] valueBytesTerminated = new byte[valueBytes.length + 1];
        System.arraycopy(valueBytes, 0, valueBytesTerminated, 0, valueBytes.length);
        return setIoctl(cmd, valueBytesTerminated);
    }

    public String setIoctl(int cmd, byte buf[]) {
        String value = Base64.encodeToString(buf, Base64.NO_WRAP);
        List<String> out = Shell.SU.run(nexutilPath + " -s" + cmd + " -b -l" + buf.length + " -v\"" + value + "\"");
        return out.toString();
    }

    public final static int NEX_GET_CAPABILITIES = 400;
    public final static int NEX_WRITE_TO_CONSOLE = 401;
    public final static int NEX_CT_EXPERIMENTS = 402;
    public final static int NEX_GET_CONSOLE = 403;
    public final static int NEX_GET_PHYREG = 404;
    public final static int NEX_SET_PHYREG = 405;
    public final static int NEX_READ_OBJMEM = 406;
    public final static int NEX_WRITE_OBJMEM = 407;
    public final static int NEX_INJECT_FRAME = 408;
    public final static int NEX_PRINT_TIMERS = 409;
    public final static int NEX_GET_SECURITYCOOKIE = 410;
    public final static int NEX_SET_SECURITYCOOKIE = 411;
    public final static int NEX_GET_WL_CNT = 412;
    public final static int NEX_GET_VERSION_STRING = 413;
    public final static int NEX_TEST_ARGPRINTF = 414;
    public final static int NEX_GET_RSPEC_OVERRIDE = 415;
    public final static int NEX_SET_RSPEC_OVERRIDE = 416;
    public final static int NEX_CLEAR_CONSOLE = 417;
    public final static int NEX_GET_CHANSPEC_OVERRIDE = 418;
    public final static int NEX_SET_CHANSPEC_OVERRIDE = 419;
    public final static int NEX_GET_AMPDU_TX = 420;
    public final static int NEX_SET_AMPDU_TX = 421;

    public final static int WLC_GET_MAGIC = 0;
    public final static int WLC_GET_VERSION = 1;
    public final static int WLC_UP = 2;
    public final static int WLC_DOWN = 3;
    public final static int WLC_GET_LOOP = 4;
    public final static int WLC_SET_LOOP = 5;
    public final static int WLC_DUMP = 6;
    public final static int WLC_GET_MSGLEVEL = 7;
    public final static int WLC_SET_MSGLEVEL = 8;
    public final static int WLC_GET_PROMISC = 9;
    public final static int WLC_SET_PROMISC = 10;
    public final static int WLC_OVERLAY_IOCTL = 11;
    public final static int WLC_GET_RATE = 12;
    public final static int WLC_GET_MAX_RATE = 13;
    public final static int WLC_GET_INSTANCE = 14;
    public final static int WLC_GET_FRAG = 15;
    public final static int WLC_SET_FRAG = 16;
    public final static int WLC_GET_RTS = 17;
    public final static int WLC_SET_RTS = 18;
    public final static int NEX_READ_D11_OBJMEM = 15;
    public final static int WLC_GET_INFRA = 19;
    public final static int WLC_SET_INFRA = 20;
    public final static int WLC_GET_AUTH = 21;
    public final static int WLC_SET_AUTH = 22;
    public final static int WLC_GET_BSSID = 23;
    public final static int WLC_SET_BSSID = 24;
    public final static int WLC_GET_SSID = 25;
    public final static int WLC_SET_SSID = 26;
    public final static int WLC_RESTART = 27;
    public final static int WLC_TERMINATED = 28;
    public final static int WLC_GET_CHANNEL = 29;
    public final static int WLC_SET_CHANNEL = 30;
    public final static int WLC_GET_SRL = 31;
    public final static int WLC_SET_SRL = 32;
    public final static int WLC_GET_LRL = 33;
    public final static int WLC_SET_LRL = 34;
    public final static int WLC_GET_PLCPHDR = 35;
    public final static int WLC_SET_PLCPHDR = 36;
    public final static int WLC_GET_RADIO = 37;
    public final static int WLC_SET_RADIO = 38;
    public final static int WLC_GET_PHYTYPE = 39;
    public final static int WLC_DUMP_RATE = 40;
    public final static int WLC_SET_RATE_PARAMS = 41;
    public final static int WLC_GET_FIXRATE = 42;
    public final static int WLC_SET_FIXRATE = 43;
    public final static int WLC_GET_KEY = 44;
    public final static int WLC_SET_KEY = 45;
    public final static int WLC_GET_REGULATORY = 46;
    public final static int WLC_SET_REGULATORY = 47;
    public final static int WLC_GET_PASSIVE_SCAN = 48;
    public final static int WLC_SET_PASSIVE_SCAN = 49;
    public final static int WLC_SCAN = 50;
    public final static int WLC_SCAN_RESULTS = 51;
    public final static int WLC_DISASSOC = 52;
    public final static int WLC_REASSOC = 53;
    public final static int WLC_GET_ROAM_TRIGGER = 54;
    public final static int WLC_SET_ROAM_TRIGGER = 55;
    public final static int WLC_GET_ROAM_DELTA = 56;
    public final static int WLC_SET_ROAM_DELTA = 57;
    public final static int WLC_GET_ROAM_SCAN_PERIOD = 58;
    public final static int WLC_SET_ROAM_SCAN_PERIOD = 59;
    public final static int WLC_EVM = 60;
    public final static int WLC_GET_TXANT = 61;
    public final static int WLC_SET_TXANT = 62;
    public final static int WLC_GET_ANTDIV = 63;
    public final static int WLC_SET_ANTDIV = 64;
    public final static int WLC_GET_TXPWR = 65;
    public final static int WLC_SET_TXPWR = 66;
    public final static int WLC_GET_CLOSED = 67;
    public final static int WLC_SET_CLOSED = 68;
    public final static int WLC_GET_MACLIST = 69;
    public final static int WLC_SET_MACLIST = 70;
    public final static int WLC_GET_RATESET = 71;
    public final static int WLC_SET_RATESET = 72;
    public final static int WLC_GET_LOCALE = 73;
    public final static int WLC_LONGTRAIN = 74;
    public final static int WLC_GET_BCNPRD = 75;
    public final static int WLC_SET_BCNPRD = 76;
    public final static int WLC_GET_DTIMPRD = 77;
    public final static int WLC_SET_DTIMPRD = 78;
    public final static int WLC_GET_SROM = 79;
    public final static int WLC_SET_SROM = 80;
    public final static int WLC_GET_WEP_RESTRICT = 81;
    public final static int WLC_SET_WEP_RESTRICT = 82;
    public final static int WLC_GET_COUNTRY = 83;
    public final static int WLC_SET_COUNTRY = 84;
    public final static int WLC_GET_PM = 85;
    public final static int WLC_SET_PM = 86;
    public final static int WLC_GET_WAKE = 87;
    public final static int WLC_SET_WAKE = 88;
    public final static int WLC_GET_D11CNTS = 89;
    public final static int WLC_GET_FORCELINK = 90;
    public final static int WLC_SET_FORCELINK = 91;
    public final static int WLC_FREQ_ACCURACY = 92;
    public final static int WLC_CARRIER_SUPPRESS = 93;
    public final static int WLC_GET_PHYREG = 94;
    public final static int WLC_SET_PHYREG = 95;
    public final static int WLC_GET_RADIOREG = 96;
    public final static int WLC_SET_RADIOREG = 97;
    public final static int WLC_GET_REVINFO = 98;
    public final static int WLC_GET_UCANTDIV = 99;
    public final static int WLC_SET_UCANTDIV = 100;
    public final static int WLC_R_REG = 101;
    public final static int WLC_W_REG = 102;
    public final static int WLC_DIAG_LOOPBACK = 103;
    public final static int WLC_RESET_D11CNTS = 104;
    public final static int WLC_GET_MACMODE = 105;
    public final static int WLC_SET_MACMODE = 106;
    public final static int WLC_GET_MONITOR = 107;
    public final static int WLC_SET_MONITOR = 108;
    public final static int WLC_GET_GMODE = 109;
    public final static int WLC_SET_GMODE = 110;
    public final static int WLC_GET_LEGACY_ERP = 111;
    public final static int WLC_SET_LEGACY_ERP = 112;
    public final static int WLC_GET_RX_ANT = 113;
    public final static int WLC_GET_CURR_RATESET = 114;
    public final static int WLC_GET_SCANSUPPRESS = 115;
    public final static int WLC_SET_SCANSUPPRESS = 116;
    public final static int WLC_GET_AP = 117;
    public final static int WLC_SET_AP = 118;
    public final static int WLC_GET_EAP_RESTRICT = 119;
    public final static int WLC_SET_EAP_RESTRICT = 120;
    public final static int WLC_SCB_AUTHORIZE = 121;
    public final static int WLC_SCB_DEAUTHORIZE = 122;
    public final static int WLC_GET_WDSLIST = 123;
    public final static int WLC_SET_WDSLIST = 124;
    public final static int WLC_GET_ATIM = 125;
    public final static int WLC_SET_ATIM = 126;
    public final static int WLC_GET_RSSI = 127;
    public final static int WLC_GET_PHYANTDIV = 128;
    public final static int WLC_SET_PHYANTDIV = 129;
    public final static int WLC_AP_RX_ONLY = 130;
    public final static int WLC_GET_TX_PATH_PWR = 131;
    public final static int WLC_SET_TX_PATH_PWR = 132;
    public final static int WLC_GET_WSEC = 133;
    public final static int WLC_SET_WSEC = 134;
    public final static int WLC_GET_PHY_NOISE = 135;
    public final static int WLC_GET_BSS_INFO = 136;
    public final static int WLC_GET_PKTCNTS = 137;
    public final static int WLC_GET_LAZYWDS = 138;
    public final static int WLC_SET_LAZYWDS = 139;
    public final static int WLC_GET_BANDLIST = 140;
    public final static int WLC_GET_BAND = 141;
    public final static int WLC_SET_BAND = 142;
    public final static int WLC_SCB_DEAUTHENTICATE = 143;
    public final static int WLC_GET_SHORTSLOT = 144;
    public final static int WLC_GET_SHORTSLOT_OVERRIDE = 145;
    public final static int WLC_SET_SHORTSLOT_OVERRIDE = 146;
    public final static int WLC_GET_SHORTSLOT_RESTRICT = 147;
    public final static int WLC_SET_SHORTSLOT_RESTRICT = 148;
    public final static int WLC_GET_GMODE_PROTECTION = 149;
    public final static int WLC_GET_GMODE_PROTECTION_OVERRIDE = 150;
    public final static int WLC_SET_GMODE_PROTECTION_OVERRIDE = 151;
    public final static int WLC_UPGRADE = 152;
    public final static int WLC_GET_MRATE = 153;
    public final static int WLC_SET_MRATE = 154;
    public final static int WLC_GET_IGNORE_BCNS = 155;
    public final static int WLC_SET_IGNORE_BCNS = 156;
    public final static int WLC_GET_SCB_TIMEOUT = 157;
    public final static int WLC_SET_SCB_TIMEOUT = 158;
    public final static int WLC_GET_ASSOCLIST = 159;
    public final static int WLC_GET_CLK = 160;
    public final static int WLC_SET_CLK = 161;
    public final static int WLC_GET_UP = 162;
    public final static int WLC_OUT = 163;
    public final static int WLC_GET_WPA_AUTH = 164;
    public final static int WLC_SET_WPA_AUTH = 165;
    public final static int WLC_GET_UCFLAGS = 166;
    public final static int WLC_SET_UCFLAGS = 167;
    public final static int WLC_GET_PWRIDX = 168;
    public final static int WLC_SET_PWRIDX = 169;
    public final static int WLC_GET_TSSI = 170;
    public final static int WLC_GET_SUP_RATESET_OVERRIDE = 171;
    public final static int WLC_SET_SUP_RATESET_OVERRIDE = 172;
    public final static int WLC_SET_FAST_TIMER = 173;
    public final static int WLC_GET_FAST_TIMER = 174;
    public final static int WLC_SET_SLOW_TIMER = 175;
    public final static int WLC_GET_SLOW_TIMER = 176;
    public final static int WLC_DUMP_PHYREGS = 177;
    public final static int WLC_GET_PROTECTION_CONTROL = 178;
    public final static int WLC_SET_PROTECTION_CONTROL = 179;
    public final static int WLC_GET_PHYLIST = 180;
    public final static int WLC_ENCRYPT_STRENGTH = 181;
    public final static int WLC_DECRYPT_STATUS = 182;
    public final static int WLC_GET_KEY_SEQ = 183;
    public final static int WLC_GET_SCAN_CHANNEL_TIME = 184;
    public final static int WLC_SET_SCAN_CHANNEL_TIME = 185;
    public final static int WLC_GET_SCAN_UNASSOC_TIME = 186;
    public final static int WLC_SET_SCAN_UNASSOC_TIME = 187;
    public final static int WLC_GET_SCAN_HOME_TIME = 188;
    public final static int WLC_SET_SCAN_HOME_TIME = 189;
    public final static int WLC_GET_SCAN_NPROBES = 190;
    public final static int WLC_SET_SCAN_NPROBES = 191;
    public final static int WLC_GET_PRB_RESP_TIMEOUT = 192;
    public final static int WLC_SET_PRB_RESP_TIMEOUT = 193;
    public final static int WLC_GET_ATTEN = 194;
    public final static int WLC_SET_ATTEN = 195;
    public final static int WLC_GET_SHMEM = 196;
    public final static int WLC_SET_SHMEM = 197;
    public final static int WLC_GET_GMODE_PROTECTION_CTS = 198;
    public final static int WLC_SET_GMODE_PROTECTION_CTS = 199;
    public final static int WLC_SET_WSEC_TEST = 200;
    public final static int WLC_SCB_DEAUTHENTICATE_FOR_REASON = 201;
    public final static int WLC_TKIP_COUNTERMEASURES = 202;
    public final static int WLC_GET_PIOMODE = 203;
    public final static int WLC_SET_PIOMODE = 204;
    public final static int WLC_SET_ASSOC_PREFER = 205;
    public final static int WLC_GET_ASSOC_PREFER = 206;
    public final static int WLC_SET_ROAM_PREFER = 207;
    public final static int WLC_GET_ROAM_PREFER = 208;
    public final static int WLC_SET_LED = 209;
    public final static int WLC_GET_LED = 210;
    public final static int WLC_GET_INTERFERENCE_MODE = 211;
    public final static int WLC_SET_INTERFERENCE_MODE = 212;
    public final static int WLC_GET_CHANNEL_QA = 213;
    public final static int WLC_START_CHANNEL_QA = 214;
    public final static int WLC_GET_CHANNEL_SEL = 215;
    public final static int WLC_START_CHANNEL_SEL = 216;
    public final static int WLC_GET_VALID_CHANNELS = 217;
    public final static int WLC_GET_FAKEFRAG = 218;
    public final static int WLC_SET_FAKEFRAG = 219;
    public final static int WLC_GET_PWROUT_PERCENTAGE = 220;
    public final static int WLC_SET_PWROUT_PERCENTAGE = 221;
    public final static int WLC_SET_BAD_FRAME_PREEMPT = 222;
    public final static int WLC_GET_BAD_FRAME_PREEMPT = 223;
    public final static int WLC_SET_LEAP_LIST = 224;
    public final static int WLC_GET_LEAP_LIST = 225;
    public final static int WLC_GET_CWMIN = 226;
    public final static int WLC_SET_CWMIN = 227;
    public final static int WLC_GET_CWMAX = 228;
    public final static int WLC_SET_CWMAX = 229;
    public final static int WLC_GET_WET = 230;
    public final static int WLC_SET_WET = 231;
    public final static int WLC_GET_PUB = 232;
    public final static int WLC_GET_KEY_PRIMARY = 235;
    public final static int WLC_SET_KEY_PRIMARY = 236;
    public final static int WLC_GET_VAR = 262;
    public final static int WLC_SET_VAR = 263;
}
