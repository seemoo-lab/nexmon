# ----------------------------------------------------------------
# * Copyright (c) 2018-2023
# * Broadcom Corporation
# * All Rights Reserved.
# *---------------------------------------------------------------
# Redistribution and use in source and binary forms, with or without modification, are permitted
# provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this list of conditions
# and the following disclaimer.  Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the documentation and/or other
# materials provided with the distribution.  Neither the name of the Broadcom nor the names of
# contributors may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USEn,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Author Robert J. McMahon, Broadcom LTD
# Date April 2016 - December 2023

import re
import subprocess
import logging
import asyncio, sys
import time, datetime
import locale
import signal
import weakref
import os
import getpass
import math
import scipy
import scipy.spatial
import numpy as np
import tkinter
import ctypes
import ipaddress
import collections
import csv

from datetime import datetime as datetime, timezone
from scipy import stats
from scipy.cluster import hierarchy
from scipy.cluster.hierarchy import linkage
import matplotlib.pyplot as plt
from collections import defaultdict

logger = logging.getLogger(__name__)

class iperf_flow(object):
    port = 61000
    iperf = '/usr/bin/iperf'
    instances = weakref.WeakSet()
    _loop = None
    flow_scope = ("flowstats")
    tasks = []
    flowid2name = defaultdict(str)

    @classmethod
    def get_instances(cls):
        return list(iperf_flow.instances)

    @classmethod
    @property
    def loop(cls):
        if not cls._loop :
            try :
                cls._loop = asyncio.get_running_loop()
            except :
              if os.name == 'nt':
                  # On Windows, the ProactorEventLoop is necessary to listen on pipes
                  cls._loop = asyncio.ProactorEventLoop()
              else:
                  cls._loop = asyncio.new_event_loop()
        return cls._loop


    @classmethod
    def close_loop(cls):
        if iperf_flow.loop.is_running():
            iperf_flow.loop.run_until_complete(loop.shutdown_asyncgens())
            iperf_flow.loop.close()

    @classmethod
    def sleep(cls, time=0, text=None, stoptext=None) :
        if text :
            logging.info('Sleep {} ({})'.format(time, text))
        iperf_flow.loop.run_until_complete(asyncio.sleep(time))
        if stoptext :
            logging.info('Sleep done ({})'.format(stoptext))


    @classmethod
    def run(cls, time=None, amount=None, flows='all', sample_delay=None, io_timer=None, preclean=True, parallel=None, epoch_sync=False) :
        if flows == 'all' :
            flows = iperf_flow.get_instances()
        if not flows:
            logging.warn('flow run method called with no flows instantiated')
            return

        if preclean:
            hosts = [flow.server for flow in flows]
            hosts.extend([flow.client for flow in flows])
            hosts=list(set(hosts))
            tasks = [asyncio.ensure_future(iperf_flow.cleanup(user='root', host=host), loop=iperf_flow.loop) for host in hosts]
            try :
                iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
            except asyncio.TimeoutError:
                logging.error('preclean timeout')
                raise

        logging.info('flow run invoked')
        tasks = [asyncio.ensure_future(flow.rx.start(time=time), loop=iperf_flow.loop) for flow in flows]
        try :
            iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
        except asyncio.TimeoutError:
            logging.error('flow server start timeout')
            raise
        iperf_flow.sleep(time=0.3, text="wait for rx up", stoptext="rx up done")

        if epoch_sync :
            dt = (datetime.now()).timestamp()
            tsec = str(dt).split('.')
            epoch_sync_time = int(tsec[0]) + 2
        else :
            epoch_sync_time = None

        tasks = [asyncio.ensure_future(flow.tx.start(time=time, amount=amount, parallel=parallel, epoch_sync_time=epoch_sync_time), loop=iperf_flow.loop) for flow in flows]

        try :
            iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
        except asyncio.TimeoutError:
            logging.error('flow client start timeout')
            raise
        if sample_delay :
            iperf_flow.sleep(time=0.3, text="ramp up", stoptext="ramp up done")
        if io_timer :
            tasks = [asyncio.ensure_future(flow.is_traffic(), loop=iperf_flow.loop) for flow in flows]
            try :
                iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
            except asyncio.TimeoutError:
                logging.error('flow traffic check timeout')
                raise
        if time :
            iperf_flow.sleep(time=time + 4, text="Running traffic start", stoptext="Stopping flows")
            # Signal the remote iperf client sessions to stop them
            tasks = [asyncio.ensure_future(flow.tx.signal_stop(), loop=iperf_flow.loop) for flow in flows]
            try :
                iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=3))
            except asyncio.TimeoutError:
                logging.error('flow tx stop timeout')
                raise

        elif amount:
            tasks = [asyncio.ensure_future(flow.transmit_completed(), loop=iperf_flow.loop) for flow in flows]
            try :
                iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
            except asyncio.TimeoutError:
                logging.error('flow tx completed timed out')
                raise
            logging.info('flow transmit completed')

        # Now signal the remote iperf server sessions to stop them
        tasks = [asyncio.ensure_future(flow.rx.signal_stop(), loop=iperf_flow.loop) for flow in flows]
        try :
            iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=3))
        except asyncio.TimeoutError:
            logging.error('flow tx stop timeout')
            raise

       # iperf_flow.loop.close()
        logging.info('flow run finished')

    @classmethod
    def commence(cls, time=None, flows='all', sample_delay=None, io_timer=None, preclean=True) :
        if flows == 'all' :
            flows = iperf_flow.get_instances()
        if not flows:
            logging.warn('flow run method called with no flows instantiated')
            return

        if preclean:
            hosts = [flow.server for flow in flows]
            hosts.extend([flow.client for flow in flows])
            hosts=list(set(hosts))
            tasks = [asyncio.ensure_future(iperf_flow.cleanup(user='root', host=host), loop=iperf_flow.loop) for host in hosts]
            try :
                iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
            except asyncio.TimeoutError:
                logging.error('preclean timeout')
                raise

        logging.info('flow start invoked')
        tasks = [asyncio.ensure_future(flow.rx.start(time=time), loop=iperf_flow.loop) for flow in flows]
        try :
            iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
        except asyncio.TimeoutError:
            logging.error('flow server start timeout')
            raise
        iperf_flow.sleep(time=0.3, text="wait for rx up", stoptext="rx up done")
        tasks = [asyncio.ensure_future(flow.tx.start(time=time), loop=iperf_flow.loop) for flow in flows]
        try :
            iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
        except asyncio.TimeoutError:
            logging.error('flow client start timeout')
            raise

    @classmethod
    def plot(cls, flows='all', title='None', directory='None') :
        if flows == 'all' :
            flows = iperf_flow.get_instances()

        tasks = []
        for flow in flows :
            for this_name in flow.histogram_names :
                path = directory + '/' + this_name
                os.makedirs(path, exist_ok=True)
                i = 0
                # group by name
                histograms = [h for h in flow.histograms if h.name == this_name]
                for histogram in histograms :
                    if histogram.ks_index is not None :
                        histogram.output_dir = directory + '/' + this_name + '/' + this_name + str(i)
                    else :
                        histogram.output_dir = directory + '/' + this_name + '/' + this_name + str(histogram.ks_index)

                    logging.info('scheduling task {}'.format(histogram.output_dir))
                    tasks.append(asyncio.ensure_future(histogram.async_plot(directory=histogram.output_dir, title=title), loop=iperf_flow.loop))
                    i += 1
        try :
            logging.info('runnings tasks')
            iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=600))
        except asyncio.TimeoutError:
            logging.error('plot timed out')
            raise


    @classmethod
    def cease(cls, flows='all') :

        if flows == 'all' :
            flows = iperf_flow.get_instances()

        # Signal the remote iperf client sessions to stop them
        tasks = [asyncio.ensure_future(flow.tx.signal_stop(), loop=iperf_flow.loop) for flow in flows]
        try :
            iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
        except asyncio.TimeoutError:
            logging.error('flow tx stop timeout')

        # Now signal the remote iperf server sessions to stop them
        tasks = [asyncio.ensure_future(flow.rx.signal_stop(), loop=iperf_flow.loop) for flow in flows]
        try :
            iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=10))
        except asyncio.TimeoutError:
            logging.error('flow rx stop timeout')

    @classmethod
    async def cleanup(cls, host=None, sshcmd='/usr/bin/ssh', user='root') :
        if host:
            logging.info('ssh {}@{} pkill iperf'.format(user, host))
            childprocess = await asyncio.create_subprocess_exec(sshcmd, '{}@{}'.format(user, host), 'pkill', 'iperf', stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout, _ = await childprocess.communicate()
            if stdout:
                logging.info('cleanup: host({}) stdout={} '.format(host, stdout))

    @classmethod
    def tos_to_txt(cls, tos) :
        switcher = {
            int(0x0)  : "BE",
            int(0x02) : "BK",
            int(0xC0) : "VO",
            int(0x80) : "VI",
        }
        return switcher.get(int(tos), None)

    @classmethod
    def txt_to_tos(cls, txt) :
        switcher = {
            "BE" : "0x0",
            "BESTEFFORT" : "0x0",
            "0x0" : "0x0",
            "BK" : "0x20",
            "BACKGROUND" : "0x20",
            "0x20" : "0x20",
            "VO" : "0xC0",
            "VOICE" : "0xC0",
            "0xC0" : "0xC0",
            "VI" : "0x80",
            "VIDEO" : "0x80",
            "0x80" : "0x80",
        }
        return switcher.get(txt.upper(), None)

    def __init__(self, name='iperf', server=None, client=None, user=None, proto='TCP', dstip='127.0.0.1', interval=1, format='b', offered_load=None, tos='BE', window='4M', src=None, srcip=None, srcport=None, dstport=None,  debug=False, length=None, ipg=0.0, amount=None, trip_times=True, prefetch=None, latency=False, bb=False, working_load=False, bb_period=None, bb_hold=None, txstart_delay_sec=None, burst_size=None, burst_period=None, fullduplex=False, cca=None, tcp_tx_delay=None):
        iperf_flow.instances.add(self)
        self.name = name
        self.latency = latency
        if not dstport :
            iperf_flow.port += 1
            self.dstport = iperf_flow.port
        else:
            self.dstport = dstport
        self.dstip = dstip
        self.srcip = srcip
        self.srcport = srcport
        try :
            self.server = server.ipaddr
        except AttributeError:
            self.server = server
        try :
            self.client = client.ipaddr
        except AttributeError:
            self.client = client

        self.client_device = client.device
        self.server_device = server.device

        if not user :
            self.user = getpass.getuser()
        else :
            self.user = user
        self.proto = proto
        self.tcp_tx_delay = tcp_tx_delay
        self.tos = tos
        if length :
            self.length = length

        if amount :
            self.amount = amount
        if trip_times :
            self.trip_times = trip_times
        if burst_period :
            self.burst_period = burst_period
        if burst_size :
            self.burst_size = burst_size

        if txstart_delay_sec:
            self.txstart_delay_sec = txstart_delay_sec

        if cca:
            self.cca = cca

        self.interval = round(interval,3)
        self.format = format
        self.offered_load = offered_load
        if self.offered_load :
            if len(self.offered_load.split(':')) == 2 :
                self.isoch = True
                self.name += '-isoch'
            else :
                self.isoch = False
        self.prefetch = prefetch
        self.ipg = ipg
        self.debug = debug
        self.TRAFFIC_EVENT_TIMEOUT = round(self.interval * 4, 3)
        self.bb = bb
        self.working_load = working_load
        self.bb_period = bb_period
        self.bb_hold = bb_hold
        self.fullduplex = fullduplex
        # use python composition for the server and client
        # i.e. a flow has a server and a client
        self.rx = iperf_server(name='{}->RX({})'.format(name, str(self.server)), loop=iperf_flow.loop, host=self.server, flow=self, debug=self.debug)
        self.tx = iperf_client(name='{}->TX({})'.format(name, str(self.client)), loop=iperf_flow.loop, host=self.client, flow=self, debug=self.debug)
        self.rx.window=window
        self.tx.window=window
        self.ks_critical_p = 0.01
        self.stats_reset()

    #def __del__(self) :
    #    iperf_flow.instances.remove(self)

    def destroy(self) :
        iperf_flow.instances.remove(self)

    def __getattr__(self, attr) :
        if attr in self.flowstats :
            return self.flowstats[attr]

    def stats_reset(self) :
        # Initialize the flow stats dictionary
        self.flowstats = {'current_rxbytes' : None , 'current_txbytes' : None , 'flowrate' : None, 'starttime' : None, 'flowid' : None, 'endtime' : None}
        self.flowstats['txdatetime']=[]
        self.flowstats['txbytes']=[]
        self.flowstats['txthroughput']=[]
        self.flowstats['writes']=[]
        self.flowstats['errwrites']=[]
        self.flowstats['retry']=[]
        self.flowstats['cwnd']=[]
        self.flowstats['rtt']=[]
        self.flowstats['rxdatetime']=[]
        self.flowstats['rxbytes']=[]
        self.flowstats['rxthroughput']=[]
        self.flowstats['reads']=[]
        self.flowstats['histograms']=[]
        self.flowstats['histogram_names'] = set()
        self.flowstats['connect_time']=[]
        self.flowstats['trip_time']=[]
        self.flowstats['jitter']=[]
        self.flowstats['rxlostpkts']=[]
        self.flowstats['rxtotpkts']=[]
        self.flowstats['meanlat']=[]
        self.flowstats['minlat']=[]
        self.flowstats['maxlat']=[]
        self.flowstats['stdevlat']=[]
        self.flowstats['rxpps']=[]
        self.flowstats['inP']=[]
        self.flowstats['inPvar']=[]
        self.flowstats['rxpkts']=[]
        self.flowstats['netPower']=[]

    async def start(self):
        self.flowstats = {'current_rxbytes' : None , 'current_txbytes' : None , 'flowrate' : None, 'flowid' : None}
        await self.rx.start()
        await self.tx.start()

    async def is_traffic(self) :
        if self.interval < 0.005 :
            logging.warn('{} {}'.format(self.name, 'traffic check invoked without interval sampling'))
        else :
            self.rx.traffic_event.clear()
            self.tx.traffic_event.clear()
            logging.info('{} {}'.format(self.name, 'traffic check invoked'))
            await self.rx.traffic_event.wait()
            await self.tx.traffic_event.wait()

    async def transmit_completed(self) :
        logging.info('{} {}'.format(self.name, 'waiting for transmit to complete'))
        await self.tx.txcompleted.wait()

    async def stop(self):
        self.tx.stop()
        self.rx.stop()

    def stats(self):
        logging.info('stats')

    def compute_ks_table(self, plot=True, directory='.', title=None) :

        if len(self.histogram_names) < 1 :
            tmp = "***Failed. Expected 1 histogram_names, but instead got {0}".format(len(self.histogram_names))
            logging.info(tmp)
            print(tmp)
            #raise

        for this_name in self.histogram_names :
            # group by name
            histograms = [h for h in self.histograms if h.name == this_name]
            for index, h in enumerate(histograms) :
                h.ks_index = index
            tmp = "{} KS Table has {} entries".format(self.name, len(histograms))
            logging.info(tmp)
            print(tmp)

            self.condensed_distance_matrix = ([])

            tasks = []
            for rowindex, h1 in enumerate(histograms) :
                resultstr = rowindex * 'x'
                maxp = None
                minp = None
                for h2 in histograms[rowindex:] :
                    d,p = stats.ks_2samp(h1.samples, h2.samples)
                    if h1 is not h2 :
                        self.condensed_distance_matrix = np.append(self.condensed_distance_matrix,d)
                    logging.debug('D,p={},{} cp={}'.format(str(d),str(p), str(self.ks_critical_p)))
                    if not minp or p < minp :
                        minp = p
                    if not maxp or (p != 1 and p > maxp) :
                        maxp = p
                    if p > self.ks_critical_p :
                        resultstr += '1'
                    else :
                        resultstr += '0'
                    if plot :
                        tasks.append(asyncio.ensure_future(flow_histogram.plot_two_sample_ks(h1=h1, h2=h2, flowname=self.name, title=title, directory=directory), loop=iperf_flow.loop))
                print('KS: {0}({1:3d}):{2} minp={3} ptest={4}'.format(this_name, rowindex, resultstr, str(minp), str(self.ks_critical_p)))
                logging.info('KS: {0}({1:3d}):{2} minp={3} ptest={4}'.format(this_name, rowindex, resultstr, str(minp), str(self.ks_critical_p)))
                if tasks :
                    try :
                        logging.debug('running KS table plotting coroutines for {} row {}'.format(this_name,str(rowindex)))
                        iperf_flow.loop.run_until_complete(asyncio.wait(tasks, timeout=300))
                    except asyncio.TimeoutError:
                        logging.error('plot timed out')
                        raise
            logging.info('{} {}(condensed distance matrix)\n{}'.format(self.name, this_name,self.condensed_distance_matrix))
            self.linkage_matrix=linkage(self.condensed_distance_matrix, 'ward')
            try :
                plt.figure(figsize=(18,10))
                dn = hierarchy.dendrogram(self.linkage_matrix)
                plt.title("{} {}".format(self.name, this_name))
                plt.savefig('{}/dn_{}_{}.png'.format(directory,self.name,this_name))
                logging.info('{} {}(distance matrix)\n{}'.format(self.name, this_name,scipy.spatial.distance.squareform(self.condensed_distance_matrix)))
                print('{} {}(distance matrix)\n{}'.format(self.name, this_name,scipy.spatial.distance.squareform(self.condensed_distance_matrix)))
                print('{} {}(cluster linkage)\n{}'.format(self.name,this_name,self.linkage_matrix))
                logging.info('{} {}(cluster linkage)\n{}'.format(self.name,this_name,self.linkage_matrix))
                flattened=scipy.cluster.hierarchy.fcluster(self.linkage_matrix, 0.75*self.condensed_distance_matrix.max(), criterion='distance')
                print('{} {} Clusters:{}'.format(self.name, this_name, flattened))
                logging.info('{} {} Clusters:{}'.format(self.name, this_name, flattened))
            except:
                pass

    def dump_stats(self, directory='.') :
            logging.info("\n********************** dump_stats for flow {} **********************".format(self.name))

            #logging.info('This flow Name={} id={} items_cnt={}'.format(iperf_flow.flowid2name[self.flowstats['flowid']], str(self.flowstats['flowid']), len(self.flowstats)))
            #logging.info('All flows Name and id: {}'.format(str(iperf_flow.flowid2name)))
            #logging.info('This flow Name={} flowstats={}'.format(self.name, str(self.flowstats)))

            csvfilename = os.path.join(directory, '{}.csv'.format(self.name))
            if not os.path.exists(directory):
                logging.debug('Making results directory {}'.format(directory))
                os.makedirs(directory)

            logging.info("Writing stats to '{}'".format(csvfilename))

            for stat_name in [stat for stat in self.flowstats.keys() if stat != 'histograms'] :
                logging.info("{}={}".format(stat_name, str(self.flowstats[stat_name])))

            with open(csvfilename, 'w', newline='') as fd :
                keynames = self.flowstats.keys()
                writer = csv.writer(fd)
                writer.writerow(keynames)
                writer.writerow([self.flowstats[keyname] for keyname in keynames])
                writer.writerow([h.samples for h in self.flowstats['histograms']])

class iperf_server(object):

    class IperfServerProtocol(asyncio.SubprocessProtocol):
        def __init__(self, server, flow):
            self.__dict__['flow'] = flow
            self._exited = False
            self._closed_stdout = False
            self._closed_stderr = False
            self._mypid = None
            self._server = server
            self._stdoutbuffer = ""
            self._stderrbuffer = ""

        def __setattr__(self, attr, value):
            if attr in iperf_flow.flow_scope:
                self.flow.__setattr__(self.flow, attr, value)
            else:
                self.__dict__[attr] = value

        # methods and attributes not here are handled by the flow object,
        # aka, the flow object delegates to this object per composition
        def __getattr__(self, attr):
            if attr in iperf_flow.flow_scope:
                return getattr(self.flow, attr)

        @property
        def finished(self):
            return self._exited and self._closed_stdout and self._closed_stderr

        def signal_exit(self):
            if not self.finished:
                return
            self._server.closed.set()
            self._server.opened.clear()

        def connection_made(self, trans):
            self._server.closed.clear()
            self._mypid = trans.get_pid()
            logging.debug('server connection made pid=({})'.format(self._mypid))

        def pipe_data_received(self, fd, data):
            if self.debug :
                logging.debug('{} {}'.format(fd, data))
            data = data.decode("utf-8")
            if fd == 1:
                self._stdoutbuffer += data
                while "\n" in self._stdoutbuffer:
                    line, self._stdoutbuffer = self._stdoutbuffer.split("\n", 1)
                    self._server.adapter.info('{} (stdout,{})'.format(line, self._server.remotepid))
                    if not self._server.opened.is_set() :
                        m = self._server.regex_open_pid.match(line)
                        if m :
                            self._server.remotepid = m.group('pid')
                            self._server.opened.set()
                            logging.debug('{} pipe reading (stdout,{})'.format(self._server.name, self._server.remotepid))
                    else :
                        if self._server.proto == 'TCP' :
                            m = self._server.regex_traffic.match(line)
                            if m :
                                timestamp = datetime.now()
                                if not self._server.traffic_event.is_set() :
                                    self._server.traffic_event.set()

                                bytes = float(m.group('bytes'))
                                if self.flowstats['current_txbytes'] :
                                    flowrate = round((bytes / self.flowstats['current_txbytes']), 2)
                                    # *consume* the current *txbytes* where the client pipe will repopulate on its next sample
                                    # do this by setting the value to None
                                    self.flowstats['current_txbytes'] = None
                                    # logging.debug('{} flow  ratio={:.2f}'.format(self._server.name, flowrate))
                                    self.flowstats['flowrate'] = flowrate
                                else :
                                    # *produce* the current *rxbytes* so the client pipe can know this event occurred
                                    # indicate this by setting the value to value
                                    self.flowstats['current_rxbytes'] = bytes
                                    self.flowstats['rxdatetime'].append(timestamp)
                                    self.flowstats['rxbytes'].append(m.group('bytes'))
                                    self.flowstats['rxthroughput'].append(m.group('throughput'))
                                    self.flowstats['reads'].append(m.group('reads'))
                            else :
                                m = self._server.regex_trip_time.match(line)
                                if m :
                                    self.flowstats['trip_time'].append(float(m.group('trip_time')) * 1000)
                        else :
                            m = self._server.regex_traffic_udp.match(line)
                            if m :
                                timestamp = datetime.now()
                                if not self._server.traffic_event.is_set() :
                                    self._server.traffic_event.set()
                                self.flowstats['rxbytes'].append(m.group('bytes'))
                                self.flowstats['rxthroughput'].append(m.group('throughput'))
                                self.flowstats['jitter'].append(m.group('jitter'))
                                self.flowstats['rxlostpkts'].append(m.group('lost_pkts'))
                                self.flowstats['rxtotpkts'].append(m.group('tot_pkts'))
                                self.flowstats['meanlat'].append(m.group('lat_mean'))
                                self.flowstats['minlat'].append(m.group('lat_min'))
                                self.flowstats['maxlat'].append(m.group('lat_max'))
                                self.flowstats['stdevlat'].append(m.group('lat_stdev'))
                                self.flowstats['rxpps'].append(m.group('pps'))
                                self.flowstats['inP'].append(m.group('inP'))
                                self.flowstats['inPvar'].append(m.group('inPvar'))
                                self.flowstats['rxpkts'].append(m.group('pkts'))
                                self.flowstats['netPower'].append(m.group('netPower'))
                        m = self._server.regex_final_histogram_traffic.match(line)
                        if m :
                            timestamp = datetime.now(timezone.utc).astimezone()
                            self.flowstats['endtime']= timestamp
                            self.flowstats['histogram_names'].add(m.group('pdfname'))
                            this_histogram = flow_histogram(name=m.group('pdfname'),values=m.group('pdf'), population=m.group('population'), binwidth=m.group('binwidth'), starttime=self.flowstats['starttime'], endtime=timestamp, outliers=m.group('outliers'), uci=m.group('uci'), uci_val=m.group('uci_val'), lci=m.group('lci'), lci_val=m.group('lci_val'))
                            self.flowstats['histograms'].append(this_histogram)
                            logging.info('pdf {} found with bin width={} us'.format(m.group('pdfname'), m.group('binwidth')))

            elif fd == 2:
                self._stderrbuffer += data
                while "\n" in self._stderrbuffer:
                    line, self._stderrbuffer = self._stderrbuffer.split("\n", 1)
                    logging.info('{} {} (stderr)'.format(self._server.name, line))
                    m = self._server.regex_rx_bind_failed.match(line)
                    if m :
                        logging.error('RX Bind Failed. Check LAN / WLAN between server and client.')
                        iperf_flow.loop.stop()
                        raise

        def pipe_connection_lost(self, fd, exc):
            if fd == 1:
                self._closed_stdout = True
                logging.debug('stdout pipe to {} closed (exception={})'.format(self._server.name, exc))
            elif fd == 2:
                self._closed_stderr = True
                logging.debug('stderr pipe to {} closed (exception={})'.format(self._server.name, exc))
            if self._closed_stdout and self._closed_stderr :
                self.remotepid = None;
            self.signal_exit()

        def process_exited(self):
            logging.debug('subprocess with pid={} closed'.format(self._mypid))
            self._exited = True
            self._mypid = None
            self.signal_exit()

    class CustomAdapter(logging.LoggerAdapter):
        def process(self, msg, kwargs):
            return '[%s] %s' % (self.extra['connid'], msg), kwargs

    def __init__(self, name='Server', loop=None, host='localhost', flow=None, debug=False):
        self.__dict__['flow'] = flow
        self.name = name
        self.iperf = '/usr/local/bin/iperf'
        self.ssh = '/usr/bin/ssh'
        self.host = host
        self.flow = flow
        self.debug = debug
        self.opened = asyncio.Event()
        self.closed = asyncio.Event()
        self.closed.set()
        self.traffic_event = asyncio.Event()
        self._transport = None
        self._protocol = None
        self.time = time
        conn_id = '{}'.format(self.name)
        self.adapter = self.CustomAdapter(logger, {'connid': conn_id})

        # ex. [  4] 0.00-0.50 sec  657090 Bytes  10513440 bits/sec  449    449:0:0:0:0:0:0:0
        self.regex_traffic = re.compile(r'\[\s+\d+] (?P<timestamp>.*) sec\s+(?P<bytes>[0-9]+) Bytes\s+(?P<throughput>[0-9]+) bits/sec\s+(?P<reads>[0-9]+)')
        self.regex_traffic_udp = re.compile(r'\[\s+\d+] (?P<timestamp>.*) sec\s+(?P<bytes>[0-9]+) Bytes\s+(?P<throughput>[0-9]+) bits/sec\s+(?P<jitter>[0-9.]+)\sms\s(?P<lost_pkts>[0-9]+)/(?P<tot_pkts>[0-9]+).+(?P<lat_mean>[0-9.]+)/(?P<lat_min>[0-9.]+)/(?P<lat_max>[0-9.]+)/(?P<lat_stdev>[0-9.]+)\sms\s(?P<pps>[0-9]+)\spps\s+(?P<netPower>[0-9\.]+)\/(?P<inP>[0-9]+)\((?P<inPvar>[0-9]+)\)\spkts\s(?P<pkts>[0-9]+)')
        self.regex_final_histogram_traffic = re.compile(r'\[\s*\d+\] (?P<timestamp>.*) sec\s+(?P<pdfname>[A-Za-z0-9\-]+)\(f\)-PDF: bin\(w=(?P<binwidth>[0-9]+)us\):cnt\((?P<population>[0-9]+)\)=(?P<pdf>.+)\s+\((?P<lci>[0-9\.]+)/(?P<uci>[0-9\.]+)/(?P<uci2>[0-9\.]+)%=(?P<lci_val>[0-9]+)/(?P<uci_val>[0-9]+)/(?P<uci_val2>[0-9]+),Outliers=(?P<outliers>[0-9]+),obl/obu=[0-9]+/[0-9]+\)')
        # 0.0000-0.5259 trip-time (3WHS done->fin+finack) = 0.5597 sec
        self.regex_trip_time = re.compile(r'.+trip\-time\s+\(3WHS\sdone\->fin\+finack\)\s=\s(?P<trip_time>\d+\.\d+)\ssec')
        self.regex_rx_bind_failed = re.compile(r'listener bind failed: Cannot assign requested address')

    def __getattr__(self, attr):
        return getattr(self.flow, attr)

    async def start(self, time=time):
        if not self.closed.is_set() :
            return

        # ex. Server listening on TCP port 61003 with pid 2565
        self.regex_open_pid = re.compile(r'^Server listening on {} port {} with pid (?P<pid>\d+)'.format(self.proto, str(self.dstport)))

        self.opened.clear()
        self.remotepid = None
        if time :
            iperftime = time + 30
            self.sshcmd=[self.ssh, self.user + '@' + self.host, self.iperf, '-s', '-p ' + str(self.dstport), '-P 1', '-e', '-t ' + str(iperftime), '-f{}'.format(self.format), '-w' , self.window, '--realtime']
        else :
            self.sshcmd=[self.ssh, self.user + '@' + self.host, self.iperf, '-s', '-p ' + str(self.dstport), '-P 1', '-e', '-f{}'.format(self.format), '-w' , self.window, '--realtime']
        if self.interval >= 0.005 :
            self.sshcmd.extend(['-i ', str(self.interval)])
        if self.server_device and self.srcip :
            self.sshcmd.extend(['-B ', '{}%{}'.format(self.dstip, self.server_device)])
        if self.proto == 'UDP' :
            self.sshcmd.extend(['-u'])
        if self.latency :
            self.sshcmd.extend(['--histograms=100u,100000,50,95'])
            self.sshcmd.extend(['--jitter-histograms'])

        logging.info('{}'.format(str(self.sshcmd)))
        self._transport, self._protocol = await iperf_flow.loop.subprocess_exec(lambda: self.IperfServerProtocol(self, self.flow), *self.sshcmd)
        await self.opened.wait()

    async def signal_stop(self):
        if self.remotepid and not self.finished :
            childprocess = await asyncio.create_subprocess_exec(self.ssh, '{}@{}'.format(self.user, self.host), 'kill', '-HUP', '{}'.format(self.remotepid), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            logging.debug('({}) sending signal HUP to {} (pid={})'.format(self.user, self.host, self.remotepid))
            stdout, _ = await childprocess.communicate()
            if stdout:
                logging.info('kill remote pid {} {}({}) {}'.format(self.remotepid, self.user, self.host, stdout))
            if not self.closed.is_set() :
                await self.closed.wait()
                logging.info('await kill completed remote pid {} {}({}) {}'.format(self.remotepid, self.user, self.host, stdout))
            logging.info('kill remote pid {} {}({}) {}'.format(self.remotepid, self.user, self.host, stdout))


class iperf_client(object):

    # Asyncio protocol for subprocess transport
    class IperfClientProtocol(asyncio.SubprocessProtocol):
        def __init__(self, client, flow):
            self.__dict__['flow'] = flow
            self._exited = False
            self._closed_stdout = False
            self._closed_stderr = False
            self._mypid = None
            self._client = client
            self._stdoutbuffer = ""
            self._stderrbuffer = ""

        def __setattr__(self, attr, value):
            if attr in iperf_flow.flow_scope:
                self.flow.__setattr__(self.flow, attr, value)
            else:
                self.__dict__[attr] = value

        def __getattr__(self, attr):
            if attr in iperf_flow.flow_scope:
                return getattr(self.flow, attr)

        @property
        def finished(self):
            return self._exited and self._closed_stdout and self._closed_stderr

        def signal_exit(self):
            if not self.finished:
                return
            self._client.closed.set()
            self._client.opened.clear()
            self._client.txcompleted.set()

        def connection_made(self, trans):
            self._client.closed.clear()
            self._mypid = trans.get_pid()
            logging.debug('client connection made pid=({})'.format(self._mypid))

        def pipe_data_received(self, fd, data):
            if self.debug :
                logging.debug('{} {}'.format(fd, data))
            data = data.decode("utf-8")
            if fd == 1:
                self._stdoutbuffer += data
                while "\n" in self._stdoutbuffer:
                    line, self._stdoutbuffer = self._stdoutbuffer.split("\n", 1)
                    self._client.adapter.info('{} (stdout,{})'.format(line, self._client.remotepid))
                    if not self._client.opened.is_set() :
                        m = self._client.regex_open_pid.match(line)
                        if m :
                            self._client.opened.set()
                            self._client.remotepid = m.group('pid')
                            self.flowstats['starttime'] = datetime.now(timezone.utc).astimezone()
                            logging.debug('{} pipe reading at {} (stdout,{})'.format(self._client.name, self.flowstats['starttime'].isoformat(), self._client.remotepid))
                    else :
                        if self.flowstats['flowid'] is None :
                            m = self._client.regex_flowid.match(line)
                            if m :
                                # [  1] local 192.168.1.15%enp1s0 port 7001 connected with 192.168.1.232 port 7001 (trip-times) (sock=3) on 2021-10-11 14:39:45 (PDT)
                                # self.regex_flowid = re.compile(r'local\s(?P<srcip>[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3}).*\sport\s(?P<srcport>[0-9]+)\sconnected with\s(?P<dstip>[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3})\sport\s(?P<dstport>[0-9]+)')
                                #
                                # temp = htonl(config->src_ip);
                                # checksum ^= bcm_compute_xor32((volatile uint32 *)&temp, sizeof(temp) / sizeof(uint32));
                                # temp = htonl(config->dst_ip);
                                # checksum ^= bcm_compute_xor32((volatile uint32 *)&temp, sizeof(temp) / sizeof(uint32));
                                # temp = (hton16(config->dst_port) << 16) | hton16(config->src_port);
                                # checksum ^= bcm_compute_xor32((volatile uint32 *)&temp, sizeof(temp) / sizeof(uint32));
                                # temp = config->proto;
                                # checksum ^= bcm_compute_xor32((volatile uint32 *)&temp, sizeof(temp) / sizeof(uint32));
                                # return "%08x" % netip
                                # NOTE: the network or big endian byte order
                                srcipaddr = ipaddress.ip_address(m.group('srcip'))
                                srcip32 = ctypes.c_uint32(int.from_bytes(srcipaddr.packed, byteorder='little', signed=False))
                                dstipaddr = ipaddress.ip_address(m.group('dstip'))
                                dstip32 = ctypes.c_uint32(int.from_bytes(dstipaddr.packed, byteorder='little', signed=False))
                                dstportbytestr = int(m.group('dstport')).to_bytes(2, byteorder='big', signed=False)
                                dstport16 = ctypes.c_uint16(int.from_bytes(dstportbytestr, byteorder='little', signed=False))
                                srcportbytestr = int(m.group('srcport')).to_bytes(2, byteorder='big', signed=False)
                                srcport16 = ctypes.c_uint16(int.from_bytes(srcportbytestr, byteorder='little', signed=False))
                                ports32 = ctypes.c_uint32((dstport16.value << 16) | srcport16.value)
                                if self._client.proto == 'UDP':
                                    proto32 = ctypes.c_uint32(0x11)
                                else :
                                    proto32 = ctypes.c_uint32(0x06)
                                quintuplehash = srcip32.value ^ dstip32.value ^ ports32.value ^ proto32.value
                                self.flowstats['flowid'] = '0x{:08x}'.format(quintuplehash)
                                if self._client.flow.name :
                                    flowkey = self._client.flow.name
                                else :
                                    flowkey = '0x{:08x}'.format(quintuplehash)
                                iperf_flow.flowid2name[self.flowstats['flowid']] = flowkey
                                logging.info('Flow quintuple hash of {} uses name {}'.format(self.flowstats['flowid'], flowkey))

                        if self._client.proto == 'TCP':
                            m = self._client.regex_traffic.match(line)
                            if m :
                                timestamp = datetime.now()
                                if not self._client.traffic_event.is_set() :
                                    self._client.traffic_event.set()

                                bytes = float(m.group('bytes'))
                                if self.flowstats['current_rxbytes'] :
                                    flowrate = round((self.flowstats['current_rxbytes'] / bytes), 2)
                                    # *consume* the current *rxbytes* where the server pipe will repopulate on its next sample
                                    # do this by setting the value to None
                                    self.flowstats['current_rxbytes'] = None
                                    # logging.debug('{} flow ratio={:.2f}'.format(self._client.name, flowrate))
                                    self.flowstats['flowrate'] = flowrate
                                else :
                                    # *produce* the current txbytes so the server pipe can know this event occurred
                                    # indicate this by setting the value to value
                                    self.flowstats['current_txbytes'] = bytes

                                self.flowstats['txdatetime'].append(timestamp)
                                self.flowstats['txbytes'].append(m.group('bytes'))
                                self.flowstats['txthroughput'].append(m.group('throughput'))
                                self.flowstats['writes'].append(m.group('writes'))
                                self.flowstats['errwrites'].append(m.group('errwrites'))
                                self.flowstats['retry'].append(m.group('retry'))
                                self.flowstats['cwnd'].append(m.group('cwnd'))
                                self.flowstats['rtt'].append(m.group('rtt'))
                            else :
                                m = self._client.regex_connect_time.match(line)
                                if m :
                                    self.flowstats['connect_time'].append(float(m.group('connect_time')))
                        else :
                            pass

            elif fd == 2:
                self._stderrbuffer += data
                while "\n" in self._stderrbuffer:
                    line, self._stderrbuffer = self._stderrbuffer.split("\n", 1)
                    logging.info('{} {} (stderr)'.format(self._client.name, line))
                    m = self._client.regex_tx_bind_failed.match(line)
                    if m :
                        logging.error('TX Bind Failed. Check LAN / WLAN between server and client.')
                        iperf_flow.loop.stop()
                        raise

        def pipe_connection_lost(self, fd, exc):
            if fd == 1:
                logging.debug('stdout pipe to {} closed (exception={})'.format(self._client.name, exc))
                self._closed_stdout = True
            elif fd == 2:
                logging.debug('stderr pipe to {} closed (exception={})'.format(self._client.name, exc))
                self._closed_stderr = True
            self.signal_exit()

        def process_exited(self):
            logging.debug('subprocess with pid={} closed'.format(self._mypid))
            self._exited = True
            self._mypid = None
            self.signal_exit()

    class CustomAdapter(logging.LoggerAdapter):
        def process(self, msg, kwargs):
            return '[%s] %s' % (self.extra['connid'], msg), kwargs

    def __init__(self, name='Client', loop=None, host='localhost', flow = None, debug=False):
        self.__dict__['flow'] = flow
        self.opened = asyncio.Event()
        self.closed = asyncio.Event()
        self.txcompleted = asyncio.Event()
        self.closed.set()
        self.txcompleted.clear()
        self.traffic_event = asyncio.Event()
        self.name = name
        self.iperf = '/usr/local/bin/iperf'
        self.ssh = '/usr/bin/ssh'
        self.host = host
        self.debug = debug
        self.flow = flow
        self._transport = None
        self._protocol = None
        conn_id = '{}'.format(self.name)
        self.adapter = self.CustomAdapter(logger, {'connid': conn_id})
        # traffic ex: [  3] 0.00-0.50 sec  655620 Bytes  10489920 bits/sec  14/211        446      446K/0 us
        self.regex_traffic = re.compile(r'\[\s+\d+] (?P<timestamp>.*) sec\s+(?P<bytes>\d+) Bytes\s+(?P<throughput>\d+) bits/sec\s+(?P<writes>\d+)/(?P<errwrites>\d+)\s+(?P<retry>\d+)\s+(?P<cwnd>\d+)K/(?P<rtt>\d+) us')
        self.regex_connect_time = re.compile(r'\[\s+\d+]\slocal.*\(ct=(?P<connect_time>\d+\.\d+) ms\)')
        # local 192.168.1.4 port 56949 connected with 192.168.1.1 port 61001
        self.regex_flowid = re.compile(r'\[\s+\d+]\slocal\s(?P<srcip>[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3}).*\sport\s(?P<srcport>[0-9]+)\sconnected with\s(?P<dstip>[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3})\sport\s(?P<dstport>[0-9]+)')
        self.regex_tx_bind_failed = re.compile(r'bind failed: Cannot assign requested address')

    def __getattr__(self, attr):
        return getattr(self.flow, attr)

    async def start(self, time=None, amount=None, parallel=None, epoch_sync_time=None):
        if not self.closed.is_set() :
            return

        self.opened.clear()
        self.txcompleted.clear()
        self.remotepid = None
        self.flowstats['flowid']=None

        # Client connecting to 192.168.100.33, TCP port 61009 with pid 1903
        self.regex_open_pid = re.compile(r'Client connecting to .*, {} port {} with pid (?P<pid>\d+)'.format(self.proto, str(self.dstport)))
        if self.client_device :
            client_dst = self.dstip + '%' + self.client_device
        else :
            client_dst = self.dstip
        self.sshcmd=[self.ssh, self.user + '@' + self.host, self.iperf, '-c', client_dst, '-p ' + str(self.dstport), '-e', '-f{}'.format(self.format), '-w' , self.window ,'--realtime']
        if self.tcp_tx_delay :
            self.sshcmd.extend(['--tcp-tx-delay', self.tcp_tx_delay])
        if self.tos :
            self.sshcmd.extend(['-S ', self.tos])
        if self.length :
            self.sshcmd.extend(['-l ', str(self.length)])
        if time:
            self.sshcmd.extend(['-t ', str(time)])
        elif amount:
            iperftime = time
            self.sshcmd.extend(['-n ',  amount])
        if parallel :
            self.sshcmd.extend(['-P', str(parallel)])
        if self.trip_times :
            self.sshcmd.extend(['--trip-times'])
        if self.prefetch :
            self.sshcmd.extend(['--tcp-write-prefetch', self.prefetch])
            self.sshcmd.extend(['--histograms=1m,100000,5,95'])

        if self.srcip :
            if self.srcport :
                self.sshcmd.extend(['-B ', '{}:{}'.format(self.srcip, self.srcport)])
            else :
                self.sshcmd.extend(['-B {}'.format(self.srcip)])

        if self.cca :
            self.sshcmd.extend(['-Z ', self.cca])
        if self.interval >= 0.005 :
            self.sshcmd.extend(['-i ', str(self.interval)])

        if self.proto == 'UDP' :
            self.sshcmd.extend(['-u '])
            if self.isoch :
                self.sshcmd.extend(['--isochronous=' + self.offered_load, ' --ipg ', str(self.ipg)])
            elif self.offered_load :
                self.sshcmd.extend(['-b', self.offered_load])
        elif self.proto == 'TCP' and self.offered_load :
            self.sshcmd.extend(['-b', self.offered_load])
        elif self.proto == 'TCP' and self.burst_size and self.burst_period :
            self.sshcmd.extend(['--burst-size', str(self.burst_size)])
            self.sshcmd.extend(['--burst-period', str(self.burst_period)])
        elif self.proto == 'TCP' and self.bb :
            self.sshcmd.extend(['--bounceback'])
            self.sshcmd.extend(['--bounceback-hold', str(self.bb_hold)])
            self.sshcmd.extend(['--bounceback-period', str(self.bb_period)])
        elif self.proto == 'TCP' and self.offered_load :
            self.sshcmd.extend(['-b', self.offered_load])
        if not self.bb and self.fullduplex :
            self.sshcmd.extend(['--full-duplex', str(" ")])

        if self.flow.bb :
            self.sshcmd.extend(['--bounceback'])
            if self.flow.working_load :
                self.sshcmd.extend(['--working-load'])

        if epoch_sync_time :
            self.sshcmd.extend(['--txstart-time', str(epoch_sync_time)])

        elif self.txstart_delay_sec :
            # use incoming txstart_delay_sec and convert it to epoch_time_sec to use with '--txstart-time' iperf parameter
            logging.info('{}'.format(str(datetime.now())))
            epoch_time_sec = (datetime.now()).timestamp()
            logging.info('Current epoch_time_sec = {}'.format(str(epoch_time_sec)))
            new_txstart_time = epoch_time_sec + self.txstart_delay_sec
            logging.info('new_txstart_time = {}'.format(str(new_txstart_time)))
            self.sshcmd.extend(['--txstart-time', str(new_txstart_time)])

        logging.info('{}'.format(str(self.sshcmd)))
        try :
            self._transport, self._protocol = await iperf_flow.loop.subprocess_exec(lambda: self.IperfClientProtocol(self, self.flow), *self.sshcmd)
            await self.opened.wait()
        except:
            logging.error('flow client start error per: {}'.format(str(self.sshcmd)))
            pass

    async def signal_stop(self):
        if self.remotepid and not self.finished :
            childprocess = await asyncio.create_subprocess_exec(self.ssh, '{}@{}'.format(self.user, self.host), 'kill', '-HUP', '{}'.format(self.remotepid), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            logging.debug('({}) sending signal HUP to {} (pid={})'.format(self.user, self.host, self.remotepid))
            stdout, _ = await childprocess.communicate()
            if stdout:
                logging.info('{}({}) {}'.format(self.user, self.host, stdout))
            if not self.closed.is_set():
                await self.closed.wait()

    async def signal_pause(self):
        if self.remotepid :
            childprocess = await asyncio.create_subprocess_exec(self.ssh, '{}@{}'.format(self.user, self.host), 'kill', '-STOP', '{}'.format(self.remotepid), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            logging.debug('({}) sending signal STOP to {} (pid={})'.format(self.user, self.host, self.remotepid))
            stdout, _ = await childprocess.communicate()
            if stdout:
                logging.info('{}({}) {}'.format(self.user, self.host, stdout))
            if not self.closed.is_set():
                await self.closed.wait()

    async def signal_resume(self):
        if self.remotepid :
            childprocess = await asyncio.create_subprocess_exec(self.ssh, '{}@{}'.format(self.user, self.host), 'kill', '-CONT', '{}'.format(self.remotepid), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            logging.debug('({}) sending signal CONT to {} (pid={})'.format(self.user, self.host, self.remotepid))
            stdout, _ = await childprocess.communicate()
            if stdout:
                logging.info('{}({}) {}'.format(self.user, self.host, stdout))
            if not self.closed.is_set():
                await self.closed.wait()

class flow_histogram(object):

    @classmethod
    async def plot_two_sample_ks(cls, h1=None, h2=None, outputtype='png', directory='.', flowname=None, title=None):

        lci_val = int(h2.lci_val) * h2.binwidth
        uci_val = int(h2.uci_val) * h2.binwidth
        mytitle = '{} {} two sample KS({},{}) ({} samples) {}/{}%={}/{} us outliers={}\\n{}'.format(flowname, h1.name, h1.ks_index, h2.ks_index, h2.population, h2.lci, h2.uci, lci_val, uci_val, h2.outliers, title)
        if h1.basefilename is None :
            h1.output_dir = directory + '/' + flowname + h1.name + '/' + h1.name + '_' + str(h1.ks_index)
            await h1.write(directory=h1.output_dir)

        if h2.basefilename is None :
            h2.output_dir = directory + '/' + flowname + h2.name + '/' + h2.name + '_' + str(h2.ks_index)
            await h2.write(directory=h2.output_dir)

        if (h1.basefilename is not None) and (h2.basefilename is not None) :
            basefilename = '{}_{}_{}'.format(h1.basefilename, h1.ks_index, h2.ks_index)
            gpcfilename = basefilename + '.gpc'
            #write out the gnuplot control file
            with open(gpcfilename, 'w') as fid :
                if outputtype == 'canvas' :
                    fid.write('set output \"{}.{}\"\n'.format(basefilename, 'html'))
                    fid.write('set terminal canvas standalone mousing size 1024,768\n')
                if outputtype == 'svg' :
                    fid.write('set output \"{}_svg.{}\"\n'.format(basefilename, 'html'))
                    fid.write('set terminal svg size 1024,768 dynamic mouse\n')
                else :
                    fid.write('set output \"{}.{}\"\n'.format(basefilename, 'png'))
                    fid.write('set terminal png size 1024,768\n')

                fid.write('set key bottom\n')
                fid.write('set title \"{}\" noenhanced\n'.format(mytitle))
                if float(uci_val) < 400:
                    fid.write('set format x \"%.2f"\n')
                else :
                    fid.write('set format x \"%.1f"\n')
                fid.write('set format y \"%.1f"\n')
                fid.write('set yrange [0:1.01]\n')
                fid.write('set y2range [0:*]\n')
                fid.write('set ytics add 0.1\n')
                fid.write('set y2tics nomirror\n')
                fid.write('set grid\n')
                fid.write('set xlabel \"time (ms)\\n{} - {}\"\n'.format(h1.starttime, h2.endtime))
                default_minx = -0.5
                if float(uci_val) < 0.4:
                    fid.write('set xrange [{}:0.4]\n'.format(default_minx))
                    fid.write('set xtics auto\n')
                elif h1.max < 2.0 and h2.max < 2.0 :
                    fid.write('set xrange [{}:2]\n'.format(default_minx))
                    fid.write('set xtics auto\n')
                elif h1.max < 5.0 and h2.max < 5.0 :
                    fid.write('set xrange [{}:5]\n'.format(default_minx))
                    fid.write('set xtics auto\n')
                elif h1.max < 10.0 and h2.max < 10.0:
                    fid.write('set xrange [{}:10]\n'.format(default_minx))
                    fid.write('set xtics add 1\n')
                elif h1.max < 20.0 and h2.max < 20.0 :
                    fid.write('set xrange [{}:20]\n'.format(default_minx))
                    fid.write('set xtics add 1\n')
                    fid.write('set format x \"%.0f"\n')
                elif h1.max < 40.0 and h2.max < 40.0:
                    fid.write('set xrange [{}:40]\n'.format(default_minx))
                    fid.write('set xtics add 5\n')
                    fid.write('set format x \"%.0f"\n')
                elif h1.max < 50.0 and h2.max < 50.0:
                    fid.write('set xrange [{}:50]\n'.format(default_minx))
                    fid.write('set xtics add 5\n')
                    fid.write('set format x \"%.0f"\n')
                elif h1.max < 75.0 and h2.max < 75.0:
                    fid.write('set xrange [{}:75]\n'.format(default_minx))
                    fid.write('set xtics add 5\n')
                    fid.write('set format x \"%.0f"\n')
                elif h1.max < 100.0 and h2.max < 100.0 :
                    fid.write('set xrange [{}:100]\n'.format(default_minx))
                    fid.write('set xtics add 10\n')
                    fid.write('set format x \"%.0f"\n')
                else :
                    fid.write('set xrange [{}:*]\n'.format(default_minx))
                    fid.write('set xtics auto\n')
                    fid.write('set format x \"%.0f"\n')
                fid.write('plot \"{0}\" using 1:2 index 0 axes x1y2 with impulses linetype 3 notitle,  \"{1}\" using 1:2 index 0 axes x1y2 with impulses linetype 2 notitle, \"{1}\" using 1:3 index 0 axes x1y1 with lines linetype 1 linewidth 2 notitle, \"{0}\" using 1:3 index 0 axes x1y1 with lines linetype -1 linewidth 2 notitle\n'.format(h1.datafilename, h2.datafilename))

            childprocess = await asyncio.create_subprocess_exec(flow_histogram.gnuplot,gpcfilename, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout, stderr = await childprocess.communicate()
            if stderr :
                logging.error('Exec {} {}'.format(flow_histogram.gnuplot, gpcfilename))
            else :
                logging.debug('Exec {} {}'.format(flow_histogram.gnuplot, gpcfilename))

    gnuplot = '/usr/bin/gnuplot'
    def __init__(self, binwidth=None, name=None, values=None, population=None, starttime=None, endtime=None, title=None, outliers=None, lci = None, uci = None, lci_val = None, uci_val = None) :
        self.raw = values
        self._entropy = None
        self._ks_1samp_dist = None
        self.bins = self.raw.split(',')
        self.name = name
        self.ks_index = None
        self.population = int(population)
        self.samples = np.zeros(int(self.population))
        self.binwidth = int(binwidth)
        self.createtime = datetime.now(timezone.utc).astimezone()
        self.starttime=starttime
        self.endtime=endtime
        self.title=title
        self.outliers=outliers
        self.uci = uci
        self.uci_val = uci_val
        self.lci = lci
        self.lci_val = lci_val
        self.basefilename = None
        ix = 0
        for bin in self.bins :
            x,y = bin.split(':')
            for i in range(int(y)) :
                self.samples[ix] = x
                ix += 1

    @property
    def entropy(self) :
        if not self._entropy :
            self._entropy = 0
            for bin in self.bins :
                x,y = bin.split(':')
                y1 = float(y) / float(self.population)
                self._entropy -= y1 * math.log2(y1)
        return self._entropy

    @property
    def ks_1samp_dist(self):
        if not self._ks_1samp_dist :
            self._ks_1samp_dist,p = stats.ks_1samp(self.samples, stats.norm.cdf)
        return self._ks_1samp_dist

    @property
    def ampdu_dump(self) :
        return self._ampdu_rawdump

    @ampdu_dump.setter
    def ampdu_dump(self, value):
        self._ampdu_rawdump = value

    async def __exec_gnuplot(self) :
        logging.info('Plotting {} {}'.format(self.name, self.gpcfilename))
        childprocess = await asyncio.create_subprocess_exec(flow_histogram.gnuplot, self.gpcfilename, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        stdout, stderr = await childprocess.communicate()
        if stderr :
            logging.error('Exec {} {}'.format(flow_histogram.gnuplot, self.gpcfilename))
        else  :
            logging.debug('Exec {} {}'.format(flow_histogram.gnuplot, self.gpcfilename))

    async def write(self, directory='.', filename=None) :
        # write out the datafiles for the plotting tool,  e.g. gnuplot
        if filename is None:
            filename = self.name

        if not os.path.exists(directory):
            logging.debug('Making results directory {}'.format(directory))
            os.makedirs(directory)

        logging.debug('Writing {} results to directory {}'.format(directory, filename))
        basefilename = os.path.join(directory, filename)
        datafilename = os.path.join(directory, filename + '.data')
        self.max  = None
        with open(datafilename, 'w') as fid :
            cummulative = 0.0
            for bin in self.bins :
                x,y = bin.split(':')
                #logging.debug('bin={} x={} y={}'.format(bin, x, y))
                if (float(y) > 1.0) or ((cummulative / float(self.population)) < 0.99) :
                    cummulative += float(y)
                    perc = cummulative / float(self.population)
                    self.max = float(x) * float(self.binwidth) / 1000.0 # max is the last value
                    fid.write('{} {} {}\n'.format((float(x) * float(self.binwidth) / 1000.0), int(y), perc))

        self.basefilename = basefilename
        self.datafilename = datafilename

    async def async_plot(self, title=None, directory='.', outputtype='png', filename=None) :
        if self.basefilename is None :
            await self.write(directory=directory, filename=filename)

        if self.basefilename is not None :
            self.gpcfilename = self.basefilename + '.gpc'
            #write out the gnuplot control file
            with open(self.gpcfilename, 'w') as fid :
                if outputtype == 'canvas' :
                    fid.write('set output \"{}.{}\"\n'.format(basefilename, 'html'))
                    fid.write('set terminal canvas standalone mousing size 1024,768\n')
                if outputtype == 'svg' :
                    fid.write('set output \"{}_svg.{}\"\n'.format(basefilename, 'html'))
                    fid.write('set terminal svg size 1024,768 dynamic mouse\n')
                else :
                    fid.write('set output \"{}.{}\"\n'.format(basefilename, 'png'))
                    fid.write('set terminal png size 1024,768\n')

                if not title and self.title :
                    title = self.title

                fid.write('set key bottom\n')
                if self.ks_index is not None :
                    fid.write('set title \"{}({}) {}({}) E={}\" noenhanced\n'.format(self.name, str(self.ks_index), title, int(self.population), self.entropy))
                else :
                    fid.write('set title \"{}{}({}) E={}\" noenhanced\n'.format(self.name, title, int(self.population), self.entropy))
                fid.write('set format x \"%.0f"\n')
                fid.write('set format y \"%.1f"\n')
                fid.write('set yrange [0:1.01]\n')
                fid.write('set y2range [0:*]\n')
                fid.write('set ytics add 0.1\n')
                fid.write('set y2tics nomirror\n')
                fid.write('set grid\n')
                fid.write('set xlabel \"time (ms)\\n{} - {}\"\n'.format(self.starttime, self.endtime))
                if self.max < 5.0 :
                    fid.write('set xrange [0:5]\n')
                    fid.write('set xtics auto\n')
                elif self.max < 10.0 :
                    fid.write('set xrange [0:10]\n')
                    fid.write('set xtics add 1\n')
                elif self.max < 20.0 :
                    fid.write('set xrange [0:20]\n')
                    fid.write('set xtics add 1\n')
                elif self.max < 40.0 :
                    fid.write('set xrange [0:40]\n')
                    fid.write('set xtics add 5\n')
                elif self.max < 50.0 :
                    fid.write('set xrange [0:50]\n')
                    fid.write('set xtics add 5\n')
                elif self.max < 75.0 :
                    fid.write('set xrange [0:75]\n')
                    fid.write('set xtics add 5\n')
                else :
                    fid.write('set xrange [0:100]\n')
                    fid.write('set xtics add 10\n')
                fid.write('plot \"{0}\" using 1:2 index 0 axes x1y2 with impulses linetype 3 notitle, \"{0}\" using 1:3 index 0 axes x1y1 with lines linetype -1 linewidth 2 notitle\n'.format(datafilename))

                if outputtype == 'png' :
                    # Create a thumbnail too
                    fid.write('unset output; unset xtics; unset ytics; unset key; unset xlabel; unset ylabel; unset border; unset grid; unset yzeroaxis; unset xzeroaxis; unset title; set lmargin 0; set rmargin 0; set tmargin 0; set bmargin 0\n')
                    fid.write('set output \"{}_thumb.{}\"\n'.format(basefilename, 'png'))
                    fid.write('set terminal png transparent size 64,32 crop\n')
                    fid.write('plot \"{0}\" using 1:2 index 0 axes x1y2 with impulses linetype 3 notitle, \"{0}\" using 1:3 index 0 axes x1y1 with lines linetype -1 linewidth 2 notitle\n'.format(datafilename))

            await self.__exec_gnuplot()
