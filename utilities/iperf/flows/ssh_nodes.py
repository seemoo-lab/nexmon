# ---------------------------------------------------------------
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
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Author Robert J. McMahon, Broadcom LTD
#
# Python object to support sending remote commands to a host
#
# Date April 2018 - December 2023

import logging
import asyncio, subprocess
import time, datetime
import weakref
import os
import re

from datetime import datetime as datetime, timezone

logger = logging.getLogger(__name__)

class ssh_node:
    DEFAULT_IO_TIMEOUT = 30.0
    DEFAULT_CMD_TIMEOUT = 30
    DEFAULT_CONNECT_TIMEOUT = 60.0
    rexec_tasks = []
    _loop = None
    instances = weakref.WeakSet()
    periodic_cmd_futures = []
    periodic_cmd_running_event = asyncio.Event()
    periodic_cmd_done_event = asyncio.Event()

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
    def sleep(cls, time=0, text=None, stoptext=None) :
        if text :
            logging.info('Sleep {} ({})'.format(time, text))
        ssh_node.loop.run_until_complete(asyncio.sleep(time))
        if stoptext :
            logging.info('Sleep done ({})'.format(stoptext))

    @classmethod
    def get_instances(cls):
        try :
            return list(ssh_node.instances)
        except NameError :
            return []

    @classmethod
    def run_all_commands(cls, timeout=None, text=None, stoptext=None) :
        if ssh_node.rexec_tasks :
            if text :
                logging.info('Run all tasks: {})'.format(time, text))
            ssh_node.loop.run_until_complete(asyncio.wait(ssh_node.rexec_tasks, timeout=timeout))
            if stoptext :
                logging.info('Commands done ({})'.format(stoptext))
            ssh_node.rexec_tasks = []

    @classmethod
    def open_consoles(cls, silent_mode=False) :
        nodes = ssh_node.get_instances()
        node_names = []
        tasks = []
        for node in nodes:
            if node.sshtype.lower() == 'ssh' :
                tasks.append(asyncio.ensure_future(node.clean(), loop=ssh_node.loop))
        if tasks :
            logging.info('Run consoles clean')
            try :
                ssh_node.loop.run_until_complete(asyncio.wait(tasks, timeout=20))
            except asyncio.TimeoutError:
                logging.error('console cleanup timeout')

        tasks = []
        ipaddrs = []
        for node in nodes :
            #see if we need control master to be started
            if node.ssh_speedups and not node.ssh_console_session and node.ipaddr not in ipaddrs:
                logging.info('Run consoles speedup')
                node.ssh_console_session = ssh_session(name=node.name, hostname=node.ipaddr, node=node, control_master=True, ssh_speedups=True, silent_mode=silent_mode)
                node.console_task = asyncio.ensure_future(node.ssh_console_session.post_cmd(cmd='/usr/bin/dmesg -w', IO_TIMEOUT=None, CMD_TIMEOUT=None), loop=ssh_node.loop)
                tasks.append(node.console_task)
                ipaddrs.append(node.ipaddr)
                node_names.append(node.name)

        if tasks :
            s = " "
            logging.info('Opening consoles: {}'.format(s.join(node_names)))
            try :
                ssh_node.loop.run_until_complete(asyncio.wait(tasks, timeout=60))
            except asyncio.TimeoutError:
                logging.error('open console timeout')
                raise

        if tasks :
            # Sleep to let the control masters settle
            ssh_node.loop.run_until_complete(asyncio.sleep(1))
            logging.info('open_consoles done')

    @classmethod
    def close_consoles(cls) :
        nodes = ssh_node.get_instances()
        tasks = []
        node_names = []
        for node in nodes :
            if node.ssh_console_session :
                node.console_task = asyncio.ensure_future(node.ssh_console_session.close(), loop=ssh_node.loop)
                tasks.append(node.console_task)
                node_names.append(node.name)

        if tasks :
            s = " "
            logging.info('Closing consoles: {}'.format(s.join(node_names)))
            ssh_node.loop.run_until_complete(asyncio.wait(tasks, timeout=60))
            logging.info('Closing consoles done: {}'.format(s.join(node_names)))

    @classmethod
    def periodic_cmds_stop(cls) :
        logging.info("Stop periodic futures")
        ssh_node.periodic_cmd_futures = []
        ssh_node.periodic_cmd_running_event.clear()
        while not ssh_node.periodic_cmd_done_event.is_set() :
            ssh_node.loop.run_until_complete(asyncio.sleep(0.25))
            logging.debug("Awaiting kill periodic futures")
        logging.debug("Stop periodic futures done")

    def __init__(self, name=None, ipaddr=None, devip=None, console=False, device=None, ssh_speedups=False, silent_mode=False, sshtype='ssh', relay=None):
        self.ipaddr = ipaddr
        self.name = name
        self.my_futures = []
        self.device = device
        self.devip = devip
        self.sshtype = sshtype.lower()
        if self.sshtype.lower() == 'ssh' :
            self.ssh_speedups = ssh_speedups
            self.controlmasters = '/tmp/controlmasters_{}'.format(self.ipaddr)
        else :
            self.ssh_speedups = False
            self.controlmasters = None
        self.ssh_console_session = None

        if relay :
            self.relay = relay
            self.ssh = ['/usr/bin/ssh', 'root@{}'.format(relay)]
        else :
            self.ssh = []
        if self.sshtype.lower() == 'ush' :
            self.ssh.extend(['/usr/local/bin/ush'])
        elif self.sshtype.lower() == 'ssh' :
            if not self.ssh :
                logging.debug("node add /usr/bin/ssh")
                self.ssh.extend(['/usr/bin/ssh'])
            logging.debug("ssh={} ".format(self.ssh))
        else :
            raise ValueError("ssh type invalid")

        ssh_node.instances.add(self)

    def rexec(self, cmd='pwd', IO_TIMEOUT=DEFAULT_IO_TIMEOUT, CMD_TIMEOUT=DEFAULT_CMD_TIMEOUT, CONNECT_TIMEOUT=DEFAULT_CONNECT_TIMEOUT, run_now=True, repeat = None) :
        io_timer = IO_TIMEOUT
        cmd_timer = CMD_TIMEOUT
        connect_timer = CONNECT_TIMEOUT
        this_session = ssh_session(name=self.name, hostname=self.ipaddr, CONNECT_TIMEOUT=connect_timer, node=self, ssh_speedups=True)
        this_future = asyncio.ensure_future(this_session.post_cmd(cmd=cmd, IO_TIMEOUT=io_timer, CMD_TIMEOUT=cmd_timer, repeat = repeat), loop=ssh_node.loop)
        if run_now:
            ssh_node.loop.run_until_complete(asyncio.wait([this_future], timeout=CMD_TIMEOUT))
        else:
            ssh_node.rexec_tasks.append(this_future)
            self.my_futures.append(this_future)
        return this_session

    async def clean(self) :
        childprocess = await asyncio.create_subprocess_exec('/usr/bin/ssh', 'root@{}'.format(self.ipaddr), 'pkill', 'dmesg', stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        stdout, stderr = await childprocess.communicate()
        if stdout :
            logging.info('{}'.format(stdout))
        if stderr :
            logging.info('{}'.format(stderr))

    def close_console(self) :
        if self.ssh_console_session:
            self.ssh_console_session.close()

    async def repeat(self, interval, func, *args, **kwargs):
        """
        Run func every interval seconds.
        If func has not finished before *interval*, will run again
        immediately when the previous iteration finished.
        *args and **kwargs are passed as the arguments to func.
        """
        logging.debug("repeat args={} kwargs={}".format(args, kwargs))

        while ssh_node.periodic_cmd_running_event.is_set() :
            await asyncio.gather(
                func(*args, **kwargs),
                asyncio.sleep(interval),
            )
            if interval == 0 :
                break

        logging.debug("Closing log_fh={}".format(kwargs['log_fh']))
        kwargs['log_fh'].flush()
        kwargs['log_fh'].close()
        ssh_node.periodic_cmd_done_event.set()

    def periodic_cmd_enable(self, cmd='ls', time_period=None, cmd_log_file=None) :
        log_file_handle = open(cmd_log_file, 'w', errors='ignore')

        if ssh_node.loop :
            future = asyncio.ensure_future(self.repeat(time_period, self.run_cmd, cmd=cmd, log_fh=log_file_handle), loop=ssh_node.loop)
            ssh_node.periodic_cmd_futures.append(future)
            ssh_node.periodic_cmd_running_event.set()
            ssh_node.periodic_cmd_done_event.clear()
        else :
            raise

    async def run_cmd(self, *args, **kwargs) :
        log_file_handle = kwargs['log_fh']
        cmd = kwargs['cmd']

        msg = "********************** Periodic Command '{}' Begins **********************".format(cmd)
        logging.info(msg)
        t = '%s' % datetime.now()
        t = t[:-3] + " " + str(msg)
        log_file_handle.write(t + '\n')
        logging.debug("ssh={} ipaddr={} cmd={} ".format(self.ssh, self.ipaddr, cmd))
        this_cmd = []
        ush_flag = False

        for item in self.ssh:
            if 'ush' in item :
                ush_flag = True

        if ush_flag :
            this_cmd.extend([*self.ssh, self.ipaddr, cmd])
        else:
            this_cmd.extend([*self.ssh, 'root@{}'.format(self.ipaddr), cmd])
        logging.info("run cmd = {}".format(this_cmd))

        childprocess = await asyncio.create_subprocess_exec(*this_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        logging.info("subprocess for periodic cmd = {}".format(this_cmd))
        stdout, stderr = await childprocess.communicate()
        if stderr:
            msg = 'Command {} failed with {}'.format(cmd, stderr)
            logging.error(msg)
            t = '%s' % datetime.now()
            t = t[:-3] + " " + str(msg)
            log_file_handle.write(t + '\n')
            log_file_handle.flush()
        if stdout:
            stdout = stdout.decode("utf-8")
            log_file_handle.write(stdout)
        msg = "********************** Periodic Command Ends **********************"
        logging.info(msg)
        t = '%s' % datetime.now()
        t = t[:-3] + " " + str(msg)
        log_file_handle.write(t + '\n')
        log_file_handle.flush()

# Multiplexed sessions need a control master to connect to. The run time parameters -M and -S also correspond
# to ControlMaster and ControlPath, respectively. So first an initial master connection is established using
# -M when accompanied by the path to the control socket using -S.
#
# ssh -M -S /home/fred/.ssh/controlmasters/fred@server.example.org:22 server.example.org
# Then subsequent multiplexed connections are made in other terminals. They use ControlPath or -S to point to the control socket.
# ssh -O check -S ~/.ssh/controlmasters/%r@%h:%p server.example.org
# ssh -S /home/fred/.ssh/controlmasters/fred@server.example.org:22 server.example.org
class ssh_session:
    sessionid = 1;
    class SSHReaderProtocol(asyncio.SubprocessProtocol):
        def __init__(self, session, silent_mode):
            self._exited = False
            self._closed_stdout = False
            self._closed_stderr = False
            self._mypid = None
            self._stdoutbuffer = ""
            self._stderrbuffer = ""
            self.debug = False
            self._session = session
            self._silent_mode = silent_mode
            if self._session.CONNECT_TIMEOUT is not None :
                self.watchdog = ssh_node.loop.call_later(self._session.CONNECT_TIMEOUT, self.wd_timer)
            self._session.closed.clear()
            self.timeout_occurred = asyncio.Event()
            self.timeout_occurred.clear()

        @property
        def finished(self):
            return self._exited and self._closed_stdout and self._closed_stderr

        def signal_exit(self):
            if not self.finished:
                return
            self._session.closed.set()

        def connection_made(self, transport):
            if self._session.CONNECT_TIMEOUT is not None :
                self.watchdog.cancel()
            self._mypid = transport.get_pid()
            self._transport = transport
            self._session.sshpipe = self._transport.get_extra_info('subprocess')
            self._session.adapter.debug('{} ssh node connection made pid=({})'.format(self._session.name, self._mypid))
            self._session.connected.set()
            if self._session.IO_TIMEOUT is not None :
                self.iowatchdog = ssh_node.loop.call_later(self._session.IO_TIMEOUT, self.io_timer)
            if self._session.CMD_TIMEOUT is not None :
                self.watchdog = ssh_node.loop.call_later(self._session.CMD_TIMEOUT, self.wd_timer)

        def connection_lost(self, exc):
            self._session.adapter.debug('{} node connection lost pid=({})'.format(self._session.name, self._mypid))
            self._session.connected.clear()

        def pipe_data_received(self, fd, data):
            if self._session.IO_TIMEOUT is not None :
                self.iowatchdog.cancel()
            if self.debug :
                logging.debug('{} {}'.format(fd, data))
            self._session.results.extend(data)
            data = data.decode("utf-8")
            if fd == 1:
                self._stdoutbuffer += data
                while "\n" in self._stdoutbuffer:
                    line, self._stdoutbuffer = self._stdoutbuffer.split("\n", 1)
                    if not self._silent_mode :
                        self._session.adapter.info('{}'.format(line.replace("\r","")))

            elif fd == 2:
                self._stderrbuffer += data
                while "\n" in self._stderrbuffer:
                    line, self._stderrbuffer = self._stderrbuffer.split("\n", 1)
                    self._session.adapter.warning('{} {}'.format(self._session.name, line.replace("\r","")))

            if self._session.IO_TIMEOUT is not None :
                self.iowatchdog = ssh_node.loop.call_later(self._session.IO_TIMEOUT, self.io_timer)

        def pipe_connection_lost(self, fd, exc):
            if self._session.IO_TIMEOUT is not None :
                self.iowatchdog.cancel()
            if fd == 1:
                self._session.adapter.debug('{} stdout pipe closed (exception={})'.format(self._session.name, exc))
                self._closed_stdout = True
            elif fd == 2:
                self._session.adapter.debug('{} stderr pipe closed (exception={})'.format(self._session.name, exc))
                self._closed_stderr = True
            self.signal_exit()

        def process_exited(self):
            if self._session.CMD_TIMEOUT is not None :
                self.watchdog.cancel()
            logging.debug('{} subprocess with pid={} closed'.format(self._session.name, self._mypid))
            self._exited = True
            self._mypid = None
            self.signal_exit()

        def wd_timer(self, type=None):
            logging.error("{}: timeout: pid={}".format(self._session.name, self._mypid))
            self.timeout_occurred.set()
            if self._session.sshpipe :
                self._session.sshpipe.terminate()

        def io_timer(self, type=None):
            logging.error("{} IO timeout: cmd='{}' host(pid)={}({})".format(self._session.name, self._session.cmd, self._session.hostname, self._mypid))
            self.timeout_occurred.set()
            self._session.sshpipe.terminate()

    class CustomAdapter(logging.LoggerAdapter):
        def process(self, msg, kwargs):
            return '[%s] %s' % (self.extra['connid'], msg), kwargs

    def __init__(self, user='root', name=None, hostname='localhost', CONNECT_TIMEOUT=None, control_master=False, node=None, silent_mode=False, ssh_speedups=True):
        self.hostname = hostname
        self.name = name
        self.user = user
        self.opened = asyncio.Event()
        self.closed = asyncio.Event()
        self.connected = asyncio.Event()
        self.closed.set()
        self.opened.clear()
        self.connected.clear()
        self.results = bytearray()
        self.sshpipe = None
        self.node = node
        self.CONNECT_TIMEOUT = CONNECT_TIMEOUT
        self.IO_TIMEOUT = None
        self.CMD_TIMEOUT = None
        self.control_master = control_master
        self.ssh = node.ssh.copy()
        self.silent_mode = silent_mode
        self.ssh_speedups = ssh_speedups
        logger = logging.getLogger(__name__)
        if control_master :
            conn_id = self.name + '(console)'
        else  :
            conn_id = '{}({})'.format(self.name, ssh_session.sessionid)
            ssh_session.sessionid += 1

        self.adapter = self.CustomAdapter(logger, {'connid': conn_id})

    def __getattr__(self, attr) :
        if self.node :
            return getattr(self.node, attr)

    @property
    def is_established(self):
        return self._exited and self._closed_stdout and self._closed_stderr

    async def close(self) :
        if self.control_master :
            logging.info('control master close called {}'.format(self.controlmasters))
            childprocess = await asyncio.create_subprocess_exec('/usr/bin/ssh', 'root@{}'.format(self.ipaddr), 'pkill', 'dmesg', stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout, stderr = await childprocess.communicate()
            if stdout :
                logging.info('{}'.format(stdout))
            if stderr :
                logging.info('{}'.format(stderr))
            self.sshpipe.terminate()
            await self.closed.wait()
            logging.info('control master exit called {}'.format(self.controlmasters))
            childprocess = await asyncio.create_subprocess_exec(self.ssh, '-o ControlPath={}'.format(self.controlmasters), '-O exit dummy-arg-why-needed', stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout, stderr = await childprocess.communicate()
            if stdout :
                logging.info('{}'.format(stdout))
            if stderr :
                logging.info('{}'.format(stderr))

        elif self.sshpipe :
            self.sshpipe.terminate()
            await self.closed.wait()

    async def post_cmd(self, cmd=None, IO_TIMEOUT=None, CMD_TIMEOUT=None, ssh_speedups=True, repeat=None) :
        logging.debug("{} Post command {}".format(self.name, cmd))
        self.opened.clear()
        self.cmd = cmd
        self.IO_TIMEOUT = IO_TIMEOUT
        self.CMD_TIMEOUT = CMD_TIMEOUT
        self.repeatcmd = None
        sshcmd = self.ssh.copy()
        if self.control_master :
            try:
                os.remove(str(self.controlmasters))
            except OSError:
                pass
            sshcmd.extend(['-o ControlMaster=yes', '-o ControlPath={}'.format(self.controlmasters), '-o ControlPersist=1'])
        elif self.node.sshtype == 'ssh' :
            sshcmd.append('-o ControlPath={}'.format(self.controlmasters))
        if self.node.ssh_speedups :
            sshcmd.extend(['{}@{}'.format(self.user, self.hostname), cmd])
        else :
            sshcmd.extend(['{}'.format(self.hostname), cmd])
        s = " "
        logging.info('{} {}'.format(self.name, s.join(sshcmd)))
        while True :
            # self in the ReaderProtocol() is this ssh_session instance
            self._transport, self._protocol = await ssh_node.loop.subprocess_exec(lambda: self.SSHReaderProtocol(self, self.silent_mode), *sshcmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=None)
            # self.sshpipe = self._transport.get_extra_info('subprocess')
            # Establish the remote command
            await self.connected.wait()
            logging.debug("post_cmd connected")
            # u = '{}\n'.format(cmd)
            # self.sshpipe.stdin.write(u.encode())
            # Wait for the command to complete
            if not self.control_master :
                await self.closed.wait()
            if not repeat :
                break
        return self.results
