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
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2023 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>   // definition of firmware version macros
#include <patcher.h>            // macros used to create patches such as BLPatch, BPatch, ...

/* SPC_STA_INJECT selects the SP-C build variant (defined early so cap_add_monitor
   below can see it). The fw TX path derefs associated-STA context (scb rate
   tables @scb+0x300, CSA, keymgmt, ...) that pure monitor mode never builds —
   three on-device traps proved it. So this variant does NOT advertise the
   "monitor" cap token (STA association keeps working) and uses the REAL scb the
   association creates for the AP (fully populated → all TX-path checks pass),
   instead of a fabricated bcmc scb. The spc_tx hook, pkt build and
   wlc_queue_80211_frag submit are shared with the monitor variant.
   0 = monitor image (RX capture); 1 = STA+inject image. */
#define SPC_STA_INJECT 1

extern unsigned char ucode0_compressed_bin[];
extern unsigned int ucode0_compressed_bin_len;
extern unsigned char ucode1_compressed_bin[];
extern unsigned int ucode1_compressed_bin_len;
extern unsigned char ucode2_compressed_bin[];
extern unsigned int ucode2_compressed_bin_len;
extern unsigned char templateram0_bin[];
extern unsigned char templateram1_bin[];
extern unsigned char templateram2_bin[];
extern unsigned char templateram3_bin[];

// Reduce reclaimed area to reserve patch space
__attribute__((at(HNDRTE_RECLAIM_3_END_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(hndrte_reclaim_3_end, PATCHSTART);

// Hook the call to wlc_ucode_write in wlc_ucode_download
__attribute__((at(WLC_UCODE_WRITE_BL_HOOK_ADDR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BLPatch(wlc_ucode_write_compressed_args, wlc_ucode_write_compressed_args);

// Update pointers to ucodes and ucodesizes
__attribute__((at(UCODE0START_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(ucode0_start, ucode0_compressed_bin);
__attribute__((at(UCODE0SIZE_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(ucode0_len, &ucode0_compressed_bin_len);

__attribute__((at(UCODE1START_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(ucode1_start, ucode1_compressed_bin);
__attribute__((at(UCODE1SIZE_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(ucode1_len, &ucode1_compressed_bin_len);

__attribute__((at(UCODE2START_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(ucode2_start, ucode2_compressed_bin);
__attribute__((at(UCODE2SIZE_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(ucode2_len, &ucode2_compressed_bin_len);

// Moving template ram to another place in the ucode region
__attribute__((at(TEMPLATERAM0START_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(templateram0_bin, templateram0_bin);
__attribute__((at(TEMPLATERAM1START_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(templateram1_bin, templateram1_bin);
__attribute__((at(TEMPLATERAM2START_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(templateram2_bin, templateram2_bin);
__attribute__((at(TEMPLATERAM3START_PTR, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(templateram3_bin, templateram3_bin);

// do not enable mmu protection by overwriting BL to hnd_mmu_enable_protection
__attribute__((at(0x2769B2, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
__attribute__((naked))
void
no_mmu_protection_patch(void)
{
    asm(
         "nop\n"
         "nop\n"
         );
}

// SP-B Part A: defeat the monitor-capability gate in wlc_set_monitor (RAM copy @0x284ca0).
// Stock: `ldrb pub[0x129]; cmp 0; beq 0x284d30` -> returns -23 (BCME_UNSUPPORTED -> errno 95)
// because Broadcom cleared the per-chip monitor-capable byte pub+0x129. The full, working
// enable body already sits below the gate (sets wlc->monitor@+0x120, phy monitor, and
// wlc_mctrl(wlc,0x1e00000,0x1e00000) = MCTL_PROMISC|KEEPCONTROL|KEEPBADFCS|KEEPBAD). NOP the
// `beq 0x284d30` (bytes 3f d0) -> Thumb NOP (00 bf) so it falls through to that enable body.
// 2 bytes only; must NOT touch the ldrb.w at 0x284cb0 (the secondary "-26 must be idle" gate).
// (Part B — the RX radiotap glue for the UNIMPL monitor-recv slot @0x20fb9c — comes next.)
__attribute__((at(0x284CAE, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch2(sp_b_monitor_cap_gate_nop, 0xBF00);

// SP-B Part A.5: advertise "monitor" in the fw "cap" iovar so the STOCK driver auto-enables
// monitor (no mon_flip module). dhd_optimised_preinit_ioctls does strstr(cap," monitor ") and
// sets dhd_pub+0x1b8. The cap-string builder wlc_cap_bcmstrbuf@0x282490 emits ~20 tokens but no
// "monitor". wlc$wlc_cap@0x2828dc calls it once via `bl 0x282490` @0x2828f4 (r0=wlc, r1=b). Hook
// that BL: run the original, then append "monitor " (trailing space, matching every other token)
// so the driver's strstr(" monitor ") matches. Zero token loss; STA cap otherwise unchanged.
#define FP_WLC_CAP_BCMSTRBUF ((void (*)(void *, void *))(0x00282490u | 1u))
#define FP_BCM_BPRINTF       ((int  (*)(void *, const char *, ...))(0x0000b5e8u | 1u))

/* SP-B telemetry, read from the host with `wlcmd <if> getstr cap`.
   The firmware console is NOT readable on this device (dhd_console_ms is
   honoured but no CONSOLE: line ever reaches dmesg — verified live), so the
   cap iovar is our only proven fw->host channel: it is re-rendered on every
   read, so these counters are live. Non-zero initialiser keeps the array in
   .data inside the patch region rather than .bss. */
volatile unsigned int spb_tel[14] = { 0x53504231u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#define SPB_T_MAGIC 0   /* 'SPB1' */
#define SPB_T_HOOK  1   /* hook entries, any state          */
#define SPB_T_MON   2   /* hook entries with monitor on     */
#define SPB_T_UP    3   /* frames handed to wl_sendup_rxcmpl */
#define SPB_T_SKIP  4   /* monitor on but not deliverable   */
#define SPB_T_CS    5   /* last chanspec | dma_flags<<16    */
#define SPB_T_LEN   6   /* last p->len   | fc<<16           */
#define SPB_T_ROOM  7   /* last headroom | monitor<<16      */
#define SPB_T_ACTIVE 8  /* monitor RX delivery live (read by the pciedev patch) */
#define SPB_T_RXCPL  9  /* last rxcplid                     */
#define SPB_T_MCODE 10  /* raw fw monitor sub-code (pciedev patch) */
#define SPB_T_CMPL  11  /* completions rewritten (pciedev patch)   */
#define SPB_T_MONAT 12  /* mon_info attach: 0 none, else slot+1    */

/* SP-C (frame injection / TX) telemetry, same cap-iovar channel as spb_tel.
   Non-zero initialiser keeps it in .data inside the patch region. */
volatile unsigned int spc_tel[15] = { 0x53504331u, 0,0,0,0,0,0,0,0,0,0,0,0,0,0 };  /* 'SPC1' */
#define SPC_T_HOOK   1  /* spc_tx iovar invocations                     */
#define SPC_T_LEN    2  /* last frame length                            */
#define SPC_T_FB     3  /* last frame first 4 bytes (le)                */
#define SPC_T_BSSCFG 4  /* primary bsscfg *(wlc+0x168)  (Task 3a probe)  */
#define SPC_T_DTXP   5  /* default txparams *(*(wlc+0x24)+0x10)          */
#define SPC_T_SCB    6  /* default no-peer scb *(dtxp+0xc)              */
#define SPC_T_SCBBSS 7  /* scb->bsscfg *(scb+0xc)                       */
#define SPC_T_SCBBND 8  /* scb band *(scb+0x28)                        */
#define SPC_T_PREP   9  /* wlc_prep_pdu return (Task 3b)                */
#define SPC_T_DMA   10  /* submit return (Task 4)                       */
#define SPC_T_ALLOC 11  /* wlc_bsscfg_bcmcscballoc return (Task 3b)      */
#define SPC_T_G6C   12  /* bsscfg[0x6c] — the bcmcscballoc guard word     */
#define SPC_T_PKT   13  /* allocated pkt pointer (Task 3b step 2)        */
#define SPC_T_TXQ   14  /* txq = *(*(bss+8)+0xc) (Task 4)               */

/* SP-C Build-1 guarded probe (de-risk before TX): read the per-radio scb
   container *(wlc+0x50) = 0x401d5044 from the wl-DPC RX context, to test whether
   it is readable while the radio is active — it faults from the ioctl/iovar
   context as a backplane bus abort (see the SP-C injection RE write-up).
   Armed by an spc_tx iovar of length 1; fired once by the next monitor RX frame.
   Non-zero initialiser keeps it in .data inside the patch region. */
volatile unsigned int spc_probe[7] = { 0x53504350u, 0,0,0,0,0,0 };  /* 'SPCP' */
#define SPCP_REQ   1  /* armed: host sets via a 1-byte spc_tx; the hook clears it */
#define SPCP_N     2  /* fired count                                             */
#define SPCP_CONT  3  /* *(wlc+0x50) — the container pointer (expect 0x401d5044) */
#define SPCP_W0    4  /* [container+0]  — THE read that faults from the ioctl ctx */
#define SPCP_W4    5  /* [container+4]                                           */
#define SPCP_OK    6  /* 0x00C0FFEE iff the read completed without a fault        */

/* --- SP-C Build-2 (corrected P2 + Fix A): read/submit from a proper wl-task DPC
   Build-1's probe read *(wlc+0x50) from the MONITOR wlc (container 0x432e14, low/
   readable-anywhere) — a false positive: RSDB has two wlc's, and injection uses
   the STA wlc whose container is 0x401d5044 (high backplane, the real wall). Live
   proof (same build): ioctl ctx (STA wlc) *(scb+0)=0x401d5044; RX ctx *(wlc+0x50)=
   0x432e14. Build-2 defers the container read (and, armed with a real frame, the
   whole submit) into a DPC bound to the WL TASK — captured cheaply via
   osl_ext_task_current() from sp_b_monitor_rx (it is hooked inside wlc_recv, which
   runs on the wl task = the normal TX-drain context). One build = the true P2
   read-check AND the Fix-A submit machinery. */
#define FP_OSL_EXT_TASK_CURRENT ((void *(*)(void))(0x00090e60u | 1u))
#define FP_HND_DPC_REGISTER     ((void *(*)(void *fn, void *arg, void *task))(0x0008ab90u | 1u))
#define FP_HND_DPC_SCHEDULE     ((int   (*)(void *dpc))(0x0027749cu | 1u))

volatile unsigned int spc_dpc_obj  = 1;   /* registered DPC object (once); .data  */
volatile unsigned int spc_wl_task  = 1;   /* captured wl-task handle              */
volatile unsigned int spc_dpc_mode = 0;   /* 1 = read container, 2 = submit frame */
volatile unsigned int spc_job_wlc2 = 1;   /* STA wlc cached by the iovar          */
volatile unsigned int spc_job_scb  = 1;   /* cached scb  (submit mode)            */
volatile unsigned int spc_job_txq  = 1;   /* cached txq  (submit mode)            */
volatile unsigned int spc_job_len  = 0;   /* cached frame length (submit mode)    */
unsigned char         spc_job_buf[0x600] = { 0xAAu }; /* cached frame; non-zero init -> .data (patch region) */

volatile unsigned int spc_probe2[8] = { 0x53504332u, 0,0,0,0,0,0,0 };  /* 'SPC2' */
#define SPC2_MODE  1  /* last dpc mode seen (1 read / 2 submit)                   */
#define SPC2_N     2  /* dpc fired count                                         */
#define SPC2_WLC   3  /* STA wlc read in the dpc                                 */
#define SPC2_CONT  4  /* *(wlc+0x50) (read) or *(scb+0) (submit) — expect 401d5044 */
#define SPC2_W0    5  /* [container+0] read | submit: alloc'd pkt ptr            */
#define SPC2_W4    6  /* [container+4] read | submit: queue_80211_frag return    */
#define SPC2_OK    7  /* 0xC0FFEE = container read OK | 0x5B117 = submit reached  */

/* --- SP-C Build-8 (ROOT-CAUSE FIX): force the MAC/backplane clock across the submit
   Builds 2-7 confirmed intermediate-deref patching is unbounded whack-a-mole: the
   container 0x401d5044 is dereferenced in many TX-path functions. Root cause (RE via
   Ghidra ROM+RAM): 0x401d5044 lives in MAC/backplane memory that is CLOCK-GATED when
   the radio is idle. Normal TX reads it only because it runs from wlc_dpc (interrupt
   context: the MAC is clocked). So instead of patching each deref, WAKE + CLOCK the
   MAC across our synchronous submit — then every container deref reads, no patching.
   wlc_force_ht(wlc,1) forces the fast HT/backplane clock (+ wlc_bmac_wait_for_ht);
   wlc_ucode_wake_override_set sets MCTL_WAKE (0x4000000) + waits for wake. */
#define FP_WLC_FORCE_HT        ((int  (*)(void *wlc, int on, void *old))(0x000a34acu | 1u))
#define FP_WAKE_OVERRIDE_SET   ((void (*)(void *wlc, unsigned int mask))(0x000e2a84u | 1u))
#define FP_WAKE_OVERRIDE_CLEAR ((void (*)(void *wlc, unsigned int mask))(0x000e2a14u | 1u))
#define SPC_WAKE_MASK 0x10u
/* Build-10: SR (save/restore) power is PER-CORE (si_srpwr_domain_all_mask=0x1f with a
   scan core). 0x401d5044 is scan-core memory whose SR domain force_ht/wake don't touch.
   wlc_bmac_srpwr_request_on_all(bmac, force) powers EVERY core's SR domain on. */
#define FP_SRPWR_ON_ALL        ((void (*)(void *bmac, int force))(0x000e18f0u | 1u))
volatile unsigned int spc_hf[4] = { 0x53504346u, 0, 0, 0 };  /* 'SPCF' diag: [1]=container [2]=[cont+0x18] [3]=RX-read arm */

/* SP-C Build-11 (SAFE READ-ONLY srpwr observation, cannot UDF).
   -------------------------------------------------------------------------------
   Build-10 tried wlc_bmac_srpwr_request_on_all(bmac, 1) from the ioctl thread and
   the fw died in si_srpwr_stat_spinwait's UDF timeout — reproducibly. Root cause,
   from the Ghidra decomp:

     si_srpwr_request(sih, mask, val) @ 0x27725c
       if (mask==0 && val==0) return raw ctrl reg   <-- SAFE, no write
       else                    write bits, spinwait_for_stat_ack, log time
     si_srpwr_stat_spinwait @ 0x277214
       poll STAT bit for up to 0x5dd*10us (~15ms), then software_udf(0xff) on
       timeout -> fatal trap. THIS is Build-10's crash, not hnd_time_us.
     si_srpwr_stat(sih) @ 0x2772bc
       si_corereg(...offset 0x1e8...) — plain MMIO read; never writes, never spins.

   Build-11 answers the load-bearing open question in INJECTION-RE §11.5(a) —
   "does the scan-core SR domain EVER assert during normal operation?" — using
   only the two safe primitives above. NO srpwr writes anywhere. Two channels:

     (1) A "len==3" spc_tx probe reports the current SR ctrl+stat from the ioctl
         context (a snapshot at the moment of the probe).
     (2) sp_b_monitor_rx samples SR stat on every RX frame (real wlc_dpc context)
         and rolls up min/max/last + a counter of frames where the scan-core bit
         (0x10) was set. Cost per RX: one MMIO read + a few loads/stores.

   If (2) EVER shows sc_up_cnt > 0, we have a natural SC-up window and Build-12
   can piggyback on it (queue-then-wait pattern). If sc_up_cnt stays 0 forever,
   Build-12 must actively bring the SC up (wl_sc_do_up equivalent) — the srpwr
   request approach is proven unsuitable from ioctl.

   RE anchors:
     wlc + 0x18 = bmac (wlc_hw_info)          [wlc_srpwr_request_on decomp]
     bmac + 0x138 = sih                       [wlc_bmac_srpwr_request_on_all decomp]
     scan-core SR mask bit = 0x10             [srpwr_request_on_all decomp: coreunit 2]
     si_srpwr_domain_all_mask = 0x1f          [scan core present on this chip]
   ------------------------------------------------------------------------------- */
#define FP_SI_SRPWR_REQUEST  ((unsigned int (*)(void *sih, int mask, int val))(0x0027725du))
#define FP_SI_SRPWR_STAT     ((unsigned int (*)(void *sih))(0x002772bdu))
#define SPC_WLC_BMAC_OFF     0x18u
#define SPC_BMAC_SIH_OFF     0x138u
#define SPC_SR_SCAN_CORE     0x10u   /* SR domain bit for coreunit 2 (scan core) */

volatile unsigned int spc_sr_tel[14] = { 0x53525031u, 0,0,0,0,0,0,0,0,0,0,0,0,0 };  /* 'SRP1' */
#define SR_T_MAGIC     0
#define SR_T_PROBE_N   1   /* count of len==3 probes (ioctl-context snapshot reads)      */
#define SR_T_WLC       2   /* wlc at probe time                                          */
#define SR_T_BMAC      3   /* bmac = *(wlc+0x18) at probe time                           */
#define SR_T_SIH       4   /* sih  = *(bmac+0x138) at probe time                         */
#define SR_T_CTRL      5   /* si_srpwr_request(sih,0,0) — raw ctrl reg at probe time     */
#define SR_T_STAT      6   /* si_srpwr_stat(sih)       — powered domains at probe time  */
#define SR_T_RX_N      7   /* count of RX frames where the sampler ran                   */
#define SR_T_LAST      8   /* last stat sampled at RX                                    */
#define SR_T_MIN       9   /* min stat seen at RX (0 = uninitialised)                    */
#define SR_T_MAX      10   /* max stat seen at RX                                        */
#define SR_T_SC_UP    11   /* count of RX frames where stat & 0x10 (scan core) was set   */
#define SR_T_STAT_OR  12   /* OR of all RX-time stats — "any domain we've ever seen on"  */
#define SR_T_STAT_AND 13   /* AND of all RX-time stats — "always-on domains"             */

/* SP-C Build-13B (2026-08-09) — hook wl_sc_dpc @0x98c5c entry, read the STA AP scb's
   container [cont+0x18] IN THAT CONTEXT. Build-12C proved the RX-context stat gate
   (STA's wlc_recv hook) is insufficient — the RX hook runs on the STA task, and by
   the time it derefs the container the SC's window has already closed. FINDINGS-E
   named wl_sc_dpc as the context that reads the container fine (it runs after
   wlc_bmac_dpc_cmn ensures the SC's MAC/backplane is clocked). This build tests
   that promise directly. Also (Path A prep, free) caches sc_state — the SC-side
   bmac wrapper wl_sc_do_up @0x98b2c needs, but is not a static literal.
     reads grows without a trap → the DPC context is the right one → Build-14
       replaces the read with wlc_queue_80211_frag under the same hook.
     reads flat, attempts grows → mirror scb stale after roam (visible in miss).
     wlan0 traps → the RE was wrong about wl_sc_dpc; back to Path A design. */
volatile unsigned int spc_dpc_tel[6] = { 0x44504331u, 0,0,0,0,0 };  /* 'DPC1' */
#define DPC_T_MAGIC   0
#define DPC_T_ATTEMPT 1  /* wl_sc_dpc entries observed                             */
#define DPC_T_CONT    2  /* last *(scb+0) seen (expect 0x401d5044/0x401d5040)      */
#define DPC_T_LAST    3  /* last [cont+0x18] value read                            */
#define DPC_T_READS   4  /* successful [cont+0x18] derefs (no trap)                */
#define DPC_T_MISS    5  /* attempts where cont was NOT in 0x40000000+ window      */

/* SC-side sc_state pointer, captured on the first wl_sc_dpc entry. Non-zero
   initialiser keeps it in .data (patch region). Path A (full wl_sc_do_up
   emulation) will read this to reach the SC's bmac. */
volatile unsigned int spc_sc_state = 1;

/* SP-C Build-13B-part2: hook wlc_bmac_srpwr_request_on @0x29daa0 (RAM).
   Diagnostic-only: proves hook wiring end-to-end on a high-frequency callsite
   (this function has many callers: wlc_up, wlc_bmac_intrson/off, wlc_bmac_isr,
   wl_sc_up_wbus, wlc_recv_sc, ...). If SRP_T_HOOK grows while DPC_T_ATTEMPT
   stays at 0, wl_sc_recv is genuinely silent on this device and we need Path A
   proper (full wl_sc_do_up emulation). If BOTH stay at 0, my BPatch mechanics
   are wrong and every hook-based experiment is compromised. Also captures the
   last observed bmac so we can see how many distinct bmacs exist. */
volatile unsigned int spc_srp_tel[3] = { 0x53525032u, 0, 0 };  /* 'SRP2' */
#define SRP_T_MAGIC 0
#define SRP_T_HOOK  1  /* wlc_bmac_srpwr_request_on invocations                  */
#define SRP_T_BMAC  2  /* last observed r0 (bmac) at call time                   */

/* SP-C Build-14 (2026-08-09) — RE-GATE [cont+0x18] on PMU_RES_STATE bit 13
   (CORE_RDY_SCAN), with Broadcom's own CORE_PWRUP_WAR pattern
   (hnd_delay + re-check; NEVER si_srpwr_stat_spinwait — Build-10's UDF).

   Build-12C's "gate insufficient" verdict was a MEASUREMENT ERROR: si_srpwr_stat
   reports powerctl STATUS = PWRSW_SCAN (res id 12), which asserts quickly but
   does NOT mean SC backplane RAM is readable. The real gate is PMU_RES_STATE
   bit CORE_RDY_SCAN (res id 13, chipc offset 0x60c) — heavier dep chain,
   ~30 ms up-timer at 32.768 kHz ILP. Broadcom's own bcmdhd driver documents the
   exact failure verbatim (GrapheneOS bcmdhd-4389 siutils.c ~L9411) and applies
   hnd_delay(SRPWR_UP_DOWN_DELAY = 100 us, '>3 ILP clocks') + re-check before
   any backplane read.

   RE anchors (all HIGH-confidence, independently RE-cross-checked):
     si_corereg     @0x00276fcc  (sih, coreidx, regoff, mask, val) -> u32
     si_findcoreidx @0x00276f82  (sih, coreid, unit) -> int   (>=0x16 = fail)
     hnd_delay      @0x00274820  (unsigned int us) -> void
     PMU coreid = 0x827 (CC_CORE_ID for BCM SI PMU cores on this fw)
     PMU_RES_STATE = chipc offset 0x60c
     CORE_RDY_SCAN = bit 13 ; CORE_RDY_AUX = bit 12 (free-lunch discriminator
     for SCAN vs AUX — settles the MED-confidence question on-device without
     another build). */
#define FP_SI_COREREG     ((unsigned int (*)(void *sih, unsigned int coreidx, unsigned int regoff, unsigned int mask, unsigned int val))(0x00276fccu | 1u))
#define FP_SI_FINDCOREIDX ((int          (*)(void *sih, unsigned int coreid, unsigned int unit))(0x00276f82u | 1u))
#define FP_HND_DELAY      ((void         (*)(unsigned int us))(0x00274820u | 1u))

#define SPC_R2_PMU_COREID       0x827u
#define SPC_R2_PMU_RES_STATE    0x60cu
#define SPC_R2_RDY_SCAN_BIT     (1u << 13)
#define SPC_R2_RDY_AUX_BIT      (1u << 12)
#define SPC_R2_UPDN_DELAY_US    100u
#define SPC_R2_MAX_SKIPS        512u
#define SPC_R2_MIN_RDY_SEEN     1u

/* SP-C Build-15 (2026-08-09) — LAYER Path #2 pin on top of Build-14.
   Build-14 disproved that CORE_RDY_SCAN gating alone is sufficient: the fast
   path (rs & bit 13 = 1 at sample time) still trapped a few cycles later.
   Matches Broadcom's own warning verbatim: "STATUS can indicate ON while in
   ON→OFF transition, and reading in that window raises an AXI slave error."
   The vendor's actual fix (per GrapheneOS bcmdhd-4389 siutils.c ~L5640) is
   `si_pmu_min_res_set` — OR-in a resource-mask into MinResMask (chipc 0x618)
   and wait for RES_PENDING to drain. This PINS the SCAN resources on so the
   ON→OFF race is eliminated entirely.

   Delta on top of Build-14: two new spc_tx handlers (len==8 arm, len==9
   release) + one new telemetry array. Build-14's srpwr_hook + res_state gate
   stays wired; the operator sequence is:
     (a) `wlcmd tx <8-byte>`  → si_pmu_min_res_set(SCAN_KEEP_MASK, set=1)
     (b) `wlcmd tx <7-byte>`  → arm the srpwr-hook deref (Build-14)
     (c) watch cap `r2 reads=N` — should now grow without a trap
     (d) `wlcmd tx <9-byte>`  → release the pin (default-min protection makes
                                 release safe against unsetting required bits)

   RE anchors (all HIGH-confidence, independently RE-cross-checked):
     si_pmu_min_res_set @0x0001b8e8  (sih, osh, mask, set)
     PMU_MIN_RES_OFF    = 0x618      (MinResMask register offset in chipc PMU)
     hnd_time           @0x00273c9c  () -> u32 microseconds (for timestamps)

   Resource bit choice (default = SCAN per Verify strand 1's MED-confidence):
     PWRSW_SCAN(12) | CORE_RDY_SCAN(13) | MACPHY_CLK_SCAN(24) = 0x01003000
   If Build-15 arms cleanly and post != pre in cap but the deref STILL traps
   under Build-14's arm, Build-16 = SPC_PIN_MASK → 0x00058000 (AUX variant).

   Safety: si_pmu_min_res_set is bounded (15 ms hard timeout, NO UDF —
   distinct from si_srpwr_stat_spinwait's UDF path). Release path is
   AND-NOT with default-min protection, so we can never unset system-required
   bits. Cold boot with no arm = no PMU state change, only passive sampling. */
#define FP_SI_PMU_MIN_RES_SET \
    ((void (*)(void *sih, void *osh, unsigned int mask, int set))(0x0001b8e9u))
#define FP_HND_TIME   ((unsigned int (*)(void))(0x00273c9du))

#define SPC_PMU_MIN_RES_OFF   0x618u
#define SPC_PIN_MASK_SCAN     ((1u<<12) | (1u<<13) | (1u<<24))    /* = 0x01003000 */
#define SPC_PIN_MASK_AUX      ((1u<<15) | (1u<<16) | (1u<<18))    /* = 0x00058000 */
/* SP-C Build-16: swap to AUX. Build-15's SCAN pin arm returned ret=0 (call
   succeeded) but the [cont+0x18] deref still trapped at same epc. Two
   hypotheses live: (A) container is in AUX (DMN2) not SCAN (DMN4) — Verify
   strand-1 was MED confidence for SCAN; (B) pin didn't take. Build-16 tests
   (A) directly by swapping the mask; ALSO reorders cap emissions and drops
   the always-fast-path deref so pin post + re-checked res_state are visible. */
#define SPC_PIN_MASK          SPC_PIN_MASK_AUX
#define SPC_R2_RDY_TEST_BIT   SPC_R2_RDY_AUX_BIT  /* gate the deref on the AUX bit too */

volatile unsigned int spc_pin_tel[8] = { 0x50494e31u, 0,0,0,0,0,0,0 };  /* 'PIN1' */

/* SP-C Build-17-A (2026-08-09, night) — POST-TRAP snapshot at wlc_scb_get_bssidx.
   The insight that broke every prior build's premise:  ALL 15+ hook attempts
   (Builds 2, 8, 9, 12C, 14, 15, 16) placed the container-deref BEFORE the
   trap-site instruction and hypothesised the ambient state was safe. Every one
   failed. An independent RE cross-check turned it around: hook AFTER the
   trap. If PC reaches 0x2d4f46, the trapping instruction at 0x2d4f38
   (`ldr r3,[r2,#0x0]` with r2=0x401d5044) has JUST EXECUTED SUCCESSFULLY in this
   very same dynamic call instance. Container-readability is now a PROOF of
   the moment, not a hypothesis about the ambient. r2 is preserved
   (unmodified since 0x2d4f36 up to 0x2d4f46) so we can just deref [r2+0x18]
   in our C hook and it will succeed — because we are, by construction, inside
   a call that already reached this instruction, i.e., inside a real fw drain.

   Wired via BPatch on 0x2d4f46, displacing the 4-byte Thumb-2-wide
   `ldr.w r4,[r2,#0x164]` (bytes D2 F8 64 41). Trampoline saves caller-saveds,
   calls sp_c_snapshot_read(r2), pops, re-executes the displaced instruction,
   rejoins at 0x2d4f4a (Thumb bit set).

   Cold-boot safety: SNAP_ARM=0 → hook is a nop that just bumps HOOKN
   (proves BPatch installed — a live counter growing without arming eliminates
   the Build-14/15 "did the hook even fire?" ambiguity that wasted cycles).

   Positive signal: after `wlcmd tx <10-byte>` arms + one drain tick, cap shows
   `snap arm=0 cont=0x401d5044 w18=<nonzero> reads=1 err=0 hookn=<large>` with
   EMPTY dmesg trap. That IS a container-readable success — the exact word
   [container+0x18] that Build-7 needed at 0x283978, captured live in patch
   RAM from a proven-safe context. Build-18 substitutes it into
   wlc_queue_80211_frag at the wall. */
volatile unsigned int spc_snap_tel[10] = { 0x534e5031u, 0,0,0,0,0,0,0,0,0 };  /* 'SNP1' */

/* SP-C Build-18: RUNTIME GATE for the sp_c_bssidx_snap_hook trampoline. When
   spc_snap_gate == 0 the hook is inert (sp_c_snapshot_read early-returns
   without touching spc_snap_tel[] or dereferencing anything). Set to 1 only
   around the actual FP_QUEUE_80211_FRAG injection call (~microseconds), so
   normal STA data flow — which also invokes wlc_scb_get_bssidx — is not
   affected by the hook at all. Fixes: Build-17-A crashed the fw ~1.5 s after
   every STA CONNECT_SUCCESS on an AP under test (SSSR dump in dhd log). Gate removes
   the hook's effect on all non-injection call sites. */
volatile unsigned int spc_snap_gate = 0;

#define SNAP_MAGIC 0
#define SNAP_ARM   1  /* set by spc_tx len==10; auto-cleared on first read      */
#define SNAP_CONT  2  /* last cont ptr observed                                 */
#define SNAP_W18   3  /* THE captured value = [cont+0x18]                       */
#define SNAP_READS 4  /* successful snapshot reads                              */
#define SNAP_ERR   5  /* reserved (was: sanity trip)                            */
#define SNAP_HOOKN 6  /* ALWAYS incremented — proves BPatch fires from drain    */
#define SNAP_SCB   7  /* r1 at hook entry = the scb fw's drain is iterating     */
#define SNAP_CONT_HI 8 /* last cont observed WITH cont >= 0x40000000            */
#define SNAP_SCB_HI  9 /* scb that had cont >= 0x40000000 (the STA scb if any!) */
#define PIN_MAGIC 0
#define PIN_ARM_N 1  /* len==8 invocations                                  */
#define PIN_REL_N 2  /* len==9 invocations                                  */
#define PIN_PRE   3  /* MinResMask read BEFORE the arm write                */
#define PIN_POST  4  /* MinResMask read AFTER the arm/release write         */
#define PIN_RD    5  /* PMU_RES_STATE at arm/release time                   */
#define PIN_ARMED 6  /* 1 iff currently armed                               */
#define PIN_TS    7  /* arm timestamp (hnd_time us) — informational only    */

volatile unsigned int spc_r2_tel[12] = { 0x50525331u, 0,0,0,0,0,0,0,0,0,0,0 };  /* 'PRS1' */
#define R2_MAGIC       0
#define R2_ARM         1   /* set by spc_tx len==7; auto-cleared on read/skip-cap */
#define R2_SAMPS       2   /* passive res_state reads (always safe)              */
#define R2_RDY_CNT     3   /* samples with CORE_RDY_SCAN=1                       */
#define R2_READS       4   /* successful [cont+0x18] derefs (no trap)            */
#define R2_DELAY_HITS  5   /* armed but had to fall through to delay+re-check    */
#define R2_LAST_RS     6   /* last PMU_RES_STATE value                           */
#define R2_LAST_W18    7   /* last [cont+0x18] value read                        */
#define R2_PMU_IDX     8   /* cached PMU coreidx (>=0x16 = si_findcoreidx fail)  */
#define R2_ERR         9   /* 1=findcoreidx failed, 2=no sih/bmac, 3=cont OOR    */
#define R2_UNLOCKED   10   /* reads that only succeeded AFTER the delay path     */
#define R2_AUX_CNT    11   /* samples with CORE_RDY_AUX=1 (free discriminator)   */

void spc_dpc_fn(void *arg);   /* defined after the FP_QUEUE_80211_FRAG block      */

/* ---------------------------------------------------------------------------
   ROM reader. Everything below RAMSTART (ROM 0x1000..0x1DB000) is invisible to
   us: the .bin we disassemble is the RAM image only, so wlc_monitor_promisc_
   enable, wlc_monitor_phy_cal, bcm_iovar_lookup and the rest of the ROM are
   black boxes. The firmware can trivially read its own ROM — it executes from
   it — so the only hard part is a channel out, and we already have one that is
   proven on-device: the cap iovar, re-rendered on every read.

   Each cap read emits one chunk as "ROM <offset> <hex>" and advances an internal
   cursor, so the host needs no way to send an offset in: it just reads until it
   has full coverage, and the offset in each line says where the bytes belong.
   That also makes it immune to dhd's own cap reads stealing a chunk.

   Chunk is deliberately small: bcm_bprintf is bounded by the caller's ioctl
   length, and dhd's own preinit cap buffer may be far smaller than ours, so a
   big chunk risks truncating. 256 bytes -> 512 hex chars keeps the whole cap
   string near 1.2KB. The ROM bytes go last, after " monitor ", so truncation can
   never eat the token the driver strstr()s for.
   --------------------------------------------------------------------------- */
#define SPB_ROMDUMP    0                 /* 1 = dedicated ROM-dump build, do NOT ship */
#define SPB_ROM_START  0x00001000u
#define SPB_ROM_END    0x001DB000u
#define SPB_ROM_CHUNK  256u              /* bytes per cap read */

#if SPB_ROMDUMP
static unsigned int spb_rom_cursor = SPB_ROM_START;

/* Emit at most one chunk, sized to whatever room the CALLER left us.
   This matters: dhd's own preinit cap read uses a buffer around 1KB, and
   overflowing it makes wlc_cap return BCME_BUFTOOSHORT (-14), which fails
   dhd_get_fw_capabilities -> dhd_open -> the interface never comes up at all.
   (Learned the hard way: a fixed 256-byte chunk bricked wifi bring-up entirely.)

   bcmstrbuf is { char *buf; uint size; char *origbuf; uint origsize; }, so +4 is
   the residual size. Requiring >= 4KB means dhd's preinit read never qualifies
   and never gets ROM bytes, while our own 16KB request always does — and if the
   layout guess were wrong, the sanity window simply emits nothing rather than
   overflowing. The residual is reported either way so the real value is visible. */
static void
spb_emit_rom(void *b)
{
    const unsigned char *p;
    unsigned int off, i, room, n;

    room = *(volatile unsigned int *)((unsigned char *)b + 4);
    FP_BCM_BPRINTF(b, "rsz %x ", room);
    if (room < 4096u || room > 0x10000u) return;   /* dhd's small buffer, or not sane */

    n = (room - 1024u) / 2u;                       /* hex costs 2 chars per byte */
    if (n > SPB_ROM_CHUNK) n = SPB_ROM_CHUNK;
    n &= ~3u;
    if (n < 16u) return;

    off = spb_rom_cursor;
    if (off < SPB_ROM_START || off >= SPB_ROM_END) off = SPB_ROM_START;
    if (off + n > SPB_ROM_END) n = SPB_ROM_END - off;

    p = (const unsigned char *)off;
    FP_BCM_BPRINTF(b, "ROM %x %x ", off, n);
    for (i = 0; i < n; i++)
        FP_BCM_BPRINTF(b, "%02x", (unsigned int)p[i]);
    FP_BCM_BPRINTF(b, " ");

    off += n;
    if (off >= SPB_ROM_END) off = SPB_ROM_START;   /* wrap, so a lost read self-heals */
    spb_rom_cursor = off;
}
#else
#define spb_emit_rom(b) ((void)0)
#endif

/* ---------------------------------------------------------------------------
   wlc->mon_info: allocate the monitor-module state this firmware never does.

   Broadcom compiled the monitor module's attach out (every monitor __ramfnptr
   points at hnd_unimpl), so wlc+0x41C stays NULL — and the ROM shows exactly
   what that costs:

     wlc_mac_promisc(wlc)                       [ROM 0xA47A8] is THE MCTL recompute
         bits = wlc_monitor_get_mctl_promisc_bits(wlc->mon_info)
         wlc_mctrl(wlc, 0x1C00000, ... | bits)  ; mask = PROMISC|KEEPCTL|KEEPBADFCS
     wlc_monitor_get_mctl_promisc_bits(NULL)    [ROM 0x125B90] returns 0
     wlc_monitor_promisc_enable(NULL, on)       [ROM 0x125D58] returns immediately

   so every recompute is told "no promisc bits" and clears all three. Capture
   works today only because wlc_set_monitor also writes MCTL directly @0x284D22.
   Note wlc_mac_promisc does NOT consult pub->_mon, so mon_info alone is enough.

   Layout, read from the ROM (NOT nexmon's bcm4375b1 layout, where wlc sits at
   +0x08 behind two buffer pointers — porting that struct by hand would have
   written the wrong fields into a ROM-consumed structure):

     +0x00  struct wlc_info *wlc
     +0x04  phy-cal timer
     +0x08  scratch buffer, >= 0x20 bytes (wlc_monitor_phy_he_config runs two
            0x10-byte iovar_op's at buf and buf+0x10)
     +0x0c  promisc flags: bit0 PROMISC, bit1 KEEPBADFCS, bit2 KEEPCONTROL
     +0x14  timer_active (u8)   +0x15/+0x16 saved HE fld_chk   +0x18 chanspec (u16)

   Statics rather than a heap allocation: nothing in the whole image (ROM+RAM)
   ever stores to wlc+0x41C, so mon_info is never freed or replaced and cannot be
   MFREE'd out from under us. Non-zero initialisers keep them in .data. Two slots
   because this chip is RSDB; beyond that we leave mon_info NULL, i.e. today's
   behaviour. Left at zero on purpose: the promisc flags, so the firmware owns
   them (it sets 3 on enable and clears them on disable).
   --------------------------------------------------------------------------- */
#define SPB_WLC_MON_INFO 0x41C
#define FP_WL_INIT_TIMER ((void *(*)(void *, void *, void *))(0x0027fd54u | 1u))
#define FP_MON_PHY_CAL_TIMER ((void *)(0x00125b5cu | 1u))   /* cb takes wlc */

static unsigned int spb_mi[2][8]   = { {1,0,0,0,0,0,0,0}, {1,0,0,0,0,0,0,0} };
static unsigned int spb_mbuf[2][8] = { {1,0,0,0,0,0,0,0}, {1,0,0,0,0,0,0,0} };
static unsigned int spb_mi_used    = 0;

void
spb_mon_attach(void *wlc)
{
    void **slot;
    unsigned int *mi;
    unsigned int i, j;
    void *wl, *timer;

    if (!wlc) return;
    slot = (void **)((unsigned char *)wlc + SPB_WLC_MON_INFO);
    if (*slot) return;                       /* already attached */
    if (spb_mi_used >= 2) return;            /* pool exhausted */
    wl = *(void **)((unsigned char *)wlc + 0x08);
    if (!wl) return;

    i = spb_mi_used;
    for (j = 0; j < 8; j++) { spb_mi[i][j] = 0; spb_mbuf[i][j] = 0; }

    /* The timer must be real. wlc_monitor_phy_cal_timer_start (reached from
       beacon processing) calls wl_add_timer straight through, and wl_add_timer
       does NOT null-check its timer — unlike wl_del_timer, which does. If we
       cannot create one, leave mon_info NULL: today's known-good behaviour beats
       a half-built struct the ROM will dereference. */
    timer = FP_WL_INIT_TIMER(wl, FP_MON_PHY_CAL_TIMER, wlc);
    if (!timer) return;

    mi = spb_mi[i];
    mi[0] = (unsigned int)wlc;               /* +0x00 wlc     */
    mi[1] = (unsigned int)timer;             /* +0x04 timer   */
    mi[2] = (unsigned int)spb_mbuf[i];       /* +0x08 scratch */

    spb_mi_used = i + 1;
    *slot = (void *)mi;
    spb_tel[SPB_T_MONAT] = i + 1;
}

void
cap_add_monitor(void *wlc, void *b)
{
#if !SPC_STA_INJECT
    spb_mon_attach(wlc);
#endif
    FP_WLC_CAP_BCMSTRBUF(wlc, b);     /* original cap tokens */
#if !SPC_STA_INJECT
    FP_BCM_BPRINTF(b, "monitor ");    /* append the monitor token the driver strstr()s for */
#endif
    /* Telemetry last, so a short cap buffer can never truncate " monitor ". */
    FP_BCM_BPRINTF(b, "spb %x %x %x %x %x %x %x %x %x %x ",
                   spb_tel[SPB_T_HOOK], spb_tel[SPB_T_MON], spb_tel[SPB_T_UP],
                   spb_tel[SPB_T_SKIP], spb_tel[SPB_T_CS], spb_tel[SPB_T_LEN],
                   spb_tel[SPB_T_ROOM], spb_tel[SPB_T_RXCPL],
                   spb_tel[SPB_T_MCODE], spb_tel[SPB_T_CMPL]);
    /* Firmware resource gauges, to find what runs dry when monitor RX stalls
       (~150 frames per firmware load, wlc_recv stops being called at all).
       All plain globals, read here rather than on the RX path so this costs
       nothing per frame. g_rxcplid_list layout from bcm_alloc_rxcplinfo
       @0x2F4784: [+4] = free count, [+0x10]/[+0x18] = the counters it bumps
       when the pool is dry. */
    {
        volatile unsigned int *L = *(volatile unsigned int * volatile *)0x002001c8u;
    /* Live state at the moment of the read, to tell "the radio stopped" from
       "monitor turned itself off". wlc->monitor is the gate our RX hook tests;
       the counter block is wlc->pub->[0x68], whose fields wlc_recv itself bumps
       on the RX error paths (+0x60 runt/len, +0x70 bad-addr, +0xe4, +0x30c). If
       those keep climbing while our hook count is frozen, the MAC is still
       receiving and something upstream of wlc_recv is eating the frames. */
    {
        unsigned int mon = *(volatile unsigned int *)((unsigned char *)wlc + 0x120);
        volatile unsigned int *c = 0;
        void *pub = *(void * volatile *)wlc;
        if (pub) c = *(volatile unsigned int * volatile *)((unsigned char *)pub + 0x68);
        {
            unsigned int *mi = *(unsigned int * volatile *)((unsigned char *)wlc + SPB_WLC_MON_INFO);
            FP_BCM_BPRINTF(b, "mi %x flags %x ", (unsigned int)mi, mi ? mi[3] : 0xffffffffu);
        }
        FP_BCM_BPRINTF(b, "live mon=%x cnt %x %x %x %x ", mon,
                       c ? c[0x60/4] : 0xffffffffu, c ? c[0x70/4] : 0u,
                       c ? c[0xe4/4] : 0u, c ? c[0x30c/4] : 0u);
    }
        FP_BCM_BPRINTF(b, "pool %x %x %x %x %x %x ",
                       L ? L[1] : 0xffffffffu,
                       L ? L[4] : 0u,
                       L ? L[6] : 0u,
                       *(volatile unsigned int *)0x002001dcu,   /* lbuf_allocfail          */
                       *(volatile unsigned int *)0x002001e0u,   /* pktget_failed_but_alloc */
                       *(volatile unsigned int *)0x002001e4u);  /* pktget_failed_not_alloc */
    }
    /* SP-C diagnostic block — GATED on the caller's remaining bcmstrbuf size
       (`*(b+4)` = residual bytes, layout confirmed in spb_emit_rom's comment).
       Build-12B (~1140-byte cap) broke wlan0 bring-up because dhd's preinit
       cap buffer is ~1024 bytes → wlc_cap returned BCME_BUFTOOSHORT →
       dhd_get_fw_capabilities failed → dhd_open failed → interface never up.
       This gate uses the same self-tuning residual pattern spb_emit_rom uses
       for ROM chunks. Threshold empirically calibrated on-device (Build-13B):
       wlcmd's `getstr cap` allocs a **2 KB** buffer (buflen=2048 reported by
       wlcmd), and the stock cap tokens + SP-B block emit ~700-800 B before
       we reach here, leaving residual ~1200-1300 B for wlcmd. dhd's preinit
       ~1 KB buffer has residual ~200 B at the same point. Threshold 1024 B
       lets wlcmd through while excluding dhd. `rsz` diagnostic emitted
       OUTSIDE the gate exposes the actual residual so future builds don't
       have to guess. Every future SP-C diagnostic line goes inside this
       gate — never re-tune the threshold per-build. */
    FP_BCM_BPRINTF(b, "rsz %x ",
                   *(volatile unsigned int *)((unsigned char *)b + 4));
    if (*(volatile unsigned int *)((unsigned char *)b + 4) >= 1024u) {
    /* SP-C Build-17-A: snapshot state FIRST (never let truncation hide it).
       hookn climbs even with arm=0 (proves BPatch live). arm=1 after len==10
       iovar; auto-clears after the deref lands. reads>0 + cont=0x401d5044 +
       w18=<nonzero> + err=0 + empty dmesg = FIRST POSITIVE RESULT on SP-C. */
    FP_BCM_BPRINTF(b, "snap arm=%x cont=%x scb=%x w18=%x reads=%x hookn=%x hi=%x/%x ",
                   spc_snap_tel[SNAP_ARM],   spc_snap_tel[SNAP_CONT],
                   spc_snap_tel[SNAP_SCB],   spc_snap_tel[SNAP_W18],
                   spc_snap_tel[SNAP_READS], spc_snap_tel[SNAP_HOOKN],
                   spc_snap_tel[SNAP_CONT_HI], spc_snap_tel[SNAP_SCB_HI]);
    /* SP-C Build-16 (2026-08-09): emission order REVERSED — the LOAD-BEARING
       diagnostic lines (r2, r2x, pin) go FIRST so that even if bcm_bprintf
       runs out of buffer during the trailing SR/spc/spcp/spc2/sphf/srp lines,
       we still see the numbers that answer "did the pin work" and "did the
       AUX-gated deref succeed". Build-14/15 emissions were LAST and got
       silently truncated after a very long `srp` line consumed ~180 bytes. */
    FP_BCM_BPRINTF(b, "r2 rs=%x rdy=%x reads=%x delay_hits=%x ",
                   spc_r2_tel[R2_LAST_RS], spc_r2_tel[R2_RDY_CNT],
                   spc_r2_tel[R2_READS],   spc_r2_tel[R2_DELAY_HITS]);
    FP_BCM_BPRINTF(b, "r2x samps=%x unlk=%x aux=%x idx=%x err=%x w18=%x arm=%x ",
                   spc_r2_tel[R2_SAMPS],   spc_r2_tel[R2_UNLOCKED],
                   spc_r2_tel[R2_AUX_CNT], spc_r2_tel[R2_PMU_IDX],
                   spc_r2_tel[R2_ERR],     spc_r2_tel[R2_LAST_W18],
                   spc_r2_tel[R2_ARM]);
    FP_BCM_BPRINTF(b, "pin arm=%x rel=%x pre=%x post=%x rd=%x armed=%x ts=%x ",
                   spc_pin_tel[PIN_ARM_N], spc_pin_tel[PIN_REL_N],
                   spc_pin_tel[PIN_PRE],   spc_pin_tel[PIN_POST],
                   spc_pin_tel[PIN_RD],    spc_pin_tel[PIN_ARMED],
                   spc_pin_tel[PIN_TS]);
    /* Original SP-C emissions (lower priority — truncated if buffer runs out). */
    FP_BCM_BPRINTF(b, "spc %x %x %x bss %x scb %x cont %x first %x cf %x txq %x pkt %x q %x ",
                   spc_tel[SPC_T_HOOK], spc_tel[SPC_T_LEN], spc_tel[SPC_T_FB],
                   spc_tel[SPC_T_BSSCFG], spc_tel[SPC_T_SCB], spc_tel[SPC_T_ALLOC],
                   spc_tel[SPC_T_DTXP], spc_tel[SPC_T_G6C],
                   spc_tel[SPC_T_TXQ], spc_tel[SPC_T_PKT], spc_tel[SPC_T_DMA]);
    /* SP-C Build-1 guarded-probe result: ok=c0ffee => 0x401d5044 read OK from the
       wl-DPC context; req/n = armed/fired; cont/w0/w4 = container ptr + words. */
    FP_BCM_BPRINTF(b, "spcp req %x n %x cont %x w0 %x w4 %x ok %x ",
                   spc_probe[SPCP_REQ], spc_probe[SPCP_N], spc_probe[SPCP_CONT],
                   spc_probe[SPCP_W0], spc_probe[SPCP_W4], spc_probe[SPCP_OK]);
    /* SP-C Build-2: DPC (wl-task) results. dpc = registered obj, task = wl task.
       mode 1: cont/w0/w4 = *(STA_wlc+0x50)=0x401d5044 read in the wl-task ctx,
       ok=c0ffee if it read. mode 2 (Fix A submit): cont=*(scb+0), w0=pkt, w4=
       queue_80211_frag return, ok=5b117 if the submit was reached. */
    FP_BCM_BPRINTF(b, "spc2 dpc %x task %x mode %x n %x wlc %x cont %x w0 %x w4 %x ok %x ",
                   spc_dpc_obj, spc_wl_task,
                   spc_probe2[SPC2_MODE], spc_probe2[SPC2_N], spc_probe2[SPC2_WLC],
                   spc_probe2[SPC2_CONT], spc_probe2[SPC2_W0], spc_probe2[SPC2_W4],
                   spc_probe2[SPC2_OK]);
    /* SP-C Build-8: force-ht diag. cont = *(scb+0) = 0x401d5044; w18 = [cont+0x18]
       read with the clock forced (the exact 0x283978 fault). Non-zero w18 = the
       root-cause fix made the container readable from the ioctl thread. */
    FP_BCM_BPRINTF(b, "sphf cont %x w18 %x ", spc_hf[1], spc_hf[2]);
    /* SP-C Build-11: SR (save/restore) power state diagnostic.
         probe=N          count of `wlcmd wlan0 spc_tx <3-byte>` snapshots taken
         wlc/bmac/sih     resolved chain at probe time
         ctrl=X           raw SR ctrl reg (bits 8..15 = requested domains)
         stat=X           SR stat (bits 16..23 = powered domains; 0x100000 = scan core)
         rxN=N            RX-hook samples (from the wl_dpc interrupt context)
         last=X           most recent RX-time stat
         min/max          bounds over the RX sample stream
         or/and=X         cumulative OR/AND — "any domain ever on" / "always on"
         sc_up=N          count of RX samples with the scan-core bit (0x10) SET. */
    FP_BCM_BPRINTF(b, "srp probe=%x wlc=%x bmac=%x sih=%x ctrl=%x stat=%x "
                      "rxN=%x last=%x min=%x max=%x or=%x and=%x sc_up=%x ",
                   spc_sr_tel[SR_T_PROBE_N], spc_sr_tel[SR_T_WLC],
                   spc_sr_tel[SR_T_BMAC],    spc_sr_tel[SR_T_SIH],
                   spc_sr_tel[SR_T_CTRL],    spc_sr_tel[SR_T_STAT],
                   spc_sr_tel[SR_T_RX_N],    spc_sr_tel[SR_T_LAST],
                   spc_sr_tel[SR_T_MIN],     spc_sr_tel[SR_T_MAX],
                   spc_sr_tel[SR_T_STAT_OR], spc_sr_tel[SR_T_STAT_AND],
                   spc_sr_tel[SR_T_SC_UP]);
    /* SP-C Build-13B: wl_sc_recv-hook autonomous read of [cont+0x18].
       n=hook-entries; cont=last observed *(scb+0); last=[cont+0x18] read;
       reads=successful derefs; miss=attempts where cont was outside the
       0x40000000+ window (stale mirror after roam). sc_state=SC state ptr
       captured on first entry (Path A prep). reads growing WITHOUT a trap
       ⇒ the SC-DPC context IS where the container reads fine ⇒ Build-14
       replaces the read with wlc_queue_80211_frag under the same hook.
       reads=0 with a fw trap ⇒ FINDINGS-E was wrong; back to Path A design. */
    FP_BCM_BPRINTF(b, "dpc n=%x cont=%x last=%x reads=%x miss=%x sc_state=%x ",
                   spc_dpc_tel[DPC_T_ATTEMPT], spc_dpc_tel[DPC_T_CONT],
                   spc_dpc_tel[DPC_T_LAST],    spc_dpc_tel[DPC_T_READS],
                   spc_dpc_tel[DPC_T_MISS],    spc_sc_state);
    /* SP-C Build-13B-part2: wlc_bmac_srpwr_request_on hit count + last bmac. */
    FP_BCM_BPRINTF(b, "srp2 n=%x bmac=%x ",
                   spc_srp_tel[SRP_T_HOOK], spc_srp_tel[SRP_T_BMAC]);
    /* Build-16: r2, r2x, pin emissions moved to the top of the block. */
    }
    spb_emit_rom(b);
}

__attribute__((at(0x2828F4, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BLPatch(cap_monitor_blhook, cap_add_monitor);

/* Attach mon_info inside wlc_set_monitor, not only on a cap read.
   Doing it from cap_add_monitor alone is too late and the telemetry proved it:
   this chip is RSDB, so there is more than one wlc, and WLC_SET_MONITOR landed on
   a wlc whose first cap read had not happened yet — wlc->mon_info was still NULL,
   wlc_monitor_promisc_enable(NULL) returned immediately, and the promisc flags
   word stayed 0 (mi 3a41f8 flags 0) even with monitor on.

   Hook 0x284CD6, `str.w r5,[r4,#0x120]` (wlc->monitor = val), which sits after
   both capability/state gates and before wlc_monitor_promisc_enable @0x284D0C —
   so the struct always exists by the time the ROM dereferences it, on whichever
   wlc is actually being switched. wlc is in r4 there, hence a trampoline rather
   than a BLPatch. 0x284CD6 is itself a branch target (cbz @0x284CCA and
   @0x284CCE) which is fine — it stays the first byte of the b.w — and nothing
   targets 0x284CD8/9. r12 is unused anywhere in wlc_set_monitor. */
__attribute__((naked))
void
sp_b_setmon_attach(void)
{
    asm(
        "push {r0-r3, r12, lr}\n"
        "mov  r0, r4\n"                /* wlc */
        "bl   spb_mon_attach\n"
        "pop  {r0-r3, r12, lr}\n"
        "str.w r5, [r4, #0x120]\n"     /* displaced instruction */
        "movw r12, #0x4CDB\n"          /* 0x284CDA|1 */
        "movt r12, #0x0028\n"
        "bx   r12\n"
    );
}

__attribute__((at(0x284CD6, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BPatch(sp_b_setmon_attach, sp_b_setmon_attach);

// ==========================================================================
// SP-B Part B: deliver received 802.11 frames to the host monitor netdev with
// a radiotap header. Ported from nexmon bcm4375b1 monitormode.c to 4389c1.
//
// HOOK POINT (re-derived 2026-07-29 — the previous one was too late).
// The first attempt hooked `bl wlc_recvdata` @0x3021F4 and never fired: that
// call only sees DATA frames of our own BSS. wlc_recv dispatches by frame type
// well before it — beacons/mgmt/ctl from foreign BSSs go to wlc_recv_mgmt_ctl
// @0x302324, amsdu to wlc_recvamsdu @0x3022F0, runts/errors to the free path
// @0x302160. nexmon's bcm4375b1 reference solves this by hooking an rssi-compute
// call *earlier* in wlc_recv (4375b1 @0x2170DA, p=r4/wlc=r5) — before filtering.
//
// The 4389c1 analogue of that call is `bl wlc_rxsig_compute_rssi_snr` @0x302184
// (r0=wlc_hw, r1=wrxh, r2=&rxsig) — but this compiler put it on only ONE of the
// two rx-header paths: `cmp r1,#2` @0x30213A splits on wrxh->rxhdr.dma_flags,
// and the !=2 path (@0x302194, zeroes rssi) jumps straight past it. Hooking
// there would silently miss half the frames.
//
// So we hook the CONVERGENCE point instead — 0x3021B2, where both paths rejoin
// and every surviving frame passes, still before the type dispatch:
//     0x3021B0  add  r3, r7      <- branch target of `bpl` @0x30218E (kept intact)
//     0x3021B2  cmp  r3, #7      <- 4 bytes replaced by `b.w sp_b_rx_trampoline`
//     0x3021B4  bgt  0x3021C2
//     0x3021B6  ldr  r3, [r5]
// Verified by disassembly of wlc_recv @0x3020BC:
//   r4 = p (sk_buff, from arg1 @0x3020D2; never rewritten)
//   r5 = wlc          (arg0 @0x3020C6; never rewritten)
//   r6 = wrxh         (= original p->data @0x3020D4; only clobbered as an error
//                      code on drop paths, all of which branch to 0x302160)
//   r10 = p->data — PLCP(6) + the 802.11 header (fw itself reads fc @sl+6 and
//                      addr2 @sl+0x10), r3 = frame-length accumulator.
// Cross-check vs 4375b1: same p=r4 / wlc=r5 allocation. And unlike 4375b1's
// point, rssi is already computed here, so wrxh->rssi is valid for radiotap.
// A whole-image scan (both alignments) found ZERO branches into 0x3021B2..B5.
// Early exit target = 0x3022F4, wlc_recv's common tail (stack-canary check then
// return) — the same place the wlc_recvdata / wlc_recvamsdu / wlc_recv_mgmt_ctl
// paths land after handing the packet off. r8 (canary global) and sp are intact.
//
// When monitor is off the hook reads wlc->monitor and returns 0 immediately, so
// wlc_recv proceeds exactly as stock => zero STA regression. Progress is
// readable from the host at any time via the cap iovar (see spb_tel above);
// the firmware console is not readable on this device.
// ==========================================================================

/* 1 = prepend radiotap and hand the frame to the host (normal operation).
   0 = receive and count only, never deliver — the diagnostic mode that proved
   the RX stall was nothing to do with delivery (it stalled identically with
   delivery off, which is what pointed at MPC). */
#define SPB_DELIVER   1

#define SPB_MON_OFF   0x120     /* wlc->monitor (u32) */
#define SPB_WL_OFF    0x08      /* wlc->wl */
#define SPB_LB_DATA   0x08      /* lbuf->data (void*) */
#define SPB_LB_LEN    0x0C      /* lbuf->len  (u16) */
#define SPB_LB_RXFL   0x17      /* lbuf byte wl_sendup passes as arg4 */

/* radiotap: version(1) pad(1) len(2) present(4) chan_freq(2) chan_flags(2)
   antsignal(1) antnoise(1) = 14, every field naturally aligned. */
#define SPB_RT_LEN     14
#define SPB_RT_PRESENT (((unsigned)1<<3)|((unsigned)1<<5)|((unsigned)1<<6))
#define SPB_PLCP_LEN   6        /* see below */
#define SPB_CH_2GHZ   0x0080u
#define SPB_CH_5GHZ   0x0100u
#define SPB_CH_OFDM   0x0040u
#define SPB_CH_DYN    0x0400u

#define FP_WF_CHANNEL2MHZ ((unsigned short (*)(unsigned int, unsigned int))(0x00212e52u | 1u))
/* Host delivery. NOT 0x280420 — that address was mis-identified earlier: the map
   calls it wl_sendup_event (`movs r3,#1; b wl_sendup`), and __ramfnptr_wl_sendup_
   monitor @0x2102B8 points at hnd_unimpl, i.e. this fw has no monitor sendup at
   all. wl_sendup_rxcmpl @0x2F8388 is the real choke point both wl_sendup and
   wl_sendup_fp funnel into; calling it directly also skips wl_sendup's 802.3
   snooping (it reads an ethertype at data+12 and runs ARP/ND offload), which
   would misread a radiotap frame. Call shape copied verbatim from wl_sendup's
   own tail call @0x2F85AC: (wl, wlcif, p, *(u8*)(p+0x17)). It takes ownership
   of p — wl_sendup never frees p after calling it. */
#define FP_WL_SENDUP_RXCMPL \
    ((void (*)(void *, void *, void *, unsigned int))(0x002f8388u | 1u))

/* rxcplid of the frame, recorded for diagnostics only. An earlier build tried
   to set the 802.11 bit via bcm_id2rxcplinfo(p+0x14)->[3] |= 2; that never
   reached the host, and bcm_id2rxcplinfo @0x2F4854 turns out to be an
   unchecked `base + 0x18*id` with no bounds test, so the write could land
   anywhere past the pool. The completion flag is set at its assembly point
   instead (see sp_b_rxcpl_802_11 at the end of this file). */
#define SPB_LB_RXCPLID  0x14

/* Call the ROM printf @0xaf70 by address, NOT via nexmon's `printf` wrapper:
   patches/common/wrapper.c only relocates that symbol for chip/fw pairs it ships
   an at() entry for, and FW_VER_20_101_90_r1098710 is one we add ourselves — so
   the wrapper would silently link a `return 0` dummy. The console is not readable
   on this device, but dhd dumps the fw console buffer on a trap, so these lines
   are the post-mortem if the hot path ever faults. */
#define FP_PRINTF ((int (*)(const char *, ...))(0x0000af70u | 1u))

#if SPB_DELIVER
static int spb_dbg_mon = 8;     /* non-zero init => .data, not .bss */
#endif

/* Returns 1 to consume the frame (wlc_recv returns early), 0 to continue.
   Live state at the call site: p->data points at a 6-byte PLCP prefix followed
   by the 802.11 header — wlc_recv itself reads the frame-control at p->data+6
   (@0x302302) and addr2 at p->data+0x10 (@0x302210), and nexmon's bcm4375b1
   port uses the same 6. Rather than pull those 6 bytes we let the radiotap
   it_len cover them (14+6), exactly as the 4375b1 reference does, so the
   consumer skips straight to the frame control. */
int
sp_b_monitor_rx(void *wlc, void *wrxh, void *p)
{
    unsigned int mon, room;
    void *wl;
    unsigned char *d;
    unsigned short len, chanspec;
#if SPB_DELIVER
    unsigned short freq, flags;
    unsigned char ch;
#endif

    if (!wlc || !wrxh || !p) return 0;

    spb_tel[SPB_T_HOOK]++;

    /* SP-C Build-2: capture the wl-task handle + register the SP-C DPC ONCE.
       This hook is inside wlc_recv, which runs on the wl task, so
       osl_ext_task_current() here IS the wl task (the TX-drain context where
       0x401d5044 is readable). spc_wl_task stays the sentinel 1 until BOTH the
       task handle and the DPC object are obtained, so it self-retries. */
    if (spc_wl_task == 1u) {
        unsigned int t = (unsigned int)FP_OSL_EXT_TASK_CURRENT();
        if (t) {
            unsigned int d = (unsigned int)FP_HND_DPC_REGISTER(
                (void *)((unsigned int)spc_dpc_fn | 1u), 0, (void *)t);
            if (d) { spc_dpc_obj = d; spc_wl_task = t; }
        }
    }

    /* SP-C Build-9 (READ-ONLY, rig-independent): this hook runs inside wlc_recv in
       the REAL wlc_dpc interrupt context — the same context where normal TX reads
       0x401d5044. Armed by an spc_tx of length 2, read the STA AP scb's container
       [*(scb+0)+0x18] (the exact addr that faults from the ioctl thread, 0x283978)
       HERE. If it reads (spc_hf[2] populated, no trap), the fix is to run the whole
       submit from this context. Pure read — nothing is transmitted. */
    if (spc_hf[3] == 1u &&
        spc_job_scb >= 0x200000u && spc_job_scb < 0x500000u) {   /* spc_ram, inlined (defined later) */
        unsigned int cont = *(volatile unsigned int *)(spc_job_scb + 0);
        spc_hf[1] = cont;                                            /* expect 0x401d5044 */
        spc_hf[2] = *(volatile unsigned int *)(cont + 0x18);         /* the faulting read  */
        spc_hf[3] = 0u;
    }

    /* SP-C Build-11: sample the SR power stat on every RX (wl_dpc interrupt ctx).
       Answers "does the scan-core SR domain (bit 0x10) EVER assert during normal
       STA/monitor operation?". Cost per frame: one MMIO read + a handful of stores;
       all in-range pointer chases guarded. Never writes any SR bit — cannot UDF. */
    {
        unsigned int bmac_ = *(volatile unsigned int *)((unsigned char *)wlc + SPC_WLC_BMAC_OFF);
        if (bmac_ >= 0x200000u && bmac_ < 0x500000u) {                       /* spc_ram inline */
            void *sih_ = *(void * volatile *)((unsigned char *)bmac_ + SPC_BMAC_SIH_OFF);
            if (sih_) {
                unsigned int stat = FP_SI_SRPWR_STAT(sih_);
                spc_sr_tel[SR_T_LAST] = stat;
                if (spc_sr_tel[SR_T_RX_N] == 0) {
                    spc_sr_tel[SR_T_MIN]      = stat;
                    spc_sr_tel[SR_T_STAT_OR]  = stat;
                    spc_sr_tel[SR_T_STAT_AND] = stat;
                } else {
                    if (stat < spc_sr_tel[SR_T_MIN]) spc_sr_tel[SR_T_MIN] = stat;
                    spc_sr_tel[SR_T_STAT_OR]  |= stat;
                    spc_sr_tel[SR_T_STAT_AND] &= stat;
                }
                if (stat > spc_sr_tel[SR_T_MAX]) spc_sr_tel[SR_T_MAX] = stat;
                if (stat & SPC_SR_SCAN_CORE)     spc_sr_tel[SR_T_SC_UP]++;
                spc_sr_tel[SR_T_RX_N]++;
            }
        }
    }

    mon = *(volatile unsigned int *)((unsigned char *)wlc + SPB_MON_OFF);
    /* Drives the pciedev completion-flag patch below. Set on EVERY hook entry,
       including the monitor-off case, so it self-clears on the first frame
       after monitor is turned off — and it is set before this frame's own
       completion is built, so it always describes the frame in flight. */
    spb_tel[SPB_T_ACTIVE] = mon ? 1u : 0u;
    if (mon == 0) return 0;

    /* SP-C Build-1: one-shot container read from THIS (wl-DPC RX, radio-active)
       context. If 0x401d5044 is readable here, the fix's premise holds; if it
       faults, the dongle traps and dhd's dmesg shows the epc + fault address. */
    if (spc_probe[SPCP_REQ]) {
        unsigned int cont = *(volatile unsigned int *)((unsigned char *)wlc + 0x50);
        spc_probe[SPCP_REQ] = 0;
        spc_probe[SPCP_N]++;
        spc_probe[SPCP_CONT] = cont;
        if (cont) {
            spc_probe[SPCP_W0] = *(volatile unsigned int *)cont;         /* faults from ioctl ctx */
            spc_probe[SPCP_W4] = *(volatile unsigned int *)(cont + 4);
            spc_probe[SPCP_OK] = 0x00C0FFEEu;
        }
    }

    spb_tel[SPB_T_MON]++;

    wl  = *(void **)((unsigned char *)wlc + SPB_WL_OFF);
    d   = *(unsigned char **)((unsigned char *)p + SPB_LB_DATA);
    len = *(unsigned short *)((unsigned char *)p + SPB_LB_LEN);

    /* wrxh IS the lbuf's original data pointer (wlc_recv: `ldr r6,[r1,#8]`
       @0x3020D4 before it pulls the rx header), so d-wrxh is exactly the
       headroom wlc_recv already consumed — a free, exact bounds check on the
       skb_push below instead of assuming the rx header is big enough. */
    room = (unsigned int)(d - (unsigned char *)wrxh);

    chanspec = *(volatile unsigned short *)((unsigned char *)wrxh + 0x24);
    spb_tel[SPB_T_CS]   = (unsigned int)chanspec |
                          ((unsigned int)*((unsigned char *)wrxh + 0x0e) << 16);
    spb_tel[SPB_T_LEN]  = (unsigned int)len |
                          ((unsigned int)(d && len > SPB_PLCP_LEN ? d[SPB_PLCP_LEN] : 0) << 16);
    spb_tel[SPB_T_ROOM] = (room & 0xffffu) | (mon << 16);

    if (!wl || !d || len <= SPB_PLCP_LEN || room < SPB_RT_LEN) {
        spb_tel[SPB_T_SKIP]++;
        return 0;               /* not deliverable — let the frame run on */
    }

#if !SPB_DELIVER
    spb_tel[SPB_T_SKIP]++;
    return 0;                   /* observe-only build */
#else
    if (spb_dbg_mon > 0) {
        spb_dbg_mon--;
        FP_PRINTF("sp_b: MON len=%x room=%x cs=%x fc=%x\n",
                  (unsigned int)len, room, (unsigned int)chanspec,
                  (unsigned int)d[SPB_PLCP_LEN]);
    }

    ch = (unsigned char)(chanspec & 0xff);
    if (ch == 0) ch = 1;
    if (ch <= 14) { freq = FP_WF_CHANNEL2MHZ(ch, 4814);  flags = SPB_CH_2GHZ | SPB_CH_DYN; }
    else          { freq = FP_WF_CHANNEL2MHZ(ch, 10000); flags = SPB_CH_5GHZ | SPB_CH_OFDM; }

    /* skb_push(p, 14) — headroom proven above */
    d -= SPB_RT_LEN;
    d[0] = 0;                                             /* it_version */
    d[1] = 0;                                             /* it_pad     */
    *(unsigned short *)(d + 2) = SPB_RT_LEN + SPB_PLCP_LEN;   /* it_len  */
    *(unsigned int   *)(d + 4) = SPB_RT_PRESENT;
    *(unsigned short *)(d + 8) = freq;
    *(unsigned short *)(d + 10) = flags;
    d[12] = (unsigned char)*(signed char *)((unsigned char *)wrxh + 0x04);  /* rssi */
    d[13] = (unsigned char)(signed char)-96;                                /* noise */

    *(unsigned char **)((unsigned char *)p + SPB_LB_DATA) = d;
    *(unsigned short *)((unsigned char *)p + SPB_LB_LEN) =
        (unsigned short)(len + SPB_RT_LEN);

    spb_tel[SPB_T_RXCPL] =
        (unsigned int)*(unsigned short *)((unsigned char *)p + SPB_LB_RXCPLID);

    spb_tel[SPB_T_UP]++;
    FP_WL_SENDUP_RXCMPL(wl, 0, p, *(unsigned char *)((unsigned char *)p + SPB_LB_RXFL));
    return 1;                   /* consumed: wl_sendup_rxcmpl owns p now */
#endif
}

/* Trampoline installed over wlc_recv+0xF6 (0x3021B2). Saves the caller-saved
   registers wlc_recv still needs (r3 holds the length accumulator), calls the C
   hook with wlc/wrxh/p out of r5/r6/r4, then either exits wlc_recv early or
   re-executes the two instructions the 4-byte b.w displaced. POP does not touch
   the flags, so the `cmp r0,#0` verdict survives the register restore.

   Returns to wlc_recv with movw/movt + bx rather than `b.w <absolute>`: an
   absolute numeric branch target is a compile-time-resolved expression, so the
   assembler measures the range from offset 0 in the object — long before the
   linker moves this code to PATCHSTART — and rejects it as out of range (clang's
   integrated assembler does exactly that; GNU as only survives it by deferring
   to a relocation). movw/movt has no range or relocation to get wrong. Scratch
   register is r12: verified unused anywhere in wlc_recv, and it is caller-saved
   so the C hook may clobber it freely. Targets carry bit0 set (Thumb). */
__attribute__((naked))
void
sp_b_rx_trampoline(void)
{
    asm(
        "push {r0-r3, r12, lr}\n"   /* 24B keeps sp 8-byte aligned            */
        "mov  r0, r5\n"             /* wlc                                    */
        "mov  r1, r6\n"             /* wrxh (wlc_d11rxhdr)                    */
        "mov  r2, r4\n"             /* p (sk_buff)                            */
        "bl   sp_b_monitor_rx\n"
        "cmp  r0, #0\n"
        "pop  {r0-r3, r12, lr}\n"
        "bne  2f\n"                 /* consumed -> wlc_recv tail             */
        "cmp  r3, #7\n"             /* displaced instruction                 */
        "bgt  1f\n"                 /* displaced instruction                 */
        "movw r12, #0x21B7\n"       /* 0x3021B6|1: rest of wlc_recv          */
        "movt r12, #0x0030\n"
        "bx   r12\n"
        "1:\n"
        "movw r12, #0x21C3\n"       /* 0x3021C2|1: the `bgt` target          */
        "movt r12, #0x0030\n"
        "bx   r12\n"
        "2:\n"
        "movw r12, #0x22F5\n"       /* 0x3022F4|1: canary check + return     */
        "movt r12, #0x0030\n"
        "bx   r12\n"
    );
}

__attribute__((at(0x3021B2, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BPatch(sp_b_rx_trampoline, sp_b_rx_trampoline);

// ==========================================================================
// SP-B Part B, final link: force the D2H rx-completion "frame type" to 802.11
// while monitor RX delivery is live.
//
// dhd_prot_process_msgbuf_rxcpl gates its monitor path on
// BCMPCIE_PKT_FLAGS_FRAME_802_11 (`ldrh w8,[msg,#0x12]; tbnz w8,#1` ->
// dhd_rx_mon_pkt, else "Received non 802.11 packet, when monitor mode is
// enabled"). The firmware picks that value in
// pciedev_data$pciedev_queue_rxcomplete_msgring:
//     0x234B96  tst.w r1, #2            ; r1 = rxcplinfo->[3]
//     0x234B9A  and   r2, r3, #3
//     0x234B9E  ite   eq
//     0x234BA0  moveq r3, #1            ; BCMPCIE_PKT_FLAGS_FRAME_802_3
//     0x234BA2  movne r3, #2            ; BCMPCIE_PKT_FLAGS_FRAME_802_11
//     0x234BA4  orr.w r3, r3, r2, lsl #8
//     0x234BA8  strh  r3, [r4, #0x12]
// Setting bit 1 of rxcplinfo->[3] from the RX hook was not enough on-device
// (the frames still arrived flagged 802.3), so instead of guessing which
// rxcplinfo instance the pciedev layer ends up using, override the value at
// the point it is assembled — one instruction before the store.
//
// Patch 0x234BA4 (4 bytes, `orr.w r3,r3,r2,lsl#8`; whole-image scan: no branch
// targets 0x234BA4..A7). r0/r1/r12 are all dead here — r0 last written
// @0x234B7A and reloaded @0x234BCA, r1 last read @0x234B96 and reloaded
// @0x234BD6, r12 first written @0x234C0C — so r12 is free as scratch.
// Gate: spb_tel[SPB_T_ACTIVE] (offset 32), which the RX hook refreshes on every
// frame. Zero when monitor is off => the original value is stored untouched
// and the STA path is bit-for-bit unchanged.
//
// The high byte the firmware builds here (`r2 = rxcplinfo->[0x12] & 3`, shifted
// left 8) is dhd's monitor sub-code — dhd_rx_mon_pkt switches on
// `msg[0x13] & 3` with the usual BCMPCIE_PKT_FLAGS_MONITOR_{NO_AMSDU,FIRST,
// INTER,LAST}_PKT = 0/1/2/3 encoding, and anything other than NO_AMSDU puts it
// into multi-buffer reassembly against a pending skb it keeps across frames.
// Every frame we deliver is one complete 802.11 frame in one buffer, and the
// value the firmware would otherwise supply is whatever happens to be in an
// rxcplinfo field, so force the whole halfword to exactly 2: frame type 802.11,
// sub-code NO_AMSDU.
// ==========================================================================
__attribute__((naked))
void
sp_b_rxcpl_802_11(void)
{
    asm(
        "ldr   r0, =spb_tel\n"
        "ldr   r12, [r0, #32]\n"       /* spb_tel[SPB_T_ACTIVE]              */
        "cmp   r12, #0\n"
        "beq   1f\n"
        "str   r2, [r0, #40]\n"        /* spb_tel[SPB_T_MCODE] = fw sub-code */
        "ldr   r1, [r0, #44]\n"
        "add.w r1, r1, #1\n"
        "str   r1, [r0, #44]\n"        /* spb_tel[SPB_T_CMPL]++              */
        "mov.w r3, #2\n"               /* 802.11 frame, monitor sub-code 0   */
        "b     2f\n"
        "1:\n"
        "orr.w r3, r3, r2, lsl #8\n"   /* displaced instruction (unmodified) */
        "2:\n"
        "movw  r12, #0x4BA9\n"         /* 0x234BA8|1: the strh + rest        */
        "movt  r12, #0x0023\n"
        "bx    r12\n"
        ".ltorg\n"
    );
}

__attribute__((at(0x234BA4, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BPatch(sp_b_rxcpl_802_11, sp_b_rxcpl_802_11);

// ==========================================================================
// SP-C: frame injection (TX). Task 2 = prove the host->fw channel ONLY.
//
// Host sends WLC_SET_VAR with buffer "spc_tx\0"+<raw 802.11 frame> (wlcmd tx).
// The VAR path does NOT go through wlc_doioctl on this chip: wlc_ioctl@0x110610
// dispatches through wlc_iocv_dispatch_ioctl (indirect via wlc+0x458), which
// routes iovars to wlc_iovar_op@0x110FF8 -> wlc_iovar_ext. We hook the LIVE RAM
// body of wlc_iovar_ext @0x2B0760 (the ROM @0x11091C is already FP-redirected
// there, so this is a plain RAM patch like every SP-B hook — no new flash-patch
// entry, unlike hooking the ROM wlc_iovar_op would need). Not wlc_doiovar
// @0x9CEB4 either: it switches on the resolved NUMERIC varid, so an unregistered
// name never reaches it (an earlier RE pass caught that trap).
//
// wlc_iovar_ext(wlc=r0, name=r1, params=r2, plen=r3, arg=[sp+0], alen=[sp+4],
// ...). An unregistered name reaches its ENTRY before the table lookup rejects
// it, so our name match fires here. wlc_iovar_ext itself normalises the value
// buffer: `if (params==NULL){ len=alen; params=arg; }` — so the effective SET
// value is `params ? params : arg`, len `params ? plen : alen`. We replicate
// exactly that in the trampoline and hand the handler one (wlc, value, len);
// no guesswork about which buffer holds the frame.
//
// Task 2's handler ECHOES only (len + first 4 bytes into spc_tel, read via
// `wlcmd <if> getstr cap`); nothing is allocated or transmitted. On a name
// match we return BCME_OK (we "handled" the iovar); other iovars fall through
// untouched. Unguarded (no SPC_STA_INJECT #if), so the committed STA+inject image
// runs this. Tasks 3-4 replace the echo body with the real rev80 build + submit.
// Every iovar pays a
// 7-byte name compare — negligible.
// ==========================================================================
/* A firmware-heap/RAM pointer sanity gate, so a garbage value can never fault a
   chained deref in the probe below (reads only; no writes, no calls). */
static int spc_ram(unsigned int p) { return p >= 0x00200000u && p < 0x00500000u; }

/* wlc_bsscfg_bcmcscballoc(wlc, bsscfg) @0xE3688 — allocates a bsscfg's broadcast
   scb (at bsscfg+0x34) AND initialises its band (+0x28) + rateset: the complete
   scb we need. 2-arg convention (r0=wlc, r1=bsscfg) read from its disassembly +
   its call site inside wlc_bsscfg_bcmc_scb_init @0xE4586. We call it directly
   rather than the outer wlc_bsscfg_bcmc_scb_init because that outer function's
   guard (bsscfg[0xc]!=0 && bsscfg[6]==0) skips the alloc for the monitor bsscfg
   (proven on-device: alloc no-op'd, scb stayed NULL). bcmcscballoc's own guard is
   only bit10 of bsscfg[0x6c] (echoed as g6c) and bsscfg[0x34]==0 (our case). */
#define FP_WLC_BCMCSCB_ALLOC ((int (*)(void *, void *))(0x000e3688u | 1u))

/* Task 3b step 2 — the rev80 TX-descriptor build primitives, all with verified
   conventions. hnd_pkt_get(osh,len) @0x160DC allocs an lbuf (type 0);
   wlc_frame_get_mgmt_ex showed the recipe: alloc len+0xd4, then reserve 0xBC of
   front headroom for the TxD before the frame. wlc_lowest_band_rspec(band)
   @0x18A584 gives the band's lowest basic rate (1 Mbps CCK on 2.4, 6 Mbps OFDM
   on 5/6 GHz) — keeps this band-agnostic. push_txparams(wlc,p,0,0,rspec,fifo)
   @0xA4B80 attaches rate+fifo; wlc_prep_pdu(wlc,scb,p) @0x18A678 builds the TxD.
   hnd_pkt_free(osh,p,1) @0x215BB8. band arg to lowest_band_rspec = *(wlc+0x24). */
#define FP_HND_PKT_GET   ((int (*)(void *, unsigned int))(0x000160dcu | 1u))
#define FP_LOWEST_BAND_RSPEC ((unsigned int (*)(void *))(0x0018a584u | 1u))
#define FP_PUSH_TXPARAMS ((void (*)(void *, void *, int, int, unsigned int, unsigned int))(0x000a4b80u | 1u))
#define FP_PREP_PDU      ((int (*)(void *, void *, void *))(0x0018a678u | 1u))
#define FP_HND_PKT_FREE  ((void (*)(void *, void *, int))(0x00215bb8u | 1u))
#define FP_MEMCPY        ((void *(*)(void *, const void *, unsigned int))(0x00212376u | 1u))

/* wlc_queue_80211_frag(wlc, p, txq, txparams, scb, char, key, rspec) @0xA54B8 —
   the fw's own "queue a raw 802.11 frame for TX" primitive. It sets up the
   pkt-tag at pkt+0x2c (which prep_pdu-direct left uninitialised → the trap),
   calls push_txparams internally, builds the rev80 descriptor, and enqueues ->
   drains -> AQM-submits. txparams=NULL uses the fw default; scb = our allocated
   bcmc scb; txq = *(*(bss+8)+0xc) (from wlc_send_action_err's own call). It OWNS
   the pkt on every path — never free it after. rspec runtime-derived (band-safe). */
#define FP_QUEUE_80211_FRAG ((int (*)(void *, void *, void *, void *, void *, int, int, unsigned int))(0x000a54b8u | 1u))

/* scb iterator (RAM bodies, FP-redirected): find the associated AP scb.
   wlc_scb_iterinit(container, iter, bsscfg, 0); wlc_scb_iternext(container, iter)
   -> scb or 0. container = wlc[0x14] = *(wlc+0x50). "associated to AP" = bit1 of
   scb+0x1c (from wlc_scb_associated_to_ap). iter is a 4-word scratch struct. */
#define FP_SCB_ITERINIT ((void (*)(unsigned int, void *, void *, unsigned int))(0x002d4f8eu | 1u))
#define FP_SCB_ITERNEXT ((int  (*)(unsigned int, void *))(0x002d4fdeu | 1u))

#if SPC_STA_INJECT
/* Snapshot of the associated-AP scb, keyed by wlc (RSDB = 2 wlc's), captured at
   wlc_scb_set_auth (the hook sp_c_scb_auth_hook below). Injection reads this
   cache instead of iterating the scb list at TX time — inject-time iteration
   raced with association churn (trap in wlc_scb_iterinit -> wlc_scb_get_bssidx).
   Non-zero init keeps them in .data inside the patch region. */
volatile unsigned int spc_ap_scb = 1;
volatile unsigned int spc_ap_wlc = 1;
#endif

/* SP-C Build-2: runs on the wl task (DPC bound via osl_ext_task_current() from
   the RX hook). mode 1 = read the STA container *(wlc+0x50)=0x401d5044 in this
   radio-active wl-task context — the read that faults from the ioctl thread.
   mode 2 (Fix A) = build the cached frame and submit via wlc_queue_80211_frag;
   its internal get_bssidx container deref now runs in the correct context. The
   iovar caches the job (wlc/scb/txq/frame) and hnd_dpc_schedule()s this fn. */
void
spc_dpc_fn(void *arg)
{
    unsigned int w    = spc_job_wlc2;
    unsigned int mode = spc_dpc_mode;
    (void)arg;
    spc_dpc_mode = 0;
    spc_probe2[SPC2_MODE] = mode;
    spc_probe2[SPC2_N]++;
    spc_probe2[SPC2_WLC] = w;
    if (!spc_ram(w)) return;

    if (mode == 1) {                    /* READ: the corrected P2 */
        unsigned int cont = *(volatile unsigned int *)(w + 0x50);
        spc_probe2[SPC2_CONT] = cont;
        if (cont) {
            spc_probe2[SPC2_W0] = *(volatile unsigned int *)cont;        /* 0x401d5044, wl-task ctx */
            spc_probe2[SPC2_W4] = *(volatile unsigned int *)(cont + 4);
            spc_probe2[SPC2_OK] = 0x00C0FFEEu;
        }
        return;
    }
    if (mode == 2) {                    /* SUBMIT: Fix A */
        unsigned int scb = spc_job_scb, txq = spc_job_txq, len = spc_job_len;
        spc_probe2[SPC2_CONT] = spc_ram(scb) ? *(volatile unsigned int *)(scb + 0) : 0;
        spc_probe2[SPC2_OK]   = 0x0005B117u;      /* 'submit reached' marker */
        if (spc_ram(scb) && spc_ram(txq) && len >= 16u && len <= 0x600u) {
            void *osh = (void *)(*(volatile unsigned int *)(w + 4));
            unsigned int p = (unsigned int)FP_HND_PKT_GET(osh, len + 0xd4);
            spc_probe2[SPC2_W0] = p;
            if (spc_ram(p)) {
                unsigned int d     = *(volatile unsigned int *)(p + 8) + 0xbc;   /* TxD headroom */
                unsigned int band  = *(volatile unsigned int *)(w + 0x24);
                unsigned int rspec = FP_LOWEST_BAND_RSPEC((void *)band);
                *(volatile unsigned int *)(p + 8)   = d;
                *(volatile unsigned short *)(p + 0xc) = (unsigned short)len;
                FP_MEMCPY((void *)d, (void *)spc_job_buf, len);
                spc_probe2[SPC2_W4] = (unsigned int)FP_QUEUE_80211_FRAG(
                    (void *)w, (void *)p, (void *)txq, 0, (void *)scb, 0, 0, rspec);
                /* queue_80211_frag owns p — do NOT free here. */
            }
        }
        return;
    }
}

int
spc_tx_handle(void *wlc, unsigned char *value, unsigned int len)
{
    unsigned int w = (unsigned int)wlc, bss, scb;

    spc_tel[SPC_T_HOOK]++;
    spc_tel[SPC_T_LEN] = len;
    spc_tel[SPC_T_FB]  = (value && len >= 4)
        ? (unsigned int)(value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24))
        : 0;

    /* SP-C Build-1: a 1-byte spc_tx arms the guarded container-read probe (run
       from the wl-DPC RX context in sp_b_monitor_rx). No injection is attempted. */
    if (len == 1) { spc_probe[SPCP_REQ] = 1u; return 0; }

    /* SP-C Build-10 (READ-ONLY): a 2-byte spc_tx powers ALL SR power domains on
       (wlc_bmac_srpwr_request_on_all — reaches the scan core's domain, which
       force_ht/wake/DPC/RX contexts do NOT) and then reads the STA AP scb's
       container [*(scb+0)+0x18] = [0x401d5044+0x18] (the exact 0x283978 fault)
       from THIS ioctl thread. If it reads (sphf w18 != 0, no trap), SR power was
       the gate -> the fix is srpwr_on_all around the submit. Nothing is transmitted.
       Result on-device: srpwr_on_all traps in si_srpwr_stat_spinwait's UDF timeout
       (scan-core SR bit never acks from ioctl). Kept for the record; DO NOT run. */
    if (len == 2) {
#if SPC_STA_INJECT
        if (spc_ap_wlc == w && spc_ram(spc_ap_scb) &&
            (*(volatile unsigned char *)(spc_ap_scb + 0x1c) & 2u)) {
            unsigned int bmac = spc_ram(w) ? *(volatile unsigned int *)(w + 0x18) : 0;
            if (bmac) FP_SRPWR_ON_ALL((void *)bmac, 1);   /* power every core's SR domain */
            {
                unsigned int cont = *(volatile unsigned int *)(spc_ap_scb + 0);  /* 0x401d5044 */
                spc_hf[1] = cont;
                spc_hf[2] = *(volatile unsigned int *)(cont + 0x18);             /* the fault addr */
            }
        }
#endif
        return 0;
    }

    /* SP-C Build-11 (SAFE OBSERVATION, no writes, cannot UDF):
       a 3-byte spc_tx snapshots the SR-power ctrl+stat from THIS ioctl context.
       Uses only the safe primitives: si_srpwr_request(sih,0,0) returns raw ctrl
       without writing; si_srpwr_stat(sih) is a plain si_corereg MMIO read. Both
       return without spinwait. Snapshot rendered in cap iovar as `srp probe=...`.
       This closes the evidence gap Build-10 left: what IS the current SR state,
       and what does the fw's own operation set/clear over time (via the RX-hook
       sampler)? See spc_sr_tel[] header for the field layout. */
    if (len == 3) {
        unsigned int bmac = spc_ram(w) ? *(volatile unsigned int *)(w + SPC_WLC_BMAC_OFF) : 0;
        void *sih = (bmac >= 0x200000u && bmac < 0x500000u)
            ? *(void * volatile *)((unsigned char *)bmac + SPC_BMAC_SIH_OFF) : 0;
        spc_sr_tel[SR_T_PROBE_N]++;
        spc_sr_tel[SR_T_WLC]  = w;
        spc_sr_tel[SR_T_BMAC] = bmac;
        spc_sr_tel[SR_T_SIH]  = (unsigned int)sih;
        if (sih) {
            spc_sr_tel[SR_T_CTRL] = FP_SI_SRPWR_REQUEST(sih, 0, 0);
            spc_sr_tel[SR_T_STAT] = FP_SI_SRPWR_STAT(sih);
        }
        return 0;
    }

    /* SP-C Build-14: len==7 = arm the CORE_RDY_SCAN-gated [cont+0x18] read.
       Refuses to arm unless the passive observer has already seen rdy>=1 in
       the ambient RX stream. Auto-disarms after the first successful read OR
       after SPC_R2_MAX_SKIPS delay-path misses (safety). Cap output "r2 arm=1
       reads=N" tells you it fired; "r2 arm=0 reads=0" means nothing yet. */
    if (len == 7) {
        if (spc_r2_tel[R2_RDY_CNT] >= SPC_R2_MIN_RDY_SEEN)
            spc_r2_tel[R2_ARM] = 1u;
        return 0;
    }

    /* SP-C Build-17-A: len==10 = ARM the post-trap snapshot hook. Refuses
       to arm unless HOOKN is non-zero (i.e., we have seen the BPatch fire
       at least once — proof the trampoline is live). Auto-disarms after
       the first successful read. Rearming for successive samples: just
       send len==10 again. */
    if (len == 10) {
        if (spc_snap_tel[SNAP_HOOKN] > 0)
            spc_snap_tel[SNAP_ARM] = 1u;
        return 0;
    }

    /* SP-C Build-15: len==8 = ARM the PMU MinResMask pin (Path #2).
       si_pmu_min_res_set(sih, osh, SPC_PIN_MASK, set=1) OR-in SCAN's
       PWRSW+CORE_RDY+MACPHY_CLK bits into MinResMask. Waits for RES_PENDING
       to drain (bounded 15 ms, NO UDF). Captures pre/post + PMU_RES_STATE
       into spc_pin_tel so cap shows exactly what took. Sequence: arm this
       FIRST (len==8), THEN arm the srpwr-hook deref (len==7). */
    if (len == 8) {
        unsigned int bmac = spc_ram(w) ? *(volatile unsigned int *)(w + SPC_WLC_BMAC_OFF) : 0;
        void *sih  = (bmac >= 0x200000u && bmac < 0x500000u)
                     ? *(void * volatile *)((unsigned char *)bmac + SPC_BMAC_SIH_OFF) : 0;
        void *osh  = spc_ram(w) ? (void *)*(volatile unsigned int *)(w + 4) : 0;
        spc_pin_tel[PIN_ARM_N]++;
        if (sih && osh && spc_r2_tel[R2_PMU_IDX] != 0 && spc_r2_tel[R2_PMU_IDX] < 0x16u) {
            spc_pin_tel[PIN_PRE] = FP_SI_COREREG(sih, spc_r2_tel[R2_PMU_IDX],
                                                 SPC_PMU_MIN_RES_OFF, 0, 0);
            FP_SI_PMU_MIN_RES_SET(sih, osh, SPC_PIN_MASK, 1);
            spc_pin_tel[PIN_POST]  = FP_SI_COREREG(sih, spc_r2_tel[R2_PMU_IDX],
                                                   SPC_PMU_MIN_RES_OFF, 0, 0);
            spc_pin_tel[PIN_RD]    = FP_SI_COREREG(sih, spc_r2_tel[R2_PMU_IDX],
                                                   SPC_R2_PMU_RES_STATE, 0, 0);
            spc_pin_tel[PIN_TS]    = FP_HND_TIME();
            spc_pin_tel[PIN_ARMED] = 1u;
        }
        return 0;
    }

    /* SP-C Build-15: len==9 = RELEASE the pin. si_pmu_min_res_set with set=0
       has default-min protection: can't unset system-required bits, so this
       is always safe. Operator must release explicitly (no auto-release in
       this build; if the pin is left armed indefinitely, SC-SRAM stays on at
       ~mW cost — noted, not a bug for a research session). */
    if (len == 9) {
        unsigned int bmac = spc_ram(w) ? *(volatile unsigned int *)(w + SPC_WLC_BMAC_OFF) : 0;
        void *sih  = (bmac >= 0x200000u && bmac < 0x500000u)
                     ? *(void * volatile *)((unsigned char *)bmac + SPC_BMAC_SIH_OFF) : 0;
        void *osh  = spc_ram(w) ? (void *)*(volatile unsigned int *)(w + 4) : 0;
        spc_pin_tel[PIN_REL_N]++;
        if (sih && osh && spc_r2_tel[R2_PMU_IDX] != 0 && spc_r2_tel[R2_PMU_IDX] < 0x16u) {
            FP_SI_PMU_MIN_RES_SET(sih, osh, SPC_PIN_MASK, 0);
            spc_pin_tel[PIN_POST]  = FP_SI_COREREG(sih, spc_r2_tel[R2_PMU_IDX],
                                                   SPC_PMU_MIN_RES_OFF, 0, 0);
            spc_pin_tel[PIN_RD]    = FP_SI_COREREG(sih, spc_r2_tel[R2_PMU_IDX],
                                                   SPC_R2_PMU_RES_STATE, 0, 0);
        }
        spc_pin_tel[PIN_ARMED] = 0u;
        return 0;
    }

    /* Task 3a — OBSERVE ONLY. The make-or-break question: is a usable scb present
       in pure monitor mode (no association)? The primary bsscfg *(wlc+0x168)
       exists (probe #1 confirmed), and wlc_bsscfg_bcmcscbinit stores that
       bsscfg's broadcast scb at bsscfg+0x34 (setting its band field +0x28 from
       wlc[9]). So the injection scb candidate = *(*(wlc+0x168)+0x34). We echo it
       plus the fields wlc_prep_pdu will dereference — scb+0x00 (word0), scb+0xc
       (its bsscfg — must equal our bsscfg), scb+0x28 (band gate, want 0 for
       2.4GHz). All derefs RAM-range-gated; no alloc, no descriptor build, no TX.
       Task 3b calls wlc_prep_pdu with this scb only after it validates here. */
    bss = spc_ram(w) ? *(volatile unsigned int *)(w + 0x168) : 0;
    spc_tel[SPC_T_BSSCFG] = bss;
#if SPC_STA_INJECT
    /* STA+inject: use the AP scb snapshotted at association (keyed by this wlc),
       validated as still associated (bit1 of scb+0x1c). Full associated context
       => every wlc_queue_80211_frag check (stats, CSA, keymgmt, ...) is satisfied.
       Its OWN bsscfg (scb+0xc) is the associated bsscfg, NOT the primary
       *(wlc+0x168): dual-STA puts the AP peer scb under a different bsscfg, which
       is why the earlier primary-bsscfg scan missed it. Derive the txq from that
       bsscfg. No inject-time scb-list iteration — that raced (trap in
       wlc_scb_iterinit -> wlc_scb_get_bssidx). */
    /* SP-C Build-17-A extension: prefer the snapshotted scb from fw's own
       natural drain (spc_snap_tel[SNAP_SCB]) over spc_ap_scb. The snapshot scb
       has *scb in low RAM (readable everywhere — no wall), so the injection
       path's downstream container derefs (queue_80211_frag's own [cont+0x18]
       at 0x283978, the ratesel iterinit, etc.) all read safely. spc_ap_scb
       has *scb=0x401d5044 (backplane) which triggers the wall for 15+ builds.
       Only use the snap scb if it's actually a valid RAM pointer.  */
    if (spc_ram(spc_snap_tel[SNAP_SCB])) {
        scb = spc_snap_tel[SNAP_SCB];       /* Build-17-A safe scb            */
    } else {
        scb = (spc_ap_wlc == w && spc_ram(spc_ap_scb) &&
               (*(volatile unsigned char *)(spc_ap_scb + 0x1c) & 2u))
              ? spc_ap_scb : 0;              /* fall back to old path if no snap */
    }
    spc_tel[SPC_T_ALLOC] = scb;             /* diag: whichever scb we chose    */
    spc_tel[SPC_T_G6C]   = spc_ap_wlc;      /* diag: wlc the cache belongs to  */
    if (spc_ram(scb)) {
        bss = *(volatile unsigned int *)(scb + 0xc);   /* scb->bsscfg          */
        spc_tel[SPC_T_BSSCFG] = bss;
    }
#else
    /* Monitor: no association exists, so fabricate the bcmc scb the fw's own way. */
    scb = spc_ram(bss) ? *(volatile unsigned int *)(bss + 0x34) : 0;
    spc_tel[SPC_T_G6C] = spc_ram(bss) ? *(volatile unsigned int *)(bss + 0x6c) : 0;
    if (scb == 0 && spc_ram(bss)) {
        spc_tel[SPC_T_ALLOC] = (unsigned int)FP_WLC_BCMCSCB_ALLOC((void *)w, (void *)bss);
        scb = *(volatile unsigned int *)(bss + 0x34);
    }
#endif
    spc_tel[SPC_T_SCB]    = scb;
    spc_tel[SPC_T_DTXP]   = spc_ram(scb) ? *(volatile unsigned int *)(scb + 0x00) : 0;
    spc_tel[SPC_T_SCBBSS] = spc_ram(scb) ? *(volatile unsigned int *)(scb + 0xc)  : 0;
    spc_tel[SPC_T_SCBBND] = spc_ram(scb) ? *(volatile unsigned int *)(scb + 0x28) : 0;

    /* Task 4 — TRANSMIT via the fw's own frame-queue primitive. Build the pkt
       (frame + 0xBC TxD headroom), then hand it to wlc_queue_80211_frag, which
       sets up the pkt-tag (fixing the prep_pdu-direct trap), builds the rev80
       descriptor, and enqueues -> drains -> AQM-submits. It OWNS the pkt on every
       path, so we NEVER free it here. Our raw frame, our scb, our runtime rspec.
       txq = *(*(bss+8)+0xc), the same source wlc_send_action_err uses.

       Build-17-A: the snap scb (0x3b8b14) has *(scb+0xc)=0x660c0101 which fails
       spc_ram — probably a bcmc/broadcast scb with a different layout. Fall back
       to the primary bsscfg (*(wlc+0x168)) for the txq derivation when the scb's
       own bsscfg isn't a RAM pointer. */
    {
        unsigned int txq_bss = spc_ram(bss) ? bss
                             : (spc_ram(w) ? *(volatile unsigned int *)(w + 0x168) : 0);
        unsigned int wif = spc_ram(txq_bss) ? *(volatile unsigned int *)(txq_bss + 8) : 0;
        spc_tel[SPC_T_TXQ] = spc_ram(wif) ? *(volatile unsigned int *)(wif + 0xc) : 0;
    }
    /* SP-C Build-8 (ROOT-CAUSE FIX): build + NORMAL synchronous submit via
       wlc_queue_80211_frag from THIS ioctl thread, but wake+clock the MAC across the
       call (FP_WAKE_OVERRIDE_SET + FP_WLC_FORCE_HT) so the container 0x401d5044 — in
       clock-gated MAC/backplane memory — is readable exactly as it is from wlc_dpc.
       No deref patching; every container access in the drain reads. */
    /* Build-17-A: dropped SCBBSS==bss guard — the snap scb's bsscfg was
       already derived from scb+0xc above, so the equality is trivially
       true; the old guard was only relevant when we hand-picked bss from
       the primary bsscfg path and then verified consistency. */
    if (spc_ram(scb) && spc_ram(spc_tel[SPC_T_TXQ]) &&
        value != 0 && len >= 16 && len <= 0x600) {
        void *osh = (void *)(spc_ram(w) ? *(volatile unsigned int *)(w + 4) : 0);
        unsigned int p = osh ? (unsigned int)FP_HND_PKT_GET(osh, len + 0xd4) : 0;
        spc_tel[SPC_T_PKT] = p;
        if (spc_ram(p)) {
            unsigned int d     = *(volatile unsigned int *)(p + 8) + 0xbc;   /* TxD headroom */
            unsigned int band  = *(volatile unsigned int *)(w + 0x24);
            unsigned int rspec = FP_LOWEST_BAND_RSPEC((void *)band);
            *(volatile unsigned int *)(p + 8)   = d;
            *(volatile unsigned short *)(p + 0xc) = (unsigned short)len;
            FP_MEMCPY((void *)d, value, len);
            /* ROOT-CAUSE FIX: wake + clock the MAC so the container 0x401d5044
               (clock-gated MAC/backplane memory) is readable from THIS ioctl thread,
               exactly as it is from wlc_dpc. Then run the NORMAL synchronous submit —
               no deref patching. */
            FP_WAKE_OVERRIDE_SET((void *)w, SPC_WAKE_MASK);
            FP_WLC_FORCE_HT((void *)w, 1, 0);
            /* diag: read the exact addr that trapped at 0x283978 (container+0x18),
               now that the clock is forced. If this reads, the fix holds. */
            {
                unsigned int cont = *(volatile unsigned int *)(scb + 0);   /* 0x401d5044 */
                spc_hf[1] = cont;
                spc_hf[2] = *(volatile unsigned int *)(cont + 0x18);       /* the faulting read */
            }
            /* Build-18: arm the snap-hook gate ONLY around the injection call.
               The hook fires in fw's drain path (which iterates our scb) to
               capture (cont, scb) into spc_snap_tel[]. Cleared immediately
               after so the hook is inert during subsequent normal STA data. */
            spc_snap_gate = 1;
            spc_tel[SPC_T_DMA] = (unsigned int)FP_QUEUE_80211_FRAG(
                (void *)w, (void *)p, (void *)spc_tel[SPC_T_TXQ],
                0 /* txparams: fw default */, (void *)scb, 0, 0, rspec);
            spc_snap_gate = 0;
            FP_WLC_FORCE_HT((void *)w, 0, 0);
            FP_WAKE_OVERRIDE_CLEAR((void *)w, SPC_WAKE_MASK);
            /* queue_80211_frag owns p (enqueues or frees) — do NOT free here. */
        }
    }
    return 0;                        /* BCME_OK; the frame goes out from the fw's own drain */
}

/* Trampoline over wlc_iovar_ext's RAM entry (0x2B0760). Displaced 4 bytes =
   `push.w {r4,r5,r6,r7,r8,sb,sl,fp,lr}` (2d e9 f0 4f); we re-execute it, match
   name (r1) == "spc_tx", normalise value/len the way the function does
   (params ? params : arg), call the echo handler, and return BCME_OK to
   iovar_ext's CALLER. On no match, rejoin at 0x2B0764 (the `sub sp,#0x84`
   right after the push) with r0-r3 intact — the function re-derives r4/r5/r6
   from them, so clobbering r4/r5 for the compare is safe. Stack: entry sp is
   8-aligned; push.w saves 9 regs (36B), so the entry stack args arg/alen sit at
   [sp+36]/[sp+40]; sub #4 re-aligns for the bl. Return to firmware addresses
   via movw/movt+bx (absolute b.w does not survive relocation — as in SP-B). */
__attribute__((naked))
void
sp_c_iovar_hook(void)
{
    asm(
        "push.w {r4,r5,r6,r7,r8,r9,r10,r11,lr}\n"  /* = displaced push.w (36B)  */
        "ldr  r4, [r1]\n"           /* name[0..3]                               */
        "movw r5, #0x7073\n"        /* "sp"                                     */
        "movt r5, #0x5f63\n"        /* => 0x5f637073 = "spc_"                   */
        "cmp  r4, r5\n"
        "bne  1f\n"
        "ldrb r4, [r1, #4]\n"
        "cmp  r4, #0x74\n"          /* 't'                                      */
        "bne  1f\n"
        "ldrb r4, [r1, #5]\n"
        "cmp  r4, #0x78\n"          /* 'x'                                      */
        "bne  1f\n"
        "ldrb r4, [r1, #6]\n"
        "cbnz r4, 1f\n"             /* must be NUL                              */
        "cmp  r2, #0\n"             /* params == NULL ?                         */
        "bne  2f\n"
        "ldr  r2, [sp, #36]\n"      /* value = arg  (first stacked arg)         */
        "ldr  r3, [sp, #40]\n"      /* len   = alen                            */
        "2:\n"
        "mov  r1, r2\n"             /* value                                   */
        "mov  r2, r3\n"             /* len                                     */
        "sub  sp, #4\n"             /* 8-byte align for the call                */
        "bl   spc_tx_handle\n"      /* (wlc=r0, value=r1, len=r2)               */
        "add  sp, #4\n"
        "movs r0, #0\n"             /* BCME_OK — we handled spc_tx              */
        "pop.w {r4,r5,r6,r7,r8,r9,r10,r11,pc}\n"  /* restore + return to caller */
        "1:\n"
        "movw r4, #0x0765\n"        /* 0x2B0764 | 1: rejoin after the push.w    */
        "movt r4, #0x002b\n"
        "bx   r4\n"
    );
}

__attribute__((at(0x2B0760, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BPatch(sp_c_iovar_hook, sp_c_iovar_hook);

/* SP-C: stub wlc_tx_status_update_counters @0x2EB130 to `movs r0,#0; bx lr`
   (0x47702000, LE bytes 00 20 70 47). It is TX statistics only — every caller
   ignores its return — and its per-scb table deref (scb+0x300) traps on our
   monitor-only scb, which never gets the associated-STA rate/stats context.
   Skipping it lets injection reach the real build/submit. Unguarded, so the
   committed STA+inject image applies this stub too (TX stats are skipped). */
__attribute__((at(0x2EB130, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(spc_txstat_stub, 0x47702000);

/* SP-C: stub wlc_csa_is_tx_blocked @0x2A9182 (RAM body) to `movs r0,#0; bx lr`
   (return 0 = "TX not blocked"). It's the channel-switch-announcement TX gate;
   its wlc_csa_get_csa_count deref traps on our injected path, and no CSA is in
   progress on the test AP, so returning "not blocked" is correct + safe.
   Callers use the return (queue_80211_frag: nonzero => fail), so 0 = proceed. */
__attribute__((at(0x2A9182, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
GenericPatch4(spc_csa_stub, 0x47702000);

/* SP-C (STA+inject): snapshot the associated-AP scb at wlc_scb_set_auth
   @0x2D5584 (RAM). At entry r0=wlc, r2=scb (param_3 — its +0x10 is the peer MAC,
   +0x1c the flags word we later test for "associated"). This fires when the AP
   scb's auth state is set during association, so caching (wlc, scb) here gives
   injection a stable AP scb WITHOUT iterating the scb list at TX time — that
   iteration raced with association churn and trapped in
   wlc_scb_iterinit -> wlc_scb_get_bssidx. Keyed by wlc because RSDB runs two.
   Displaced 4 bytes = `push.w {r4-fp,lr}` (2d e9 f0 4f); we re-execute it, cache,
   then rejoin at 0x2D5588 (the `sub sp,#0x1c` right after). r4 is clobbered but
   the push saved the caller's r4 and the function re-derives its own via
   `mov r4,r2` at 0x2D5590; r0-r3 are untouched. STA-image only. */
#if SPC_STA_INJECT
__attribute__((naked))
void
sp_c_scb_auth_hook(void)
{
    asm(
        "push.w {r4,r5,r6,r7,r8,r9,r10,r11,lr}\n"  /* = displaced push.w (36B)  */
        "ldr  r4, =spc_ap_scb\n"
        "str  r2, [r4]\n"           /* spc_ap_scb = scb (param_3, r2)           */
        "ldr  r4, =spc_ap_wlc\n"
        "str  r0, [r4]\n"           /* spc_ap_wlc = wlc (param_1, r0)           */
        "movw r4, #0x5589\n"        /* 0x2D5588 | 1: rejoin after the push.w    */
        "movt r4, #0x002d\n"
        "bx   r4\n"
        ".ltorg\n"
    );
}

__attribute__((at(0x2D5584, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BPatch(sp_c_scb_auth_hook, sp_c_scb_auth_hook);

/* SP-C Build-13B — hook wl_sc_recv @0x2f87d4 entry (RAM, called from
   wl_sc_dpc's RX branch — same SC-DPC context).

   Why wl_sc_recv and not wl_sc_dpc itself: wl_sc_dpc is ROM @0x98c5c and
   would need the project's first flash-patch entry to hook directly.
   wl_sc_recv is already in RAM (mapped by __ramfnptr_wl_sc$wl_sc_recv
   @0x20fb0c) and runs INSIDE wl_sc_dpc after wlc_bmac_dpc_cmn has verified
   the SC's MAC/backplane is clocked — the exact "wl_sc_dpc context"
   FINDINGS-E named as where [0x401d5044] reads fine. Build-12C proved the
   RX-context stat gate in the STA's wlc_recv (one core away) does NOT
   reach this window; testing wl_sc_recv's window here.

   Entry disasm (Ghidra Disasm3 on 0x2f87d4):
     0x2f87d4  push.w {r4,r5,r6,r7,r8,r9,r10,r11,lr}   ; 4 bytes -- displaced
     0x2f87d8  mov    r11, r3                            ; rejoin here (thumb|1)
     0x2f87da  ldr    r3, [0x2f88d0]
     ...
   r0 at entry = sc_state (same as wl_sc_dpc's r0). Displaced reg list
   0x4FF0 -> push.w encoding E92D 4FF0 -> bytes 2d e9 f0 4f. IDENTICAL to
   sp_c_scb_auth_hook's + sp_c_iovar_hook's displaced push — proven shape.
   Trampoline replays it, calls our C hook with r0=sc_state, rejoins at
   0x2f87d8|1.

   Hook body (safe): cache sc_state on first entry (Path A prep, free),
   then if the cached AP scb is valid, try the [cont+0x18] deref. Cont-
   window guard (>= 0x40000000) makes a stale mirror harmless. NO writes
   to fw state — pure observation.

   The RX-hook Build-11 sampler stays in place; this hook adds a SECOND
   observation channel inside the SC's own RX processing (not the STA's).
   Cap output "dpc n=N cont=X last=X reads=N miss=N" tells the story:
   reads growing without a trap = SC context IS the right one -> Build-14
   swaps the read for wlc_queue_80211_frag. */
void
sp_c_sc_dpc_read(void *sc_state)
{
    /* Build-21: gate this hook on spc_snap_gate — the (cont >= 0x40000000)
       check below is INSUFFICIENT: 0x401d5044 IS the walled scan-core
       container address, and passes that check trivially. When SR power
       domain is de-powered (~normal STA data flow), *(cont+0x18) traps
       (dhd_mem_dump ~1.5s after CONNECT_SUCCESS on Build-17..19). Matches
       Build-18/19 gating of sp_c_snapshot_read + sp_c_srpwr_read. */
    if (!spc_snap_gate) return;
    unsigned int cont;
    spc_dpc_tel[DPC_T_ATTEMPT]++;
    if (spc_sc_state == 1u) {
        spc_sc_state = (unsigned int)sc_state;                /* one-time capture */
    }
    if (spc_ap_scb < 0x200000u || spc_ap_scb >= 0x500000u) return;
    cont = *(volatile unsigned int *)(spc_ap_scb + 0);
    spc_dpc_tel[DPC_T_CONT] = cont;
    if (cont < 0x40000000u) {
        spc_dpc_tel[DPC_T_MISS]++;
        return;
    }
    spc_dpc_tel[DPC_T_LAST]  = *(volatile unsigned int *)(cont + 0x18);
    spc_dpc_tel[DPC_T_READS]++;
}

__attribute__((naked))
void
sp_c_sc_dpc_hook(void)
{
    asm(
        "push.w {r4,r5,r6,r7,r8,r9,r10,r11,lr}\n"   /* = displaced push.w (36B)  */
        "push   {r0,r1,r2,r3,r12,lr}\n"             /* save caller-saveds for C  */
        /* r0 = sc_state (still in r0 -- push preserved it) */
        "bl     sp_c_sc_dpc_read\n"                 /* void(void *sc_state)      */
        "pop    {r0,r1,r2,r3,r12,lr}\n"
        "movw   r12, #0x87d9\n"                     /* 0x2f87d8 | 1: rejoin      */
        "movt   r12, #0x002f\n"
        "bx     r12\n"
        ".ltorg\n"
    );
}

__attribute__((at(0x2f87d4, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BPatch(sp_c_sc_dpc_hook, sp_c_sc_dpc_hook);

/* SP-C Build-13B-part2 — BPatch on wlc_bmac_srpwr_request_on @0x29daa0.
   Diagnostic-only: counts invocations + records the bmac argument. Fires on
   ISR-related and SC-RX paths (many callers) — a high-frequency test that the
   BPatch mechanism itself works end-to-end. If SRP2_HOOK grows while
   dpc n=0 stays flat, we've proven wl_sc_recv is genuinely silent on this
   device (not a broken hook) and the design must move to Path A proper.

   Entry disasm (Ghidra Disasm3 on 0x29daa0):
     0x29daa0  push  {r4,lr}       ; 2 bytes
     0x29daa2  mov   r4, r0        ; 2 bytes
     0x29daa4  bl    0x000dfa7c    ; wlc_bmac_coreunit(r0=bmac); rejoin here (|1)
     0x29daa8  cbz   r0, ...
   Displacing 4 bytes (push + mov) fits a b.w. Trampoline replays both, then
   rejoins at 0x29daa4|1. r0 = bmac at entry — passed to our C hook. */
/* SP-C Build-14: cache the PMU coreidx once (~$0 cost per subsequent hit).
   si_findcoreidx returns a small index (<0x16) on success; sentinel 0x17 on
   failure so we don't retry every hook fire. Chipc is always-on so si_corereg
   at that coreidx is safe under any power state. */
static int
spc_r2_ensure_pmu_idx(void *sih)
{
    int idx;
    if (spc_r2_tel[R2_PMU_IDX] != 0 && spc_r2_tel[R2_PMU_IDX] < 0x16u) return 1;
    if (!sih) { spc_r2_tel[R2_ERR] = 2; return 0; }
    idx = FP_SI_FINDCOREIDX(sih, SPC_R2_PMU_COREID, 0);
    if ((unsigned int)idx >= 0x16u) {
        spc_r2_tel[R2_ERR] = 1;
        spc_r2_tel[R2_PMU_IDX] = 0x17u;   /* remember-the-failure so we don't spam findcoreidx */
        return 0;
    }
    spc_r2_tel[R2_PMU_IDX] = (unsigned int)idx;
    return 1;
}

/* SP-C Build-14 core: passive PMU_RES_STATE sampler + armed [cont+0x18] read
   under the CORE_RDY_SCAN gate with CORE_PWRUP_WAR (delay + re-check). Runs
   from the wlc_bmac_srpwr_request_on hook (fires ~1000×/s). Every step is
   guarded — cold boot with R2_ARM=0 does NOTHING but sample, so the fw is
   safe if the SCAN hypothesis is wrong. */
static void
spc_r2_regate_probe(void *bmac)
{
    void *sih;
    unsigned int rs, cont;

    if (!bmac) return;
    sih = *(void * volatile *)((unsigned char *)bmac + SPC_BMAC_SIH_OFF);
    if (!sih) { spc_r2_tel[R2_ERR] = 2; return; }
    if (!spc_r2_ensure_pmu_idx(sih)) return;

    /* (1) PASSIVE snapshot — pure ChipC MMIO, always safe. */
    rs = FP_SI_COREREG(sih, spc_r2_tel[R2_PMU_IDX], SPC_R2_PMU_RES_STATE, 0, 0);
    spc_r2_tel[R2_LAST_RS] = rs;
    spc_r2_tel[R2_SAMPS]++;
    if (rs & SPC_R2_RDY_SCAN_BIT) spc_r2_tel[R2_RDY_CNT]++;
    if (rs & SPC_R2_RDY_AUX_BIT)  spc_r2_tel[R2_AUX_CNT]++;   /* free-lunch */

    /* (2) Armed-deref gate — every clause must pass, else we never touch cont. */
    if (!spc_r2_tel[R2_ARM]) return;
    /* Build-16: use TEST_BIT (SCAN or AUX depending on SPC_PIN_MASK setting)
       instead of hard-coding SCAN. Build-14 hard-coded SCAN and never emerged
       from the wall; if the container is in the OTHER domain we'd never even
       consider derefing under an ambient reading. Testing both bits explicitly. */
    if ((spc_r2_tel[R2_RDY_CNT] + spc_r2_tel[R2_AUX_CNT]) < SPC_R2_MIN_RDY_SEEN) return;
    if (!spc_ram(spc_ap_scb)) return;
    if (!(*(volatile unsigned char *)(spc_ap_scb + 0x1c) & 2u)) return;
    cont = *(volatile unsigned int *)(spc_ap_scb + 0);
    if (cont < 0x40000000u) { spc_r2_tel[R2_ERR] = 3; return; }

    /* (3) Build-16: ALWAYS apply CORE_PWRUP_WAR delay + re-check. Build-14's
       fast path (deref immediately on first-sample rs & bit == 1) matches EXACTLY
       Broadcom's "STATUS can indicate ON while in ON→OFF transition" warning —
       and Build-14 trapped in exactly that path. Drop it. Even if the first
       sample says ready, wait 100 µs first, re-sample, only then deref. */
    FP_HND_DELAY(SPC_R2_UPDN_DELAY_US);
    rs = FP_SI_COREREG(sih, spc_r2_tel[R2_PMU_IDX], SPC_R2_PMU_RES_STATE, 0, 0);
    spc_r2_tel[R2_LAST_RS] = rs;
    if (rs & SPC_R2_RDY_TEST_BIT) {
        spc_r2_tel[R2_LAST_W18] = *(volatile unsigned int *)(cont + 0x18);
        spc_r2_tel[R2_READS]++;
        spc_r2_tel[R2_UNLOCKED]++;    /* Build-16: all reads take the WAR path */
        spc_r2_tel[R2_ARM] = 0;
        return;
    }

    /* (4) Test bit still 0 after delay — SKIP; auto-disarm at safety cap. */
    spc_r2_tel[R2_DELAY_HITS]++;
    if (spc_r2_tel[R2_DELAY_HITS] >= SPC_R2_MAX_SKIPS) spc_r2_tel[R2_ARM] = 0;
}

void
sp_c_srpwr_read(void *bmac)
{
    /* Build-19: gate this hook too. Reuses spc_snap_gate — both hooks
       are only meaningful during our injection window. The srpwr hook
       fires from wlc_bmac_srpwr_request_on which runs 60k+ times/session
       during normal STA (LE scan, roaming, DPC), and unguarded activity
       here appears to destabilise SR-power-domain state, causing later
       reads to the walled scan-core container (0x401d5044) to fault with
       "Dongle_Trap type 0x4" ~1.5s after CONNECT_SUCCESS on Build-18.
       Build-18 gated snap hook alone but crash persisted with srp2 n=63k. */
    if (!spc_snap_gate) return;
    spc_srp_tel[SRP_T_HOOK]++;
    spc_srp_tel[SRP_T_BMAC] = (unsigned int)bmac;
    spc_r2_regate_probe(bmac);          /* SP-C Build-14 */
}

__attribute__((naked))
void
sp_c_srpwr_hook(void)
{
    asm(
        "push  {r4, lr}\n"                    /* displaced push {r4,lr}     */
        "mov   r4, r0\n"                      /* displaced mov r4,r0        */
        "push  {r0, r1, r2, r3, r12, lr}\n"   /* save caller-saveds for C   */
        "bl    sp_c_srpwr_read\n"             /* void(void *bmac), r0=bmac  */
        "pop   {r0, r1, r2, r3, r12, lr}\n"
        "movw  r12, #0xdaa5\n"                /* 0x29daa4 | 1: bl coreunit  */
        "movt  r12, #0x0029\n"
        "bx    r12\n"
        ".ltorg\n"
    );
}

__attribute__((at(0x29daa0, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BPatch(sp_c_srpwr_hook, sp_c_srpwr_hook);

/* SP-C Build-17-A — post-trap snapshot at wlc_scb_get_bssidx+0x16 (0x2d4f46).
   The C worker is called every time PC reaches the hook, so HOOKN always
   increments — that alone proves the BPatch is live from natural drains.
   The real work (deref [cont+0x18]) is gated on SNAP_ARM: cold boot never
   dereferences anything. Once ARM is set (via `wlcmd tx <10-byte>`), the
   very next drain-hook invocation captures the value and auto-disarms. */
void
sp_c_snapshot_read(unsigned int cont, unsigned int scb)
{
    /* Build-22: Build-18's blanket gate removed. Build-21 proved the STA-crash
       is in sp_c_sc_dpc_read (which does an unconditional walled-container
       deref), NOT in this function (which only writes counters + captures cont
       and scb; the *(cont+0x18) deref below is guarded by SNAP_ARM which stays
       0 unless the operator arms it). The gate was overly-conservative
       insurance and it actively broke injection: when this hook is prevented
       from firing on natural drains, SNAP_SCB never populates, so spc_tx_handle
       has no captured scb and falls back to the walled scb -> ENOMEM/EINVAL.
       Now: hook fires on every natural drain, harmlessly captures (cont, scb);
       injection has SNAP_SCB ready when it needs it. Verified live 2026-08-10:
       Build-21 (this gate present) breaks injection; Build-22 (removed) works. */
    spc_snap_tel[SNAP_HOOKN]++;
    /* ALWAYS track the last (cont, scb) pair fw's drain naturally uses.
       This is FREE info even without arm — tells us which scb's iteration
       path is safe. Build-17-A found cont=0x3d4890 is what fw's drain
       actually iterates; the "STA container 0x401d5044" we've been trying
       to gate for 15 builds is on a scb path fw NEVER touches naturally. */
    spc_snap_tel[SNAP_CONT] = cont;
    spc_snap_tel[SNAP_SCB]  = scb;
    /* If we ever see a high-backplane cont, keep that as a separate
       trophy — that IS the STA container Build-14/15/16 fought over. */
    if (cont >= 0x40000000u) {
        spc_snap_tel[SNAP_CONT_HI] = cont;
        spc_snap_tel[SNAP_SCB_HI]  = scb;
    }
    if (!spc_snap_tel[SNAP_ARM]) return;
    spc_snap_tel[SNAP_W18]  = *(volatile unsigned int *)(cont + 0x18);
    spc_snap_tel[SNAP_READS]++;
    spc_snap_tel[SNAP_ARM] = 0;
}

/* Trampoline at 0x2d4f46 — displaces the 4-byte Thumb-2-wide
   `ldr.w r4,[r2,#0x164]` (bytes D2 F8 64 41). r2 = container = 0x401d5044 at
   this point (unchanged since 0x2d4f36's `ldr r2,[r1,#0x0]`). After our C
   call, restore r0-r3, re-execute the displaced ldr, then rejoin at
   0x2d4f4a (the next instruction, `ldr.w r4,[r4,r0,lsl #0x2]`). */
__attribute__((naked))
void
sp_c_bssidx_snap_hook(void)
{
    asm(
        /* Build-18: expand save list to r4-r7 as AAPCS-belt-and-suspenders.
           Push size = 10 regs * 4 = 40 bytes (multiple of 8, keeps sp
           alignment class). If the C worker's compiler-emitted prologue
           correctly saves callee-clobbered regs (per AAPCS) this is
           redundant; the extra insurance costs 16 stack bytes per call and
           removes register corruption as a residual crash cause. */
        "push  {r0, r1, r2, r3, r4, r5, r6, r7, r12, lr}\n"
        "mov   r0, r2\n"                        /* arg0 = cont = *(scb+0)    */
        /* r1 already = scb (fn arg 2) at hook entry, preserved by push+mov */
        "bl    sp_c_snapshot_read\n"            /* void(cont, scb)           */
        "pop   {r0, r1, r2, r3, r4, r5, r6, r7, r12, lr}\n"
        "ldr.w r4, [r2, #0x164]\n"              /* displaced instruction     */
        "movw  r12, #0x4f4b\n"                  /* 0x2d4f4a | 1 : rejoin     */
        "movt  r12, #0x002d\n"
        "bx    r12\n"
        ".ltorg\n"
    );
}

__attribute__((at(0x2d4f46, "", CHIP_VER_BCM4389c1, FW_VER_20_101_90_r1098710)))
BPatch(sp_c_bssidx_snap_hook, sp_c_bssidx_snap_hook);
#endif  /* SPC_STA_INJECT */
