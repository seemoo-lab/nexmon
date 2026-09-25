import sys
import time
import subprocess
from scapy.all import (
    RadioTap, Dot11, Dot11ProbeReq, Dot11Beacon, Dot11Elt, 
    sendp, conf
)

conf.sniff_promisc = 0
iface = "mon0"
count_per_type = 20

def get_test_frames(channel_num):
    # 1. Management: Probe Request
    probe_req = (
        RadioTap() /
        Dot11(type=0, subtype=4, addr1="ff:ff:ff:ff:ff:ff", addr2="00:11:22:33:44:55", addr3="ff:ff:ff:ff:ff:ff") /
        Dot11ProbeReq() /
        Dot11Elt(ID="SSID", info="NexmonMultiTest")
    )

    # 2. Management: Beacon Frame
    beacon = (
        RadioTap() /
        Dot11(type=0, subtype=8, addr1="ff:ff:ff:ff:ff:ff", addr2="02:22:33:44:55:66", addr3="02:22:33:44:55:66") /
        Dot11Beacon(cap="ESS+privacy") /
        Dot11Elt(ID="SSID", info="NexmonMultiBeacon") /
        Dot11Elt(ID="Rates", info=b"\x82\x84\x8b\x96\x0c\x12\x18\x24") /
        Dot11Elt(ID="DSset", info=bytes([channel_num]))
    )

    # 3. Data: Null Data Frame (type=2, subtype=4)
    null_data = (
        RadioTap() /
        Dot11(type=2, subtype=4, addr1="02:00:00:00:00:01", addr2="00:11:22:33:44:55", addr3="02:00:00:00:00:01")
    )

    # 4. Data: QoS Null Data Frame (type=2, subtype=12)
    qos_null = (
        RadioTap() /
        Dot11(type=2, subtype=12, addr1="02:00:00:00:00:01", addr2="00:11:22:33:44:55", addr3="02:00:00:00:00:01")
    )

    # 5. Control: RTS (Request to Send: type=1, subtype=11)
    rts = (
        RadioTap() /
        Dot11(type=1, subtype=11, addr1="02:00:00:00:00:01", addr2="00:11:22:33:44:55")
    )

    # 6. Control: CTS (Clear to Send: type=1, subtype=12)
    cts = (
        RadioTap() /
        Dot11(type=1, subtype=12, addr1="00:11:22:33:44:55")
    )

    return [
        ("Probe Request (Mgmt)", probe_req),
        ("Beacon (Mgmt)", beacon),
        ("Null Data (Data)", null_data),
        ("QoS Null Data (Data)", qos_null),
        ("RTS (Control)", rts),
        ("CTS (Control)", cts),
    ]

def run_test_on_channel(ch_str, ch_num):
    print(f"\n=======================================================")
    print(f"[*] Testing Multi-Frame Injection on Channel {ch_str}")
    print(f"=======================================================")

    subprocess.run(["nexutil", "-k", ch_str], check=True)
    time.sleep(0.5)

    frames = get_test_frames(ch_num)
    results = []

    for name, pkt in frames:
        t0 = time.time()
        try:
            sendp(pkt, iface=iface, count=count_per_type, inter=0.01, verbose=False)
            dt = time.time() - t0
            rate = count_per_type / dt
            print(f"  [+] {name:<22}: Injected {count_per_type:2d} pkts in {dt:.3f}s ({rate:5.1f} pkt/s)")
            results.append((name, True, dt, rate))
        except Exception as e:
            print(f"  [-] {name:<22}: FAILED ({e})")
            results.append((name, False, 0, 0))

    return results

if __name__ == "__main__":
    res_24 = run_test_on_channel("6", 6)
    res_5g = run_test_on_channel("36", 36)

    print("\n=======================================================")
    print("[*] SUMMARY OF MULTI-FRAME INJECTION")
    print("=======================================================")
    all_ok = all(r[1] for r in res_24 + res_5g)
    if all_ok:
        print(f"[+] All 6 802.11 frame categories injected successfully on both 2.4 GHz & 5 GHz!")
    else:
        print(f"[-] Some frame injections failed.")

