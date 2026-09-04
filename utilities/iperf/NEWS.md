**Iperf 2 - Network testing tool** *(based from 2.0.5)* 

This document is not done. RJM 8/24/2021  
    
Man page: https:://iperf2.sourceforge.io/iperf-manpage.html    

---
Iperf 2, this program, is different from the iperf 3 found 
at https://github.com/esnet/iperf 

Each can be used to 
measure network performance, however, **iperf 2 and iperf 3 DO NOT interoperate.**   
They are completely different implementations with different strengths, capabilities and 
different options. Iperf 2 took its code base from the original iperf code (that stalled at 2.0.5.) 
Iperf 3 is a rewrite from scratch.
  
Both Iperf 2 (now at 2.1.4) and iperf 3 are both under active development (as of mid-2021)  
  
Iperf 2 vs 3 table: https://iperf2.sourceforge.io/IperfCompare.html
  
***See the end of the file for license conditions***

---
Iperf 2.1.4 has many user visible changes since 2.0.13 and even more since
2.0.5 The below describes many of these user visible changes with
a focus on 2.1.4 compared to 2.0.13    
  
    
---
**Iperf 2 new metrics**

**NetPwr**

    Network power: The network power (NetPwr) metric originates from Kleinrock and Jaffe circa 1980.
    It is a measure of a desirable property divided by an undesirable property.
    It is defined as throughput/delay. For TCP transmits, the delay is the sampled RTT times.
    For TCP receives, the delay is the write to read latency. For UDP the delay is the
    packet end/end latency.

    Note, one must use -i interval with TCP to get this as that's what sets the RTT sampling rate.
    The metric is scaled to assist with human readability.

**InP**

    The InP metric is derived from Little's Law or Little's Lamma. LL in queuing theory is a
    theorem that determines the average number of items (L) in a stationary queuing system
    based on the average waiting time (W) of an item within a system and the average number
    of items arriving at the system per unit of time (lambda). Mathematically,
    it's L = lambda * W. As used here, the units are bytes. The arrival rate is
    taken from the writes.  
  
---  

**Iperf 2 Enhanced Reports**

Much of the new outputs require **-e** for **--enhanced-reports**. This is supported on both the client and server  
    
**Client side (TCP)**
        
***Write:*** the number of socket write calls  
***Err:*** the number of write syscalls that returned with a non fatal error  
***Rtry:*** The sampled TCP retry value   
***RTT:*** the sampled TCP round trip time  
***CWND:*** the sampled TCP congestion window  
***NetPwr:*** the computed network power (using RTT)  
	
**Server side (TCP)**  

***Burst Latency:*** The avg/min/max/stdev message latencies  
***cnt:*** the number of bursts or write messages  
***size:*** the average burst or write size  
***inP:*** the computed bytes in flight per Little's law  
***NetPwr:*** the computed network power (using burst arrival rates)  
***Reads:*** histogram of read sizes  
  
    
**Client side (UDP)**
        
***Write:*** the number of socket write calls  
***Err:*** the number of write syscalls that returned with a non fatal error  
***PPS:*** The sampled packets per second (computed as a derivative)  

**Server side (UDP)**
        
***Write:*** the number of socket write calls  
***Err:*** the number of write syscalls that returned with a non fatal error  
***PPS:*** The sampled packets per second (computed as a derivative)  
  
  
    
     
 --- 

* configure '**--enable-fast-sampling**'

  This configuration causes the iperf binary to support units
  of microseconds. It casues iperf to use four units of precision
  in it's timing interval output, i.e. 1e-4, as one example

>     iperf -c 192.168.1.64 -n 4 -C
>     ------------------------------------------------------------
>     Client connecting to 192.168.1.64, TCP port 5001
>     TCP window size: 85.0 KByte (default)
>     ------------------------------------------------------------
>     [  1] local 192.168.1.133 port 56568 connected with 192.168.1.64 port 5001
>     [ ID] Interval       Transfer     Bandwidth
>     [  1] 0.0000-0.0172 sec  4.00 Bytes  1.86 Kbits/sec

* '**--trip-times**' on the client

  This option indicates to iperf a few things. First, that the user
  has syncrhonized the clients' and servers' clocks. A good way to do
  this is using Precision Time Protocol and a GPS atomic clock as a
  reference. This knowledge allows iperf to use  many time stamps
  to be sender based, i.e. taken from the sender's write timestamp
  (which is carried in the payloads.)

  The connect message on both the server and the client will indicate
  that '--trip-times' has been enabled.

  Both UDP an TCP support '--trip-times'

>     iperf -c 192.168.1.64 --trip-times
>     ------------------------------------------------------------
>     Client connecting to 192.168.1.64, TCP port 5001
>     TCP window size: 85.0 KByte (default)
>     ------------------------------------------------------------
>     [  1] local 192.168.1.133 port 56580 connected with 192.168.1.64 port 5001 (trip-times)
> 
>     iperf -s
>     ------------------------------------------------------------
>     Server listening on TCP port 5001
>     TCP window size:  128 KByte (default)
>     ------------------------------------------------------------
>     [  1] local 192.168.1.64%enp2s0 port 5001 connected with 192.168.1.133 port 56580 (MSS=1448) (trip-times) (sock=4) (peer 2.1.4) on 2021-08-22 11:12:08 (PDT)




----------------------------------------------------------------------
This file is part of iperf 2.

Iperf 2 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Iperf 2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with iperf 2.  If not, see <https://www.gnu.org/licenses/>.