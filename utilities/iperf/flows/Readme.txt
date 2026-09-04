The following steps needs to be done before running pyflows test:

1) Install Python 3.10 and above

2) sudo dnf install python3-matplotlib

3) install gnuplot 'sudo dnf install gnuplot'

4) Update your ".bashrc" and add the location of your flows files "/your_local_dir/iperf2-code/flows" to PYTHONPATH:
export PYTHONPATH=/your_local_dir/iperf2-code/flows:$PYTHONPATH
echo $PYTHONPATH

5) compile the following modules:
cd /your_local_dir/iperf2-code/flows
python3 -m py_compile flows.py  ssh_nodes.py

6) Configure the IP addresses and LAN addresses in the router_latency.py

7) Make sure passwordless ssh is configured for all the ssh DUTs, e.g.

[bm932125@rjm-wifibt-ctrl:/ltf-local/Code/LTF/pyflows/scripts] $ ssh-copy-id -i ~/.ssh/id_rsa.pub root@10.19.85.40
/usr/bin/ssh-copy-id: INFO: Source of key(s) to be installed: "/home/bm932125/.ssh/id_rsa.pub"
/usr/bin/ssh-copy-id: INFO: attempting to log in with the new key(s), to filter out any that are already installed
/usr/bin/ssh-copy-id: INFO: 1 key(s) remain to be installed -- if you are prompted now it is to install the new keys
root@10.19.85.40's password:

Number of key(s) added: 1

Now try logging into the machine, with:   "ssh 'root@10.19.85.40'"
and check to make sure that only the key(s) you wanted were added.

8) Make sure all the wireless devices are loaded and connected to the SSID

9) Run the test:
cd /your_local_dir/iperf2-code/flows
python3 router_latency.py
