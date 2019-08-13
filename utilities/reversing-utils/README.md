# Reversing Utilities
Collection of scripts aimed to speedup the reversing process of new firmwares

* nexmon_find_offsets_from_pattern.py 
    IDAPython script to find functions addresses inside an IDB from a PAT pattern file
* nexmon_set_labels.py 
    IDAPython script to relabel your IDB wit names/offsets from an offset file (like wrapper.c)

The idea is to label an IDB of an already reversed FW, export a pattern file and use it to find the offests in other FWs.



