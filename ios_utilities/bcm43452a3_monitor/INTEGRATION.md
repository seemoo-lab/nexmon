# Nexmon integration boundary

This contribution is an experimental standalone iOS utility under `ios_utilities`: source builder, exact-target loader/control CLI, native helpers, tests and sanitized evidence. It does not alter existing Nexmon targets, claim a generic BCM4345-family port or add an unverified supported-device entry.

The pre-monitor-copy mirror and bounded split-RX fetch preserve verified packet ownership and bound allocation/in-flight work. The independent `nxrawv1` control starts disarmed, scopes one runtime AP and excludes both own addresses. Epoch52 must not overlap own-address bytes32..49. Coherent reads reject bad sequence/scope/epoch/length/ordinal/FCS; observed ordinal gaps are reported.

Both code regions are bound to the exact stock/output hashes and examined target contracts. Do not transplant nearby-chip addresses or blindly relax unknown relocations. The two known heap donors, modeled startup/reclaim/MPU/upload behavior and actual target startup/capture were investigated; general lifetime on other devices remains unproven.

The supplied iOS loading path uses exact pinned Dopamine3.0.11 and a temporary digest-data transaction with restoration. A running timed-out native/loader operation blocks another operation and reload. Generic BPF RFMON, injection, channel hopping and permanent system-image replacement are absent. Shared locks also coordinate an installed Wi-Fi Map when present.

Maintainers can review this independent target proof before deciding how to adapt its builder/wrappers into the normal Nexmon build hierarchy. This directory is not presented as that integration already being complete. [Reproduction](REPRODUCING.md), [dependency sources](DEPENDENCIES.md) and [provenance](PROVENANCE.md) define the actual supported scope.
