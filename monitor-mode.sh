#!/bin/bash
# monitor-mode.sh - enable/disable nexmon monitor mode on the bcm43455c0.
#
# Implements the sequence recorded as working in
# firmwares/bcm43455c0/7_45_265/PROGRESS.md, Stage 5c:
#
#     rfkill unblock -> STA up -> read phy -> add monitor vif -> mon up ->
#     STA down -> nexutil -m2
#
# "vif before mode", per the 43430a1 lesson noted there.
#
# The one addition is the precondition check in wait_unassociated(). The rig
# that procedure was verified on had the chip NM-unmanaged with autoconnect
# disabled, so it was never associated when monitor mode was switched on. On a
# box where NetworkManager holds the chip on a profile, enabling monitor mode
# while the STA is associated has been observed to trap the dongle
# (brcmf_fw_crashed, type 0x4 data abort, ~65ms after SET_MONITOR returns OK).
# So this script disassociates first and refuses to continue until the STA is
# actually down - it will not hand a live association to SET_MONITOR.
#
# Auto-recovery: with the nexmon 7.3.y driver that carries the
# monitor-restore-after-reset fix, a control-DCMD wedge (the -110 ETIMEDOUT
# cascade -> brcmf_sdio_ctl_watchdog -> SDIO bus reset) now re-probes the chip
# AND re-creates $MON automatically - same name, new ifindex - then brings it
# back up and restores the active chanspec, so a capture survives a wedge
# without re-running this script. The driver also automatically brings $STA
# down during monitor restoration so cfg80211 immediately allows channel hopping
# without -EBUSY. Re-running '$0 on' is idempotent and re-asserts full state.
# Tearing down with '$0 off' happens with the bus up, so the driver clears its
# restore intent - a later wedge will NOT bring monitor back. Intended semantics.
#
# NOTE: this script has not been run verbatim end-to-end. The driver-level
# monitor bring-up and the wedge-reset auto-recovery above were exercised on this
# bcm43455c0 (kernel 7.3.0-rc4) on 2026-09-24 with the nexmon 7.3.y build (via
# iw/tcpdump; the nexutil -m2 path here was not re-checked in that run). Verify
# against dmesg on first use.
#
# Usage:
#   ./monitor-mode.sh [on] [channel] [country]  enable monitor mode (default)
#   ./monitor-mode.sh <channel> [country]       enable monitor mode on channel
#   ./monitor-mode.sh channel <channel>         switch channel on running monitor
#   ./monitor-mode.sh off                       tear down, hand STA back to NM
#   ./monitor-mode.sh status                    show current state
#
# Env overrides:  STA=wlan0  MON=mon0  CHAN=<channel>  COUNTRY=<cc>  NEXUTIL=<path>

set -u

STA="${STA:-wlan0}"
MON="${MON:-mon0}"
CHAN="${CHAN:-}"
COUNTRY="${COUNTRY:-}"
# QUIESCE=1 (default) stops wpa_supplicant for the monitor session. It keeps a
# p2p-dev handle on the phy and background-polls get_channel; on the
# single-outstanding BCDC/SDIO path those DCMDs collide with a fast channel
# hopper (airodump-ng) and can time the firmware out (-110). Restored on 'off'.
# Set QUIESCE=0 to leave it running. NetworkManager is only set unmanaged (never
# stopped) because it also manages the wired uplink.
QUIESCE="${QUIESCE:-1}"
STATE_FILE="${STATE_FILE:-/run/nexmon-monitor.state}"

die()  { printf '\033[0;31merror:\033[0m %s\n' "$*" >&2; exit 1; }
info() { printf '\033[0;32m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[0;33mwarn:\033[0m %s\n' "$*" >&2; }

[ "$(id -u)" -eq 0 ] || die "must run as root"

# The two safety gates below (check_not_uplink, wait_unassociated) are the only
# things standing between SET_MONITOR and a trapped dongle, and both fail OPEN
# when their tool is missing: an absent `iw` leaves the pipeline empty, grep
# finds no '^Connected', and the `!' inverts that into "unassociated". Likewise
# an absent `ip` leaves the uplink device empty and the comparison silently
# passes. Preflight so a missing tool aborts instead of arming the crash.
require_tools() {
    local t missing=""
    for t in ip iw dmesg awk grep seq; do
        command -v "$t" >/dev/null 2>&1 || missing="$missing $t"
    done
    [ -z "$missing" ] || die "missing required tool(s):$missing
       Install them (apt install iproute2 iw) and re-run. Refusing to continue:
       the association and uplink checks cannot be evaluated without them, and
       enabling monitor mode while the STA is associated traps the dongle."
}
require_tools

find_nexutil() {
    if [ -n "${NEXUTIL:-}" ]; then echo "$NEXUTIL"; return; fi
    for c in /root/nexmon/utilities/nexutil/nexutil \
             /usr/local/bin/nexutil \
             "$(command -v nexutil 2>/dev/null)"; do
        [ -n "$c" ] && [ -x "$c" ] && { echo "$c"; return; }
    done
    return 1
}

# Cached variant to avoid 6 forks per `on`
__NEX_CACHED=""
find_nexutil_cached() {
    if [ -n "$__NEX_CACHED" ] && [ -x "$__NEX_CACHED" ]; then
        printf '%s' "$__NEX_CACHED"
        return 0
    fi
    __NEX_CACHED=$(find_nexutil) || return 1
    printf '%s' "$__NEX_CACHED"
}

get_cfg_country() {
    iw reg get 2>/dev/null | awk '/^country/{sub(/:/,"",$2); print $2; exit}'
}

get_fw_country() {
    local dev="${1:-$STA}"
    local nex cc hex
    nex=$(find_nexutil_cached) || return 1
    cc=$("$nex" -I "$dev" -g 262 -v country -l 16 -r 2>/dev/null | head -c 2 || true)
    if [[ "$cc" =~ ^[A-Z0-9]{2}$ ]]; then
        printf '%s' "$cc"
        return 0
    fi
    hex=$("$nex" -I "$dev" -g 262 -v country -l 16 2>/dev/null | head -1 || true)
    if [[ "$hex" =~ 0x[0-9a-fA-F]+:\ +([0-9a-fA-F]{2})\ +([0-9a-fA-F]{2}) ]]; then
        printf '%b' "\\x${BASH_REMATCH[1]}\\x${BASH_REMATCH[2]}" 2>/dev/null || true
        return 0
    fi
    return 1
}

poll_for_country() {
    local expected="$1" tries=0 cur
    while [ $tries -lt 10 ]; do
        sleep 0.05
        cur=$(get_cfg_country)
        if [ -n "$expected" ] && [ "$cur" = "$expected" ]; then
            return 0
        fi
        if [ -z "$expected" ] && [ -n "$cur" ] && [ "$cur" != "00" ]; then
            return 0
        fi
        tries=$((tries+1))
    done
    return 1
}

sync_regulatory() {
    local dev="${1:-$STA}"
    local desired="${COUNTRY:-}"
    local cfg_cc fw_cc

    cfg_cc=$(get_cfg_country)
    fw_cc=$(get_fw_country "$dev" 2>/dev/null || true)

    if [ -n "$desired" ]; then
        desired=$(printf '%s' "$desired" | tr '[:lower:]' '[:upper:]')
        info "pinning regulatory domain to $desired (explicit COUNTRY override)"
        iw reg set "$desired" 2>/dev/null || warn "iw reg set $desired returned non-zero"
        poll_for_country "$desired" >/dev/null 2>&1 || sleep 0.05
    else
        if [ -z "$cfg_cc" ] || [ "$cfg_cc" = "00" ]; then
            if [ -n "$fw_cc" ] && [ "$fw_cc" != "00" ] && [[ "$fw_cc" =~ ^[A-Z]{2}$ ]]; then
                info "cfg80211 regdom is unset/world (00); syncing to firmware country ($fw_cc)"
                iw reg set "$fw_cc" 2>/dev/null || true
                poll_for_country "$fw_cc" >/dev/null 2>&1 || sleep 0.05
            else
                info "pinning regulatory domain to US (broad 5 GHz channel coverage)"
                iw reg set US 2>/dev/null || true
                poll_for_country "US" >/dev/null 2>&1 || sleep 0.05
            fi
        elif [ -n "$fw_cc" ] && [ "$fw_cc" != "$cfg_cc" ] && [[ "$cfg_cc" =~ ^[A-Z]{2}$ ]]; then
            info "syncing firmware country ($fw_cc) to cfg80211 regdom ($cfg_cc)"
            iw reg set "$cfg_cc" 2>/dev/null || true
            poll_for_country "$cfg_cc" >/dev/null 2>&1 || sleep 0.05
        fi
    fi

    cfg_cc=$(get_cfg_country)
    fw_cc=$(get_fw_country "$dev" 2>/dev/null || echo "<unknown>")

    if [ -n "$cfg_cc" ] && [ "$cfg_cc" = "$fw_cc" ]; then
        info "regulatory domain synchronized: $cfg_cc (cfg80211: $cfg_cc, firmware: $fw_cc)"
    else
        warn "regulatory domain desynchronized: cfg80211=$cfg_cc, firmware=$fw_cc"
        warn "  firmware may reject chanspec requests with -52 (-EBADE). Set COUNTRY=<cc> to match."
    fi
}

# Refuse to tear down the interface the default route lives on. On the dev box
# that is eth0 and the 43455 is wlan0, but do not assume it.
#
# Distinguish "ip could not answer" from "there is no default route". The old
# form collapsed both into an empty $dev, so the comparison passed silently and
# the guard never fired.
check_not_uplink() {
    local routes family dev
    for family in -4 -6; do
        if routes=$(ip "$family" route show default 2>&1); then
            for dev in $(printf '%s\n' "$routes" | awk '{ for (i=1; i<=NF; i++) if ($i == "dev") print $(i+1) }'); do
                if [ "$dev" = "$STA" ]; then
                    die "$STA carries a default route ($family) - bringing it down would cut you off.
       Connect over another link (or set STA=<other iface>) first."
                fi
            done
        fi
    done
}

# The precondition the crash hinges on: the STA must not be associated.
# Returns 0 = unassociated, 1 = still associated after the timeout,
# 2 = could not determine. Callers MUST treat 2 as fatal.
#
# `iw dev X link` exits 0 and prints "Not connected." when the STA is down, but
# exits non-zero when it cannot query the device at all (237 for a missing
# interface), so the exit status is what separates a genuine negative from a
# failed probe. The old pipeline collapsed the two: a failed probe produced
# empty output, grep found no '^Connected', and the `!' inverted that into
# "unassociated" - reporting the exact state that traps the dongle.
wait_unassociated() {
    local _ out
    for _ in $(seq 1 100); do
        if ! out=$(iw dev "$STA" link 2>&1); then
            warn "cannot query association state of $STA: $out"
            return 2
        fi
        case "$out" in
            *Connected*) sleep 0.1 ;;
            *)           return 0 ;;
        esac
    done
    return 1
}

fw_crashed_since() {
    local mark="$1" out
    [ -n "$mark" ] || { echo 0; return 0; }
    out=$(dmesg 2>/dev/null | awk -v m="$mark" 'index($0,m){seen=1} seen' 2>/dev/null || true)
    if [ -z "$out" ] || ! printf "%s" "$out" | grep -q . 2>/dev/null; then
        if command -v journalctl >/dev/null 2>&1; then
            out=$(journalctl -k --no-pager 2>/dev/null | awk -v m="$mark" 'index($0,m){seen=1} seen' 2>/dev/null || true)
        elif [ -r /dev/kmsg ]; then
            out=$(dd if=/dev/kmsg iflag=nonblock bs=64k 2>/dev/null | awk -v m="$mark" 'index($0,m){seen=1} seen' 2>/dev/null || true)
        fi
    fi
    printf "%s" "$out" | grep -c 'fw_crashed' || true
}

phy_of() { cat "/sys/class/net/$1/phy80211/name" 2>/dev/null; }

do_status() {
    info "driver:   $(cat /sys/module/brcmfmac/version 2>/dev/null || echo '<not loaded>')"
    local nex fw_ver="" cur_chan=""
    nex=$(find_nexutil_cached 2>/dev/null || true)
    if [ -n "$nex" ]; then
        fw_ver=$("$nex" -I "$MON" -V 2>/dev/null | awk '/^firmware /{sub(/^firmware /,""); print; exit}' || true)
        if [ -z "$fw_ver" ]; then
            fw_ver=$("$nex" -I "$STA" -V 2>/dev/null | awk '/^firmware /{sub(/^firmware /,""); print; exit}' || true)
        fi
    fi
    if [ -z "$fw_ver" ]; then
        fw_ver=$(dmesg 2>/dev/null | grep 'Firmware: BCM' | tail -1 | sed 's/.*Firmware: //')
    fi
    info "firmware: ${fw_ver:-<unknown>}"

    ip -br link show "$STA" 2>/dev/null || warn "$STA absent"
    ip -br link show "$MON" 2>/dev/null || echo "$MON: not present"

    if ip link show "$MON" >/dev/null 2>&1 && [ -e "/sys/class/net/$STA/flags" ] \
       && [ $(( $(cat "/sys/class/net/$STA/flags") & 1 )) -eq 1 ]; then
        warn "$STA is admin-up while $MON exists - cfg80211 will block channel hops (-EBUSY)."
        warn "  fix: ip link set $STA down"
    fi

    if [ -n "$nex" ]; then
        echo -n "monitor:  "; "$nex" -I "$MON" -m 2>/dev/null \
            || "$nex" -I "$STA" -m 2>/dev/null || echo "<unreadable>"
        cur_chan=$("$nex" -I "$MON" -k 2>/dev/null || "$nex" -I "$STA" -k 2>/dev/null || true)
        if [ -n "$cur_chan" ]; then
            info "channel:  $cur_chan"
        fi
    fi

    local cfg_cc fw_cc
    cfg_cc=$(get_cfg_country)
    fw_cc=$(get_fw_country "$MON" 2>/dev/null || get_fw_country "$STA" 2>/dev/null || echo "<unknown>")
    if [ -n "$cfg_cc" ] && [ "$cfg_cc" = "$fw_cc" ]; then
        info "regulatory: cfg80211=$cfg_cc, firmware=$fw_cc (in sync)"
    elif [ "$cfg_cc" = "00" ]; then
        warn "regulatory: cfg80211=00 (world), firmware=$fw_cc (DESYNCHRONIZED - Broadcom rejects '00'!)"
        warn "  channels permitted by cfg80211 will fail in firmware with -52 (-EBADE). Fix: COUNTRY=$fw_cc $0 on"
    else
        warn "regulatory: cfg80211=$cfg_cc, firmware=$fw_cc (DESYNCHRONIZED - causes -52 errors!)"
        warn "  fix: COUNTRY=$cfg_cc $0 on   # or COUNTRY=US / COUNTRY=DE"
    fi
    iw dev "$STA" link 2>/dev/null | head -2
}

# Stop daemons that background-poll the phy for the session. Records what was
# stopped in STATE_FILE so restore_daemons() can put it back on teardown.
quiesce_daemons() {
    [ "$QUIESCE" = 1 ] || return 0
    if command -v systemctl >/dev/null 2>&1 \
       && systemctl is-active --quiet wpa_supplicant 2>/dev/null; then
        # Use flock to avoid TOCTOU race on concurrent `on` and ensure 600 perms.
        (
            flock -x 200 2>/dev/null || { warn "cannot acquire lock on $STATE_FILE"; exit 1; }
            if ! grep -qx wpa_supplicant "$STATE_FILE" 2>/dev/null; then
                ( umask 077 && touch "$STATE_FILE" ) 2>/dev/null || true
                chmod 600 "$STATE_FILE" 2>/dev/null || true
                if ! printf '%s\n' wpa_supplicant >> "$STATE_FILE" 2>/dev/null; then
                    warn "cannot record daemon state in $STATE_FILE - refusing to stop wpa_supplicant"
                    exit 1
                fi
            fi
        ) 200>"$STATE_FILE.lock" || return 1
        info "stopping wpa_supplicant for the session (removes background phy polling)"
        if ! systemctl stop wpa_supplicant >/dev/null 2>&1; then
            warn "could not stop wpa_supplicant - background scans may still collide with hops"
            return 1
        fi
    fi
}

# Undo quiesce_daemons(). Safe to call unconditionally; a no-op if nothing was stopped.
restore_daemons() {
    [ -f "$STATE_FILE" ] || return 0
    local rc=0
    (
        flock -x 200 2>/dev/null || exit 1
        [ -f "$STATE_FILE" ] || exit 0
        if grep -qx wpa_supplicant "$STATE_FILE" 2>/dev/null; then
            if ! command -v systemctl >/dev/null 2>&1; then
                warn "systemctl unavailable - retaining $STATE_FILE for a later restoration attempt"
                exit 1
            fi
            if systemctl start wpa_supplicant >/dev/null 2>&1; then
                info "wpa_supplicant restarted"
            else
                warn "could not restart wpa_supplicant - retaining $STATE_FILE; retry '$0 off'"
                exit 1
            fi
        fi
        rm -f "$STATE_FILE" || exit 1
    ) 200>"$STATE_FILE.lock" || rc=$?
    return $rc
}

do_off() {
    info "tearing down monitor mode"
    # $MON goes down with the bus up, so the driver's net_mon_stop clears its
    # monitor-restore intent: a later wedge will not auto-bring monitor back.
    # That is deliberate - 'off' means off, including across a reset.
    if NEX=$(find_nexutil_cached) && ip link show "$MON" >/dev/null 2>&1; then
        "$NEX" -I "$MON" -m0 >/dev/null 2>&1 || true
    fi
    ip link set "$MON" down 2>/dev/null || true
    iw dev "$MON" del 2>/dev/null || true
    ip link set "$STA" up 2>/dev/null || true
    if command -v nmcli >/dev/null 2>&1 && systemctl is-active --quiet NetworkManager 2>/dev/null; then
        nmcli dev set "$STA" managed yes >/dev/null 2>&1 || true
        info "$STA handed back to NetworkManager"
    fi
    restore_daemons || return $?
    info "done"
}

# Rollback guard for do_on: once armed, any failure (a die/exit, an interrupt,
# or the trapped-dongle exit) tears the half-configured state back down via
# do_off instead of leaving NetworkManager detached and the interfaces in a
# partial monitor setup. Disarmed on success.
_MON_ROLLBACK_ARMED=0
_WAS_ALREADY_ON=0
monitor_rollback() {
    [ "$_MON_ROLLBACK_ARMED" = 1 ] || return 0
    _MON_ROLLBACK_ARMED=0
    warn "monitor-mode setup did not complete - rolling back to a clean state"
    if [ "${_WAS_ALREADY_ON:-0}" = 1 ]; then
        warn "retaining pre-existing monitor session without full teardown"
    else
        do_off
    fi
}

do_on() {
    check_not_uplink

    NEX=$(find_nexutil_cached) || die "nexutil not found - build utilities/nexutil, or set NEXUTIL=<path>"
    info "nexutil: $NEX"

    ip link show "$STA" >/dev/null 2>&1 || die "$STA does not exist (is brcmfmac loaded?)"

    local was_already_on=0
    if ip link show "$MON" >/dev/null 2>&1 && iw dev "$MON" info 2>/dev/null | grep -q "type monitor"; then
        was_already_on=1
    fi
    _WAS_ALREADY_ON=$was_already_on

    local mark
    mark=$(dmesg 2>/dev/null | tail -1)

    # Arm rollback now: every step below mutates host state, so from here a
    # failure must undo it rather than leave the box half-configured.
    _MON_ROLLBACK_ARMED=1
    trap monitor_rollback EXIT INT TERM

    # 1. Stop NetworkManager reassociating behind our back. This is the step
    #    that separates this box from the rig Stage 5c was verified on.
    if command -v nmcli >/dev/null 2>&1 && systemctl is-active --quiet NetworkManager 2>/dev/null; then
        info "setting $STA unmanaged"
        nmcli dev set "$STA" managed no >/dev/null 2>&1 \
            || warn "nmcli failed - if something re-associates $STA, this will trap the dongle"
    fi

    # 1b. Stop wpa_supplicant's background polling of the phy (see QUIESCE).
    #     Its get_channel DCMDs otherwise collide with airodump's channel hops
    #     and can time the firmware out (-110). Restored by do_off.
    quiesce_daemons || die "could not safely quiesce daemons"

    # 2. rfkill unblock, STA up (the vif is created off a live phy).
    command -v rfkill >/dev/null 2>&1 && rfkill unblock wifi 2>/dev/null || true
    ip link set "$STA" up || die "could not bring $STA up"

    # 2b. Synchronize cfg80211 and Broadcom firmware regulatory domains.
    sync_regulatory "$STA"

    # 3. Confirm the precondition before anything touches SET_MONITOR.
    info "waiting for $STA to be unassociated"
    local rc=0
    wait_unassociated || rc=$?
    case $rc in
        0) info "$STA is unassociated - safe to proceed" ;;
        2) die "could not verify that $STA is unassociated.
       Refusing to issue SET_MONITOR: enabling monitor mode while the STA is
        associated is what traps the firmware. Confirm that $STA exists and that
        'iw dev $STA link' succeeds, then re-run." ;;
        *) die "$STA is still associated.
       Enabling monitor mode now is what traps the firmware. Disconnect it
        (nmcli dev disconnect $STA, or stop wpa_supplicant) and re-run." ;;
    esac

    # 4. Monitor vif, before the mode. Note that bringing $MON up already makes
    #    the driver issue SET_MONITOR=2 (brcmf_net_mon_open sees
    #    ARPHRD_IEEE80211_RADIOTAP); the explicit nexutil -m2 below follows the
    #    documented procedure and is harmless if the mode is already set.
    local PHY
    PHY=$(phy_of "$STA") || true
    [ -n "${PHY:-}" ] || die "could not read phy for $STA"
    info "phy: $PHY"

    if [ "$was_already_on" = 1 ]; then
        info "monitor vif $MON already active; reusing existing interface"
    else
        iw dev "$MON" del 2>/dev/null || true
        info "adding monitor vif $MON"
        iw phy "$PHY" interface add "$MON" type monitor \
            || die "could not create $MON"
    fi

    # Keep NetworkManager off the monitor vif too, so it never grabs or polls
    # get_channel on it (verified: with $STA + $MON both unmanaged and
    # wpa_supplicant stopped, nothing polls the phy while idle).
    command -v nmcli >/dev/null 2>&1 && systemctl is-active --quiet NetworkManager 2>/dev/null && nmcli dev set "$MON" managed no >/dev/null 2>&1 || true

    ip link set "$MON" up || die "could not bring $MON up"

    # 5. Leave the STA up (unassociated). wlc_send_q / hwrs_scb are tied to
    #    the primary BSS; bringing $STA down leaves that BSS not-up so
    #    injected frames are echoed locally but never DMA'd. Power save on
    #    the STA also re-gates the D11 clocks.
    iw dev "$STA" set power_save off 2>/dev/null || true

    info "enabling nexmon radiotap monitor mode (-m2)"
    "$NEX" -I "$MON" -m2 || warn "nexutil -m2 returned non-zero"
    # Re-assert radio-up ioctls in case SET_MONITOR flipped them.
    # Do not set scansuppress: airodump/aireplay then hang in SET_CHANNEL (-110).
    "$NEX" -I "$MON" -s86 -i -v0 >/dev/null 2>&1 || true   # WLC_SET_PM=0

    # cfg80211_set_monitor_channel returns -EBUSY unless the phy has only
    # monitor interfaces running. Take the STA netdev down so airodump-ng
    # can hop. The nexmon driver leaves the primary BSS up in firmware
    # while a monitor vif is running, so hwrs_scb / D11 clocks stay put
    # and injection still keys.
    info "bringing $STA netdev down so cfg80211 allows channel hops"
    ip link set "$STA" down || warn "could not bring $STA down"

    "$NEX" -I "$MON" -m2 >/dev/null 2>&1 || true
    "$NEX" -I "$MON" -s86 -i -v0 >/dev/null 2>&1 || true

    # Poll for monitor mode to settle (was fixed sleep 1) — 20×0.05s = 1s max
    tries=0
    while [ $tries -lt 20 ]; do
        sleep 0.05
        if "$NEX" -I "$MON" -m >/dev/null 2>&1; then
            break
        fi
        tries=$((tries+1))
    done

    # 6. Verify: mode readback, and no dongle trap since we started.
    local mode crashes
    mode=$("$NEX" -I "$MON" -m 2>/dev/null || echo "<unreadable>")
    crashes=$(fw_crashed_since "$mark")

    echo
    info "monitor state: $mode"
    if [ "${crashes:-0}" -gt 0 ]; then
        warn "dongle trapped ($crashes x brcmf_fw_crashed) - check: dmesg | tail -40"
        warn "if $STA was genuinely unassociated, the association theory is wrong;"
        warn "next step is mapping the trap epc/lr with buildtools/fwmap."
        exit 1
    fi
    info "no firmware trap detected"
    if [ -n "$CHAN" ]; then
        info "setting channel $CHAN"
        # Check if the requested channel is marked disabled under active regulatory domain
        if iw phy "$PHY" channels 2>/dev/null | grep -E "\[$CHAN\]" | grep -q "disabled"; then
            warn "channel $CHAN is marked (disabled) under regulatory domain $(get_cfg_country)!"
            warn "  firmware will likely reject this channel with -52 (-EBADE / BCME_BADCHAN)."
            warn "  hint: US allows 5GHz 149-165; DE/NO/ETSI allows channels 12-13. Re-run with COUNTRY=<cc>."
        fi
        # cfg80211 returns EBUSY for iw set channel while the STA vif is up;
        # nexutil talks to the firmware chanspec directly.
        iw dev "$MON" set channel "$CHAN" 2>/dev/null \
            || "$NEX" -I "$MON" -k"$CHAN" \
            || warn "could not set channel $CHAN"
    fi
    echo
    info "capture with:   tshark -i $MON -c 40"
    info "hop/scan with:  airodump-ng $MON"
    info "5 GHz too:      airodump-ng --band abg $MON"
    info "tear down with: $0 off"
    info "if the chip wedges (-110 cascade) the driver auto-recreates $MON and"
    info "  restores channel & link state automatically (surviving wedges transparently)"

    # Success: keep the configured monitor state, disarm the rollback.
    _MON_ROLLBACK_ARMED=0
    trap - EXIT INT TERM
}

do_channel() {
    local ch="${1:-}"
    [ -n "$ch" ] || die "missing channel number. Usage: $0 channel <1-165>"
    local nex phy
    nex=$(find_nexutil_cached) || die "nexutil not found - build utilities/nexutil, or set NEXUTIL=<path>"
    ip link show "$MON" >/dev/null 2>&1 || die "$MON is not present. Enable monitor mode first ('$0 on')."

    phy=$(phy_of "$STA" 2>/dev/null || phy_of "$MON" 2>/dev/null || true)
    if [ -n "$phy" ] && iw phy "$phy" channels 2>/dev/null | grep -E "\[$ch\]" | grep -q "disabled"; then
        warn "channel $ch is marked (disabled) under regulatory domain $(get_cfg_country)!"
        warn "  firmware will likely reject this channel with -52 (-EBADE / BCME_BADCHAN)."
    fi

    info "setting $MON channel to $ch"
    if iw dev "$MON" set channel "$ch" 2>/dev/null || "$nex" -I "$MON" -k"$ch"; then
        local curr
        curr=$("$nex" -I "$MON" -k 2>/dev/null || true)
        info "channel set: ${curr:-$ch}"
    else
        die "could not set channel $ch"
    fi
}

case "${1:-on}" in
    on|enable)
        if [ -n "${2:-}" ]; then
            CHAN="$2"
        fi
        if [ -n "${3:-}" ]; then
            COUNTRY="$3"
        fi
        do_on
        ;;
    "")
        do_on
        ;;
    off|disable)
        do_off
        ;;
    status)
        do_status
        ;;
    channel|chan|set-channel|ch)
        do_channel "${2:-}"
        ;;
    *)
        if [[ "$1" =~ ^[0-9]+(/[0-9]+)?$ ]] || [[ "$1" =~ ^6g[0-9]+(/[0-9]+)?$ ]]; then
            # Numeric / chanspec first argument: e.g. ./monitor-mode.sh 36 [country], 36/80, 6g37
            CHAN="$1"
            if [ -n "${2:-}" ]; then
                COUNTRY="$2"
            fi
            do_on
        else
            die "usage: $0 [on [channel] [country] | off | status | channel <1-196>]"
        fi
        ;;
esac
