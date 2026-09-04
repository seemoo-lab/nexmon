# mdk3-compat

These are the original **mdk3** shared sources, retained verbatim after the
standalone `mdk3` utility was replaced by `mdk4` (see the parent directory).

They exist solely for the in-app attack plugins under
`app/app/src/main/external/src/` (`libauthflood`, `libbeaconflood`,
`libcountermeasures`, `libwids`), which were derived from mdk3 and compile a
subset of these files directly. Those plugins depend on the mdk3-era API
(notably the single-interface `osdep_start(char *interface)` and the
libnl-free `channelhopper.c`), which changed incompatibly in mdk4, so they are
kept pinned to these sources rather than ported.

Nothing here is built by the `mdk4` tool or by `make -C utilities`; it is
source-only. Do not add build files here.
