/* packet-dcerpc-fldb.c
 *
 * Routines for DCE DFS Fileset Location Server Calls
 * Copyright 2004, Jaime Fournier <Jaime.Fournier@hush.com>
 * This information is based off the released idl files from opengroup.
 * ftp://ftp.opengroup.org/pub/dce122/dce/src/file.tar.gz file/flserver/fldb_proc.idl
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"


#include <epan/packet.h>
#include "packet-dcerpc.h"
#include "packet-dcerpc-dce122.h"

void proto_register_fldb (void);
void proto_reg_handoff_fldb (void);

static int proto_fldb = -1;
static int hf_fldb_opnum = -1;

static gint ett_fldb = -1;
static gint ett_fldb_vldbentry = -1;
static gint ett_fldb_afsnetaddr = -1;
static gint ett_fldb_siteflags = -1;
static gint ett_fldb_afsflags = -1;
static gint ett_fldb_vlconf_cell = -1;
static gint ett_fldb_afsNameString_t = -1;


static e_guid_t uuid_fldb =
  { 0x4d37f2dd, 0xed43, 0x0000, {0x02, 0xc0, 0x37, 0xcf, 0x2e, 0x00, 0x00, 0x01}
};
static guint16 ver_fldb = 4;

#if 0
static int hf_fldb_getentrybyname_rqst_var1 = -1;
static int hf_fldb_getentrybyname_rqst_key_size = -1;
#endif
static int hf_fldb_releaselock_rqst_fsid_high = -1;
static int hf_fldb_releaselock_rqst_fsid_low = -1;
static int hf_fldb_releaselock_rqst_voltype = -1;
static int hf_fldb_releaselock_rqst_voloper = -1;
static int hf_fldb_setlock_rqst_fsid_high = -1;
static int hf_fldb_setlock_rqst_fsid_low = -1;
static int hf_fldb_setlock_rqst_voltype = -1;
static int hf_fldb_setlock_rqst_voloper = -1;
#if 0
static int hf_fldb_setlock_resp_st = -1;
static int hf_fldb_setlock_resp_st2 = -1;
#endif
static int hf_fldb_listentry_rqst_previous_index = -1;
static int hf_fldb_listentry_rqst_var1 = -1;
static int hf_fldb_listentry_resp_count = -1;
static int hf_fldb_listentry_resp_next_index = -1;
#if 0
static int hf_fldb_listentry_resp_key_t = -1;
static int hf_fldb_listentry_resp_key_size = -1;
static int hf_fldb_listentry_resp_key_t2 = -1;
static int hf_fldb_listentry_resp_key_size2 = -1;
static int hf_fldb_listentry_resp_voltype = -1;
static int hf_fldb_createentry_rqst_key_t = -1;
static int hf_fldb_createentry_rqst_key_size = -1;
#endif
static int hf_fldb_deleteentry_rqst_fsid_high = -1;
static int hf_fldb_deleteentry_rqst_fsid_low = -1;
static int hf_fldb_deleteentry_rqst_voltype = -1;
static int hf_fldb_deleteentry_rqst_voloper = -1;
#if 0
static int hf_fldb_getentrybyid_rqst_fsid_high = -1;
static int hf_fldb_getentrybyid_rqst_fsid_low = -1;
static int hf_fldb_getentrybyid_rqst_voltype = -1;
static int hf_fldb_getentrybyid_rqst_voloper = -1;
#endif
static int hf_fldb_replaceentry_rqst_fsid_high = -1;
static int hf_fldb_replaceentry_rqst_fsid_low = -1;
static int hf_fldb_replaceentry_rqst_voltype = -1;
#if 0
static int hf_fldb_replaceentry_rqst_key_size = -1;
static int hf_fldb_replaceentry_rqst_key_t = -1;
static int hf_fldb_replaceentry_resp_st = -1;
static int hf_fldb_replaceentry_resp_st2 = -1;
#endif
#if 0
static int hf_fldb_getentrybyname_resp_volumetype = -1;
static int hf_fldb_getentrybyname_resp_numservers = -1;
static int hf_fldb_getentrybyname_resp_sitecookies = -1;
static int hf_fldb_getentrybyname_resp_sitepartition = -1;
static int hf_fldb_getentrybyname_resp_siteflags = -1;
static int hf_fldb_getentrybyname_resp_sitemaxreplat = -1;
static int hf_fldb_getentrybyname_resp_volid_high = -1;
static int hf_fldb_getentrybyname_resp_volid_low = -1;
static int hf_fldb_getentrybyname_resp_voltype = -1;
static int hf_fldb_getentrybyname_resp_cloneid_high = -1;
static int hf_fldb_getentrybyname_resp_cloneid_low = -1;
static int hf_fldb_getentrybyname_resp_flags = -1;
static int hf_fldb_getentrybyname_resp_maxtotallat = -1;
static int hf_fldb_getentrybyname_resp_hardmaxtotlat = -1;
static int hf_fldb_getentrybyname_resp_minpouncedally = -1;
static int hf_fldb_getentrybyname_resp_defaultmaxreplat = -1;
static int hf_fldb_getentrybyname_resp_reclaimdally = -1;
static int hf_fldb_getentrybyname_resp_whenlocked = -1;
static int hf_fldb_getentrybyname_resp_spare1 = -1;
static int hf_fldb_getentrybyname_resp_spare2 = -1;
static int hf_fldb_getentrybyname_resp_spare3 = -1;
static int hf_fldb_getentrybyname_resp_spare4 = -1;
static int hf_fldb_getentrybyname_resp_key_t = -1;
static int hf_fldb_getentrybyname_resp_key_size = -1;
static int hf_fldb_getentrybyname_resp_test = -1;
static int hf_dcerpc_error_status = -1;
#endif
static int hf_fldb_vldbentry_volumename = -1;
static int hf_fldb_vldbentry_volumetype = -1;
static int hf_fldb_vldbentry_nservers = -1;
static int hf_fldb_vldbentry_sitepartition = -1;
static int hf_fldb_afsnetaddr_type = -1;
static int hf_fldb_afsnetaddr_data = -1;
static int hf_fldb_siteflags = -1;
static int hf_fldb_vldbentry_sitemaxreplicalatency = -1;
static int hf_fldb_vldbentry_siteprincipal = -1;
static int hf_fldb_vldbentry_siteowner = -1;
static int hf_fldb_vldbentry_siteobjid = -1;
static int hf_fldb_vldbentry_volids_high = -1;
static int hf_fldb_vldbentry_volids_low = -1;
static int hf_fldb_vldbentry_voltypes = -1;
static int hf_fldb_vldbentry_cloneid_high = -1;
static int hf_fldb_vldbentry_cloneid_low = -1;
static int hf_fldb_afsflags_flags = -1;
static int hf_fldb_vldbentry_maxtotallatency = -1;
static int hf_fldb_vldbentry_hardmaxtotallatency = -1;
static int hf_fldb_vldbentry_minimumpouncedally = -1;
static int hf_fldb_vldbentry_defaultmaxreplicalatency = -1;
static int hf_fldb_vldbentry_reclaimdally = -1;
static int hf_fldb_vldbentry_whenlocked = -1;
static int hf_fldb_vldbentry_spare1 = -1;
static int hf_fldb_vldbentry_spare2 = -1;
static int hf_fldb_vldbentry_spare3 = -1;
static int hf_fldb_vldbentry_spare4 = -1;
static int hf_fldb_vldbentry_lockername = -1;
static int hf_fldb_vldbentry_charspares = -1;
static int hf_fldb_vlconf_cell_name = -1;
static int hf_fldb_vlconf_cell_cellid_high = -1;
static int hf_fldb_vlconf_cell_cellid_low = -1;
static int hf_fldb_vlconf_cell_numservers = -1;
static int hf_fldb_vlconf_cell_hostname = -1;
static int hf_fldb_vlconf_cell_spare1 = -1;
static int hf_fldb_vlconf_cell_spare2 = -1;
static int hf_fldb_vlconf_cell_spare3 = -1;
static int hf_fldb_vlconf_cell_spare4 = -1;
static int hf_fldb_vlconf_cell_spare5 = -1;
static int hf_fldb_flagsp = -1;
static int hf_fldb_nextstartp = -1;
static int hf_fldb_afsNameString_t_principalName_string = -1;
static int hf_fldb_afsNameString_t_principalName_size = -1;
/* static int hf_fldb_afsNameString_t_principalName_size2 = -1; */
static int hf_fldb_namestring = -1;
static int hf_error_st = -1;
static int hf_fldb_creationquota = -1;
static int hf_fldb_creationuses = -1;
static int hf_fldb_deletedflag = -1;
/* static int hf_fldb_namestring_size = -1; */
static int hf_fldb_numwanted = -1;
static int hf_fldb_spare2 = -1;
static int hf_fldb_spare3 = -1;
static int hf_fldb_spare4 = -1;
static int hf_fldb_spare5 = -1;
static int hf_fldb_uuid_objid = -1;
static int hf_fldb_uuid_owner = -1;
static int hf_fldb_volid_high = -1;
static int hf_fldb_volid_low = -1;
static int hf_fldb_voltype = -1;
static guint32 st;
static const guint8 *st_str;

#define AFS_FLAG_RETURNTOKEN         0x00001
#define AFS_FLAG_TOKENJUMPQUEUE      0x00002
#define AFS_FLAG_SKIPTOKEN           0x00004
#define AFS_FLAG_NOOPTIMISM          0x00008
#define AFS_FLAG_TOKENID             0x00010
#define AFS_FLAG_RETURNBLOCKER       0x00020
#define AFS_FLAG_ASYNCGRANT          0x00040
#define AFS_FLAG_NOREVOKE            0x00080
#define AFS_FLAG_MOVE_REESTABLISH    0x00100
#define AFS_FLAG_SERVER_REESTABLISH  0x00200
#define AFS_FLAG_NO_NEW_EPOCH        0x00400
#define AFS_FLAG_MOVE_SOURCE_OK      0x00800
#define AFS_FLAG_SYNC                0x01000
#define AFS_FLAG_ZERO                0x02000
#define AFS_FLAG_SKIPSTATUS          0x04000
#define AFS_FLAG_FORCEREVOCATIONS    0x08000
#define AFS_FLAG_FORCEVOLQUIESCE     0x10000
#define AFS_FLAG_FORCEREVOCATIONDOWN 0x20000

#define AFS_FLAG_SEC_SERVICE           0x01
#define AFS_FLAG_CONTEXT_NEW_IF        0x02
#define AFS_FLAG_CONTEXT_DO_RESET      0x04
#define AFS_FLAG_CONTEXT_NEW_ACL_IF    0x08
#define AFS_FLAG_CONTEXT_NEW_TKN_TYPES 0x10

#define VLSF_NEWREPSITE   0x01
#define VLSF_SPARE1       0x02
#define VLSF_SPARE2       0x04    /* used for VLSF_RWVOL in flprocs.c */
#define VLSF_SPARE3       0x08    /* used for VLSF_BACKVOL in flprocs.c */
#define VLSF_SAMEASPREV   0x10
#define VLSF_DEFINED      0x20
#define VLSF_PARTIALADDRS 0x40
#define VLSF_ZEROIXHERE         0x80000000

#define MACRO_ST_CLEAR(name) \
  offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_error_st, &st); \
  st_str = val_to_str_ext (st, &dce_error_vals_ext, "%u");              \
  if (st) {                                                              \
      col_add_fstr (pinfo->cinfo, COL_INFO, "%s st:%s ", name, st_str); \
  } else {                                                                \
      col_append_fstr (pinfo->cinfo, COL_INFO, " st:%s ", st_str);      \
  }


static int
dissect_afsnetaddr (tvbuff_t *tvb, int offset,
                    packet_info *pinfo, proto_tree *parent_tree,
                    dcerpc_info *di, guint8 *drep)
{
  proto_item *item       = NULL;
  proto_tree *tree       = NULL;
  int         old_offset = offset;
  guint16     type;
  guint8      data;
  int         i;

  if (parent_tree)
    {
      tree = proto_tree_add_subtree (parent_tree, tvb, offset, -1,
                                     ett_fldb_afsnetaddr, &item, "afsNetAddr:");
    }

/*                 unsigned16 type;
                   unsigned8 data[14];
*/

  offset =
    dissect_ndr_uint16 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_afsnetaddr_type, &type);

  if (type)
    {
      col_append_fstr (pinfo->cinfo, COL_INFO, " Type:%u ", type);

      for (i = 0; i < 14; i++)
        {
          offset =
            dissect_ndr_uint8 (tvb, offset, pinfo, tree, di, drep,
                               hf_fldb_afsnetaddr_data, &data);

          switch (i)
            {
            case 1:
              if (data)
                {
                  col_append_fstr (pinfo->cinfo, COL_INFO, " Port:%u", data);
                }
              break;
            case 2:
                col_append_fstr (pinfo->cinfo, COL_INFO, " IP:%u.", data);
              break;
            case 3:
                col_append_fstr (pinfo->cinfo, COL_INFO, "%u.", data);
              break;
            case 4:
                col_append_fstr (pinfo->cinfo, COL_INFO, "%u.", data);
              break;
            case 5:
                col_append_fstr (pinfo->cinfo, COL_INFO, "%u", data);
              break;
            }

        }
    }
  else
    {
      offset += 14;             /* space left after reading in type for the array. */
    }

  proto_item_set_len (item, offset - old_offset);

  return offset;
}


static int
dissect_vlconf_cell (tvbuff_t *tvb, int offset,
                     packet_info *pinfo, proto_tree *parent_tree,
                     dcerpc_info *di, guint8 *drep)
{
  proto_item   *item       = NULL;
  proto_tree   *tree       = NULL;
  int           old_offset = offset;
#define MAXVLCELLCHARS    128
#define MAXVLHOSTSPERCELL 64
  const guint8 *name, *hostname;
  int           i;
  guint32       cellid_high, cellid_low, numservers, spare1, spare2, spare3, spare4,
    spare5;

  if (parent_tree)
    {
      tree = proto_tree_add_subtree (parent_tree, tvb, offset, -1,
                                     ett_fldb_vlconf_cell, &item, "vlconf_cell:");
    }

  /* byte name[MAXVLCELLCHARS];          Cell name */
  proto_tree_add_item (tree, hf_fldb_vlconf_cell_name, tvb, offset, 114, ENC_ASCII|ENC_NA);
  name = tvb_get_string_enc(wmem_packet_scope(), tvb, offset, MAXVLCELLCHARS, ENC_ASCII); /* XXX why 114 above and 128 here?? */
  offset += MAXVLCELLCHARS;     /* some reason this 114 seems to be incorrect... cutting 4 short to compensate.... */
  col_append_fstr (pinfo->cinfo, COL_INFO, " Name: %s", name);

  /* afsHyper CellID;                     identifier for that cell  */

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vlconf_cell_cellid_high, &cellid_high);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vlconf_cell_cellid_low, &cellid_low);
  col_append_fstr (pinfo->cinfo, COL_INFO, " CellID:%u-%u", cellid_high,
                     cellid_low);

  /* unsigned32 numServers;              *Num active servers for the cell */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vlconf_cell_numservers, &numservers);
  col_append_fstr (pinfo->cinfo, COL_INFO, " numServers:%u", numservers);

  /*    afsNetAddr hostAddr[MAXVLHOSTSPERCELL]; *addresses for cell's servers */
  for (i = 0; i < MAXVLHOSTSPERCELL; i++)
    {
      offset = dissect_afsnetaddr (tvb, offset, pinfo, tree, di, drep);
    }

  /* hostnam hostName[MAXVLHOSTSPERCELL];        *Names for cell's servers* */

  for (i = 0; i < MAXVLHOSTSPERCELL; i++)
    {
      proto_tree_add_item_ret_string(tree, hf_fldb_vlconf_cell_hostname, tvb, offset,
                             64, ENC_ASCII|ENC_NA, wmem_packet_scope(), &hostname);
      offset += 64;             /* some reason this 114 seems to be incorrect... cutting 4 short to compensate.... */
      col_append_fstr (pinfo->cinfo, COL_INFO, " hostName: %s", hostname);
    }

  /*     unsigned32 spare1; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vlconf_cell_spare1, &spare1);
  col_append_fstr (pinfo->cinfo, COL_INFO, " spare1:%u", spare1);

  /*     unsigned32 spare2; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vlconf_cell_spare2, &spare2);
  col_append_fstr (pinfo->cinfo, COL_INFO, " spare2:%u", spare2);

  /*     unsigned32 spare3; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vlconf_cell_spare3, &spare3);
  col_append_fstr (pinfo->cinfo, COL_INFO, " spare3:%u", spare3);

  /*     unsigned32 spare4; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vlconf_cell_spare4, &spare4);
  col_append_fstr (pinfo->cinfo, COL_INFO, " spare4:%u", spare4);

  /*     unsigned32 spare5; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vlconf_cell_spare5, &spare5);
  col_append_fstr (pinfo->cinfo, COL_INFO, " spare5:%u", spare5);

  proto_item_set_len (item, offset - old_offset);

  return offset;
}


static int
dissect_afsNameString_t (tvbuff_t *tvb, int offset,
                         packet_info *pinfo, proto_tree *parent_tree,
                         dcerpc_info *di, guint8 *drep)
{
/*
typedef [string] byte   NameString_t[AFS_NAMEMAX];
*/

  proto_item   *item       = NULL;
  proto_tree   *tree       = NULL;
  int           old_offset = offset;
#define AFS_NAMEMAX 256
  guint32       string_size;
  const guint8 *namestring;

  if (di->conformant_run)
    {
      return offset;
    }

  if (parent_tree)
    {
      tree = proto_tree_add_subtree (parent_tree, tvb, offset, -1,
                                     ett_fldb_afsNameString_t, &item, "afsNameString_t:");    }

  offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                               hf_fldb_afsNameString_t_principalName_size,
                               &string_size);
  col_append_fstr (pinfo->cinfo, COL_INFO, " String_size:%u", string_size);
  if (string_size < AFS_NAMEMAX)
    {
/* proto_tree_add_string(tree, id, tvb, start, length, value_ptr); */
      proto_tree_add_item_ret_string(tree, hf_fldb_afsNameString_t_principalName_string,
                             tvb, offset, string_size, ENC_ASCII|ENC_NA, wmem_packet_scope(), &namestring);
      offset += string_size;
        col_append_fstr (pinfo->cinfo, COL_INFO, " Principal:%s", namestring);
    }
  else
    {
        col_append_fstr (pinfo->cinfo, COL_INFO,
                         " :FIXME!: Invalid string length of  %u",
                         string_size);
    }
  proto_item_set_len (item, offset - old_offset);
  return offset;
}


static int
dissect_afsflags (tvbuff_t *tvb, int offset,
                  packet_info *pinfo, proto_tree *parent_tree,
                  dcerpc_info *di, guint8 *drep)
{
  proto_item *item       = NULL;
  proto_tree *tree       = NULL;
  int         old_offset = offset;
  guint32     afsflags;

  if (parent_tree)
    {
      tree = proto_tree_add_subtree (parent_tree, tvb, offset, -1, ett_fldb_afsflags, &item, "afsFlags:");
    }

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_afsflags_flags, &afsflags);
  if (afsflags)
    {
      col_append_str (pinfo->cinfo, COL_INFO, " afsFlags=");
      if ((afsflags & AFS_FLAG_RETURNTOKEN) == AFS_FLAG_RETURNTOKEN)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":RETURNTOKEN");
        }
      if ((afsflags & AFS_FLAG_TOKENJUMPQUEUE) == AFS_FLAG_TOKENJUMPQUEUE)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":TOKENJUMPQUEUE");
        }
      if ((afsflags & AFS_FLAG_SKIPTOKEN) == AFS_FLAG_SKIPTOKEN)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":SKIPTOKEN");
        }
      if ((afsflags & AFS_FLAG_NOOPTIMISM) == AFS_FLAG_NOOPTIMISM)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":NOOPTIMISM");
        }
      if ((afsflags & AFS_FLAG_TOKENID) == AFS_FLAG_TOKENID)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":TOKENID");
        }
      if ((afsflags & AFS_FLAG_RETURNBLOCKER) == AFS_FLAG_RETURNBLOCKER)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":RETURNBLOCKER");
        }
      if ((afsflags & AFS_FLAG_ASYNCGRANT) == AFS_FLAG_ASYNCGRANT)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":ASYNCGRANT");
        }
      if ((afsflags & AFS_FLAG_NOREVOKE) == AFS_FLAG_NOREVOKE)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":NOREVOKE");
        }
      if ((afsflags & AFS_FLAG_MOVE_REESTABLISH) == AFS_FLAG_MOVE_REESTABLISH)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":MOVE_REESTABLISH");
        }
      if ((afsflags & AFS_FLAG_SERVER_REESTABLISH) ==
          AFS_FLAG_SERVER_REESTABLISH)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":SERVER_REESTABLISH");
          if ((afsflags & AFS_FLAG_NO_NEW_EPOCH) == AFS_FLAG_NO_NEW_EPOCH)
            {
              col_append_str (pinfo->cinfo, COL_INFO, ":NO_NEW_EPOCH");
            }
          if ((afsflags & AFS_FLAG_MOVE_SOURCE_OK) == AFS_FLAG_MOVE_SOURCE_OK)
            {
              col_append_str (pinfo->cinfo, COL_INFO, ":MOVE_SOURCE_OK");
            }
          if ((afsflags & AFS_FLAG_SYNC) == AFS_FLAG_SYNC)
            {
              col_append_str (pinfo->cinfo, COL_INFO, ":SYNC");
            }
          if ((afsflags & AFS_FLAG_ZERO) == AFS_FLAG_ZERO)
            {
              col_append_str (pinfo->cinfo, COL_INFO, ":ZERO");
            }
          if ((afsflags & AFS_FLAG_SKIPSTATUS) == AFS_FLAG_SKIPSTATUS)
            {
              col_append_str (pinfo->cinfo, COL_INFO, ":SKIPSTATUS");
            }
          if ((afsflags & AFS_FLAG_FORCEREVOCATIONS) ==
              AFS_FLAG_FORCEREVOCATIONS)
            {
              col_append_str (pinfo->cinfo, COL_INFO, ":FORCEREVOCATIONS");
            }
          if ((afsflags & AFS_FLAG_FORCEVOLQUIESCE) ==
              AFS_FLAG_FORCEVOLQUIESCE)
            {
              col_append_str (pinfo->cinfo, COL_INFO, ":FORCEVOLQUIESCE");
            }
          if ((afsflags & AFS_FLAG_SEC_SERVICE) == AFS_FLAG_SEC_SERVICE)
            {
              col_append_str (pinfo->cinfo, COL_INFO, ":SEC_SERVICE");
            }
          if ((afsflags & AFS_FLAG_CONTEXT_NEW_ACL_IF) ==
              AFS_FLAG_CONTEXT_NEW_ACL_IF)
            {
              col_append_str (pinfo->cinfo, COL_INFO,
                                ":CONTEXT_NEW_ACL_IF");
            }

        }
    }

  proto_item_set_len (item, offset - old_offset);

  return offset;
}


static int
dissect_siteflags (tvbuff_t *tvb, int offset,
                   packet_info *pinfo, proto_tree *parent_tree,
                   dcerpc_info *di, guint8 *drep)
{
  proto_item *item       = NULL;
  proto_tree *tree       = NULL;
  int         old_offset = offset;
  guint32     siteflags;

  if (parent_tree)
    {
      tree = proto_tree_add_subtree (parent_tree, tvb, offset, -1, ett_fldb_siteflags, &item, "SiteFlags:");
    }

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_siteflags,
                        &siteflags);

  if (siteflags)
    {
      col_append_str (pinfo->cinfo, COL_INFO, " SiteFlags");
      if ((siteflags & VLSF_NEWREPSITE) == VLSF_NEWREPSITE)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":NEWREPSITE");
        }
      if ((siteflags & VLSF_SPARE1) == VLSF_SPARE1)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":SPARE1");
        }
      if ((siteflags & VLSF_SPARE2) == VLSF_SPARE2)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":SPARE2");
        }
      if ((siteflags & VLSF_SPARE3) == VLSF_SPARE3)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":SPARE3");
        }
      if ((siteflags & VLSF_SAMEASPREV) == VLSF_SAMEASPREV)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":SAMEASPREV");
        }
      if ((siteflags & VLSF_DEFINED) == VLSF_DEFINED)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":DEFINED");
        }
      if ((siteflags & VLSF_PARTIALADDRS) == VLSF_PARTIALADDRS)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":PARTIALADDRS ");
        }
      if ((siteflags & VLSF_ZEROIXHERE) == VLSF_ZEROIXHERE)
        {
          col_append_str (pinfo->cinfo, COL_INFO, ":ZEROIXHERE");

        }
    }

  proto_item_set_len (item, offset - old_offset);

  return offset;
}


static int
dissect_vldbentry (tvbuff_t *tvb, int offset,
                   packet_info *pinfo, proto_tree *parent_tree,
                   dcerpc_info *di, guint8 *drep)
{
  proto_item   *item;
  proto_tree   *tree;
  int           old_offset = offset;
  const guint8 *volumename, *siteprincipal, *charspares, *lockername;
  guint32       volumetype, nservers, sitepartition, sitemaxreplicalatency;
  guint32       volids_high, volids_low, voltypes, cloneid_high, cloneid_low;
  guint32       maxtotallatency, hardmaxtotallatency, minimumpouncedally;
  guint32       defaultmaxreplicalatency, reclaimdally, whenlocked;
  guint32       spare1, spare2, spare3, spare4;
  e_guid_t      siteowner, siteobjid;
  gint          i;
#define MAXNSERVERS 16
#define MAXVOLTYPES 8
#define MAXLOCKNAMELEN 64

  tree = proto_tree_add_subtree (parent_tree, tvb, offset, -1, ett_fldb_vldbentry, &item, "vldbentry:");

/*    byte            name[114];      Volume name  */

  proto_tree_add_item_ret_string(tree, hf_fldb_vldbentry_volumename, tvb, offset, 114,
                       ENC_ASCII|ENC_NA, wmem_packet_scope(), &volumename);
  offset += 110;                /* some reason this 114 seems to be incorrect... cutting 4 short to compensate.... */
  col_append_fstr (pinfo->cinfo, COL_INFO, " Name: %s", volumename);

  /* unsigned32      volumeType; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_volumetype, &volumetype);
    col_append_fstr (pinfo->cinfo, COL_INFO, " Type:%u", volumetype);

  /*unsigned32      nServers; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_nservers, &nservers);
    col_append_fstr (pinfo->cinfo, COL_INFO, " nServers:%u", nservers);

  /* afsNetAddr      siteAddr[MAXNSERVERS]; 16 */
  for (i = 0; i < MAXNSERVERS; i++)
    {
      /* col_append_fstr (pinfo->cinfo, COL_INFO, " Site:%u", i); */

      offset = dissect_afsnetaddr (tvb, offset, pinfo, tree, di, drep);
    }

/*                unsigned32      sitePartition[MAXNSERVERS]; */
  for (i = 0; i < MAXNSERVERS; i++)
    {
      offset =
        dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                            hf_fldb_vldbentry_sitepartition, &sitepartition);
      if (sitepartition)
        {
            col_append_fstr (pinfo->cinfo, COL_INFO, " LFS:%u",
                             sitepartition);
        }
    }

  /* unsigned32      siteFlags[MAXNSERVERS]; */
  for (i = 0; i < MAXNSERVERS; i++)
    {
      offset = dissect_siteflags (tvb, offset, pinfo, tree, di, drep);
    }

  /*  unsigned32      sitemaxReplicaLatency[MAXNSERVERS]; */
  for (i = 0; i < MAXNSERVERS; i++)
    {
      offset =
        dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                            hf_fldb_vldbentry_sitemaxreplicalatency,
                            &sitemaxreplicalatency);
      if (sitemaxreplicalatency)
        {
            col_append_fstr (pinfo->cinfo, COL_INFO, " MaxRepLat%d:%u", i,
                             sitemaxreplicalatency);
        }
    }
  /* kerb_princ_name sitePrincipal[MAXNSERVERS];      principal  */
  for (i = 0; i < MAXNSERVERS; i++)
    {
      proto_tree_add_item_ret_string(tree, hf_fldb_vldbentry_siteprincipal, tvb,
                           offset, 64, ENC_ASCII|ENC_NA, wmem_packet_scope(), &siteprincipal);
      offset += 64;
        col_append_fstr (pinfo->cinfo, COL_INFO, " Princ: %s", siteprincipal);
    }

  /* afsUUID         siteOwner[MAXNSERVERS]; */

  for (i = 0; i < MAXNSERVERS; i++)
    {
      offset =
        dissect_ndr_uuid_t (tvb, offset, pinfo, tree, di, drep,
                            hf_fldb_vldbentry_siteowner, &siteowner);
        col_append_fstr (pinfo->cinfo, COL_INFO,
                         " SiteOwner - %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                         siteowner.data1, siteowner.data2, siteowner.data3,
                         siteowner.data4[0], siteowner.data4[1],
                         siteowner.data4[2], siteowner.data4[3],
                         siteowner.data4[4], siteowner.data4[5],
                         siteowner.data4[6], siteowner.data4[7]);
    }

/*                afsUUID         siteObjID[MAXNSERVERS]; */
  for (i = 0; i < MAXNSERVERS; i++)
    {
      offset =
        dissect_ndr_uuid_t (tvb, offset, pinfo, tree, di, drep,
                            hf_fldb_vldbentry_siteobjid, &siteobjid);
        col_append_fstr (pinfo->cinfo, COL_INFO,
                         " SiteObjID - %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                         siteobjid.data1, siteobjid.data2, siteobjid.data3,
                         siteobjid.data4[0], siteobjid.data4[1],
                         siteobjid.data4[2], siteobjid.data4[3],
                         siteobjid.data4[4], siteobjid.data4[5],
                         siteobjid.data4[6], siteobjid.data4[7]);
    }

  /* afsHyper        VolIDs[MAXVOLTYPES]; */
  /* XXX for these hypers, I will skip trying to use non portable guint64, and just read both, and use only low.
     never seen a case of a volid going anywhere the overflow of the 32 low; */
  for (i = 0; i < MAXVOLTYPES; i++)
    {
      offset =
        dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                            hf_fldb_vldbentry_volids_high, &volids_high);
      offset =
        dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                            hf_fldb_vldbentry_volids_low, &volids_low);
        col_append_fstr (pinfo->cinfo, COL_INFO, " VolIDs%d:%u", i,
                         volids_low);
    }

  /* unsigned32      VolTypes[MAXVOLTYPES]; */
  for (i = 0; i < MAXVOLTYPES; i++)
    {
      offset =
        dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                            hf_fldb_vldbentry_voltypes, &voltypes);
      if (voltypes)
        {
            col_append_fstr (pinfo->cinfo, COL_INFO, " VolTypes:%d:%u", i,
                             voltypes);
        }
    }

  /* afsHyper        cloneId;         Used during cloning  */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_cloneid_high, &cloneid_high);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_cloneid_low, &cloneid_low);
  if (cloneid_low)
    {
        col_append_fstr (pinfo->cinfo, COL_INFO, " CloneId:%u", cloneid_low);
    }

  /*  unsigned32      flags;           General flags  */
  offset = dissect_afsflags (tvb, offset, pinfo, tree, di, drep);

  /* unsigned32      maxTotalLatency; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_maxtotallatency, &maxtotallatency);
    col_append_fstr (pinfo->cinfo, COL_INFO, " MaxTotLat:%u",
                     maxtotallatency);

  /* unsigned32      hardMaxTotalLatency; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_hardmaxtotallatency,
                        &hardmaxtotallatency);
    col_append_fstr (pinfo->cinfo, COL_INFO, " HardMaxTotLat:%u",
                     hardmaxtotallatency);

  /* unsigned32      minimumPounceDally; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_minimumpouncedally,
                        &minimumpouncedally);
    col_append_fstr (pinfo->cinfo, COL_INFO, " minPounceDally:%u",
                     minimumpouncedally);

  /* unsigned32      defaultMaxReplicaLatency; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_defaultmaxreplicalatency,
                        &defaultmaxreplicalatency);
    col_append_fstr (pinfo->cinfo, COL_INFO, " defaultMaxReplicaLatency:%u",
                     defaultmaxreplicalatency);

  /* unsigned32      reclaimDally; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_reclaimdally, &reclaimdally);
    col_append_fstr (pinfo->cinfo, COL_INFO, " reclaimDally:%u",
                     reclaimdally);

  /*   unsigned32      WhenLocked; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_whenlocked, &whenlocked);
    col_append_fstr (pinfo->cinfo, COL_INFO, " WhenLocked:%u", whenlocked);

  /*                unsigned32      spare1; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_spare1, &spare1);
    col_append_fstr (pinfo->cinfo, COL_INFO, " spare1:%u", spare1);

  /*                unsigned32      spare2; */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_spare2, &spare2);
    col_append_fstr (pinfo->cinfo, COL_INFO, " spare2:%u", spare2);

  /*                unsigned32      spare3; */
  offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_spare3, &spare3);
    col_append_fstr (pinfo->cinfo, COL_INFO, " spare3:%u", spare3);

  /*                unsigned32      spare4; */
  offset = dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_vldbentry_spare4, &spare4);
  col_append_fstr (pinfo->cinfo, COL_INFO, " spare4:%u", spare4);


  /* byte            LockerName[MAXLOCKNAMELEN]; */
  proto_tree_add_item_ret_string(tree, hf_fldb_vldbentry_lockername, tvb, offset,
                       MAXLOCKNAMELEN, ENC_ASCII|ENC_NA, wmem_packet_scope(), &lockername);
  offset += MAXLOCKNAMELEN;     /* some reason this 114 seems to be incorrect... cutting 4 short to compensate.... */
  col_append_fstr (pinfo->cinfo, COL_INFO, " LockerName: %s", lockername);

  /*     byte            charSpares[50]; */
  proto_tree_add_item_ret_string(tree, hf_fldb_vldbentry_charspares, tvb, offset, 50,
                       ENC_ASCII|ENC_NA, wmem_packet_scope(), &charspares);
  offset += 50;                 /* some reason this 114 seems to be incorrect... cutting 4 short to compensate.... */
  col_append_fstr (pinfo->cinfo, COL_INFO, " charSpares:%s", charspares);

  proto_item_set_len (item, offset - old_offset);

  return offset;
}


static int
fldb_dissect_getcellinfo_resp (tvbuff_t *tvb, int offset,
                               packet_info *pinfo, proto_tree *tree,
                               dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

/* [out] vlconf_cell *MyCell */
  offset = dissect_vlconf_cell (tvb, offset, pinfo, tree, di, drep);

  return offset;
}


static int
fldb_dissect_getentrybyname_rqst (tvbuff_t *tvb, int offset,
                                  packet_info *pinfo, proto_tree *tree,
                                  dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

  /*
   *     [in] volumeName volName,
   */

  offset += 4;
  offset = dissect_afsNameString_t (tvb, offset, pinfo, tree, di, drep);

  return offset;
}


static int
fldb_dissect_getentrybyname_resp (tvbuff_t *tvb, int offset,
                                  packet_info *pinfo, proto_tree *tree,
                                  dcerpc_info *di, guint8 *drep)
{
  /*
     [out] vldbentry *entry
   */
  if (di->conformant_run)
    {
      return offset;
    }

  offset = dissect_vldbentry (tvb, offset, pinfo, tree, di, drep);

  MACRO_ST_CLEAR ("GetEntryByName reply");
  return offset;
}


static int
fldb_dissect_getsiteinfo_rqst (tvbuff_t *tvb, int offset,
                               packet_info *pinfo, proto_tree *tree,
                               dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

  /*
   *   [in] afsNetAddr *OldAddr,
   *
   */

  offset = dissect_afsnetaddr (tvb, offset, pinfo, tree, di, drep);

  /*
   *
   * [in] afsNetAddr *OldAddr,
   *         unsigned16 type;
   unsigned8 data[14];
   */

  return offset;
}


static int
fldb_dissect_getsiteinfo_resp (tvbuff_t *tvb, int offset,
                               packet_info *pinfo, proto_tree *tree,
                               dcerpc_info *di, guint8 *drep)
{
  const guint8 *namestring;
  e_guid_t      owner, objid;
  guint32       creationquota, creationuses, deletedflag;
  guint32       spare2, spare3, spare4, spare5;

  if (di->conformant_run)
    {
      return offset;
    }

  /*
     [out] siteDessiib *FullSiteInfo
     afsNetAddr Addr[ADDRSINSITE];
     byte KerbPrin[MAXKPRINCIPALLEN] 64;
     afsUUID Owner;
     afsUUID ObjID;
     unsigned32 CreationQuota;
     unsigned32 CreationUses;
     unsigned32 DeletedFlag;
     unsigned32 spare2;
     unsigned32 spare3;
     unsigned32 spare4;
     unsigned32 spare5;
   */

  offset = dissect_afsnetaddr (tvb, offset, pinfo, tree, di, drep);

  /* handle byte KerbPrin[64]. */

  offset += 48;                 /* part of kerbprin before name... */

  proto_tree_add_item_ret_string(tree, hf_fldb_namestring, tvb, offset, 64, ENC_ASCII|ENC_NA, wmem_packet_scope(), &namestring);
  offset += 64;
  col_append_fstr (pinfo->cinfo, COL_INFO, " %s", namestring);

  offset = dissect_ndr_uuid_t (tvb, offset, pinfo, tree, di, drep, hf_fldb_uuid_owner, &owner);
  col_append_fstr (pinfo->cinfo, COL_INFO,
                     " Owner - %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     owner.data1, owner.data2, owner.data3, owner.data4[0],
                     owner.data4[1], owner.data4[2], owner.data4[3],
                     owner.data4[4], owner.data4[5], owner.data4[6],
                     owner.data4[7]);

  offset =
    dissect_ndr_uuid_t (tvb, offset, pinfo, tree, di, drep, hf_fldb_uuid_objid,
                        &objid);
  col_append_fstr (pinfo->cinfo, COL_INFO,
                     " ObjID - %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     objid.data1, objid.data2, objid.data3, objid.data4[0],
                     objid.data4[1], objid.data4[2], objid.data4[3],
                     objid.data4[4], objid.data4[5], objid.data4[6],
                     objid.data4[7]);

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_creationquota,
                        &creationquota);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_creationuses,
                        &creationuses);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_deletedflag,
                        &deletedflag);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_spare2,
                        &spare2);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_spare3,
                        &spare3);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_spare4,
                        &spare4);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_spare5,
                        &spare5);

  col_append_fstr (pinfo->cinfo, COL_INFO,
                     " CreationQuota:%u CreationUses:%u DeletedFlag:%u Spare2:%u Spare3:%u Spare4:%u Spare5:%u",
                     creationquota, creationuses, deletedflag, spare2, spare3,
                     spare4, spare5);

  MACRO_ST_CLEAR ("GetSiteInfo reply");

  return offset;
}


static int
fldb_dissect_listentry_rqst (tvbuff_t *tvb, int offset,
                             packet_info *pinfo, proto_tree *tree,
                             dcerpc_info *di, guint8 *drep)
{
  guint32 var1, previous_index;

  if (di->conformant_run)
    {
      return offset;
    }

  /*
   *               [in] unsigned32 previous_index,
   *               [out] unsigned32 *count,
   *               [out] unsigned32 *next_index,
   *               [out] vldbentry *entry
   */

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_listentry_rqst_previous_index,
                        &previous_index);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_listentry_rqst_var1, &var1);


  col_append_fstr (pinfo->cinfo, COL_INFO, " :PrevIndex: %u",
                     previous_index);

  return offset;
}


static int
fldb_dissect_listentry_resp (tvbuff_t *tvb, int offset,
                             packet_info *pinfo, proto_tree *tree,
                             dcerpc_info *di, guint8 *drep)
{
  guint32 count, next_index;

  if (di->conformant_run)
    {
      return offset;
    }

  /*
   *               [out] unsigned32 *count,
   *               [out] unsigned32 *next_index,
   *               [out] vldbentry *entry
   */

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_listentry_resp_count, &count);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_listentry_resp_next_index, &next_index);
  offset = dissect_vldbentry (tvb, offset, pinfo, tree, di, drep);
  return offset;
}


static int
fldb_dissect_setlock_rqst (tvbuff_t *tvb, int offset,
                           packet_info *pinfo, proto_tree *tree,
                           dcerpc_info *di, guint8 *drep)
{
  guint32 fsid_high, fsid_low, voltype, voloper;

  if (di->conformant_run)
    {
      return offset;
    }

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_setlock_rqst_fsid_high, &fsid_high);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_setlock_rqst_fsid_low, &fsid_low);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_setlock_rqst_voltype, &voltype);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_setlock_rqst_voloper, &voloper);

  col_append_fstr (pinfo->cinfo, COL_INFO,
                     " :FSID:%u/%u VolType:0x%x VolOper:%u", fsid_high,
                     fsid_low, voltype, voloper);

  return offset;
}


static int
fldb_dissect_setlock_resp (tvbuff_t *tvb, int offset,
                           packet_info *pinfo, proto_tree *tree,
                           dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

  MACRO_ST_CLEAR ("SetLock reply");

  return offset;
}


static int
fldb_dissect_deleteentry_resp (tvbuff_t *tvb, int offset,
                               packet_info *pinfo, proto_tree *tree,
                               dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

  MACRO_ST_CLEAR ("DeleteEntry reply");

  return offset;
}


static int
fldb_dissect_deleteentry_rqst (tvbuff_t *tvb, int offset,
                               packet_info *pinfo, proto_tree *tree,
                               dcerpc_info *di, guint8 *drep)
{
/*
                [in] afsHyper *Volid,
                [in] unsigned32 voltype
*/

  guint32 fsid_high, fsid_low, voltype, voloper;

  if (di->conformant_run)
    {
      return offset;
    }

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_deleteentry_rqst_fsid_high, &fsid_high);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_deleteentry_rqst_fsid_low, &fsid_low);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_deleteentry_rqst_voltype, &voltype);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_deleteentry_rqst_voloper, &voloper);

  col_append_fstr (pinfo->cinfo, COL_INFO, " :FSID:%u/%u", fsid_high,
                     fsid_low);


  return offset;
}


static int
fldb_dissect_createentry_resp (tvbuff_t *tvb, int offset,
                               packet_info *pinfo, proto_tree *tree,
                               dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

  MACRO_ST_CLEAR ("CreateEntry reply");

  return offset;
}


static int
fldb_dissect_createentry_rqst (tvbuff_t *tvb, int offset,
                               packet_info *pinfo, proto_tree *tree,
                               dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

  offset = dissect_vldbentry (tvb, offset, pinfo, tree, di, drep);
  return offset;
}


static int
fldb_dissect_getentrybyid_rqst (tvbuff_t *tvb, int offset,
                                packet_info *pinfo, proto_tree *tree,
                                dcerpc_info *di, guint8 *drep)
{
  guint32 volid_high, volid_low, voltype;

  if (di->conformant_run)
    {
      return offset;
    }

/*
                [in] afsHyper *Volid,
                [in] unsigned32 voltype,
*/

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_volid_high,
                        &volid_high);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_volid_low,
                        &volid_low);

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_voltype,
                        &voltype);

  col_append_fstr (pinfo->cinfo, COL_INFO, " VolID:%u/%u VolType:0x%x",
                     volid_high, volid_low, voltype);

  return offset;
}


static int
fldb_dissect_getnewvolumeids_rqst (tvbuff_t *tvb, int offset,
                                   packet_info *pinfo, proto_tree *tree,
                                   dcerpc_info *di, guint8 *drep)
{
/*              [in] unsigned32 numWanted,
                [in] afsNetAddr *ServerAddr,
*/
  guint32 numwanted;

  if (di->conformant_run)
    {
      return offset;
    }

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_numwanted,
                        &numwanted);
  col_append_fstr (pinfo->cinfo, COL_INFO, " numWanted:%u", numwanted);

  offset = dissect_afsnetaddr (tvb, offset, pinfo, tree, di, drep);

  return offset;
}


static int
fldb_dissect_getentrybyid_resp (tvbuff_t *tvb, int offset,
                                packet_info *pinfo, proto_tree *tree,
                                dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

  offset = dissect_vldbentry (tvb, offset, pinfo, tree, di, drep);
  return offset;
}


static int
fldb_dissect_releaselock_resp (tvbuff_t *tvb, int offset,
                               packet_info *pinfo, proto_tree *tree,
                               dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

  MACRO_ST_CLEAR ("ReleaseLock reply");


  return offset;
}


static int
fldb_dissect_releaselock_rqst (tvbuff_t *tvb, int offset,
                               packet_info *pinfo, proto_tree *tree,
                               dcerpc_info *di, guint8 *drep)
{
  guint32 fsid_high, fsid_low, voltype, voloper;

  if (di->conformant_run)
    {
      return offset;
    }

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_releaselock_rqst_fsid_high, &fsid_high);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_releaselock_rqst_fsid_low, &fsid_low);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_releaselock_rqst_voltype, &voltype);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_releaselock_rqst_voloper, &voloper);

  col_append_fstr (pinfo->cinfo, COL_INFO, " :FSID:%u/%u", fsid_high,
                     fsid_low);

  return offset;

}


static int
fldb_dissect_replaceentry_resp (tvbuff_t *tvb, int offset,
                                packet_info *pinfo, proto_tree *tree,
                                dcerpc_info *di, guint8 *drep)
{
  if (di->conformant_run)
    {
      return offset;
    }

  MACRO_ST_CLEAR ("ReplaceEntry reply");

  return offset;
}


static int
fldb_dissect_getnextserversbyid_resp (tvbuff_t *tvb, int offset,
                                      packet_info *pinfo, proto_tree *tree,
                                      dcerpc_info *di, guint8 *drep)
{
  guint32 nextstartp, flagsp;

  if (di->conformant_run)
    {
      return offset;
    }

  /*    [out] unsigned32 *nextStartP, */
/* XXX */
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_nextstartp,
                        &nextstartp);
  col_append_fstr (pinfo->cinfo, COL_INFO, " nextStartP:%u", nextstartp);

  /*  [out] vldbentry *entry, */
  offset = dissect_vldbentry (tvb, offset, pinfo, tree, di, drep);


  /* [out] unsigned32 *flagsP */

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep, hf_fldb_flagsp,
                        &flagsp);

  col_append_fstr (pinfo->cinfo, COL_INFO, " flagsp:%u", flagsp);

  return offset;
}


static int
fldb_dissect_replaceentry_rqst (tvbuff_t *tvb, int offset,
                                packet_info *pinfo, proto_tree *tree,
                                dcerpc_info *di, guint8 *drep)
{
  guint32 fsid_high, fsid_low, voltype;

  if (di->conformant_run)
    {
      return offset;
    }

  /*
   * [in] afsHyper *Volid,
   * [in] unsigned32 voltype,
   * [in] vldbentry *newentry,
   * [in] unsigned32 ReleaseType
   */

  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_replaceentry_rqst_fsid_high, &fsid_high);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_replaceentry_rqst_fsid_low, &fsid_low);
  offset =
    dissect_ndr_uint32 (tvb, offset, pinfo, tree, di, drep,
                        hf_fldb_replaceentry_rqst_voltype, &voltype);

  col_append_fstr (pinfo->cinfo, COL_INFO, " FSID:%u/%u Name:", fsid_high,
                   fsid_low);

  offset = dissect_vldbentry (tvb, offset, pinfo, tree, di, drep);

  return offset;

}


static dcerpc_sub_dissector fldb_dissectors[] = {
  {0, "GetEntryByID", fldb_dissect_getentrybyid_rqst,
   fldb_dissect_getentrybyid_resp},
  {1, "GetEntryByName", fldb_dissect_getentrybyname_rqst,
   fldb_dissect_getentrybyname_resp},
  {2, "Probe", NULL, NULL},
  {3, "GetCellInfo", NULL, fldb_dissect_getcellinfo_resp},
  {4, "GetNextServersByID", NULL, fldb_dissect_getnextserversbyid_resp},
  {5, "GetNextServersByName", NULL, NULL},
  {6, "GetSiteInfo", fldb_dissect_getsiteinfo_rqst,
   fldb_dissect_getsiteinfo_resp},
  {7, "GetCEntryByID", NULL, NULL},
  {8, "GetCEntryByName", NULL, NULL},
  {9, "GetCNextServersByID", NULL, NULL},
  {10, "GetCNextServersByName", NULL, NULL},
  {11, "ExpandSiteCookie", NULL, NULL},
  {12, "GetServerInterfaces", NULL, NULL},
  {13, "CreateEntry", fldb_dissect_createentry_rqst,
   fldb_dissect_createentry_resp},
  {14, "DeleteEntry", fldb_dissect_deleteentry_rqst,
   fldb_dissect_deleteentry_resp},
  {15, "GetNewVolumeId", NULL, NULL},
  {16, "ReplaceEntry", fldb_dissect_replaceentry_rqst,
   fldb_dissect_replaceentry_resp},
  {17, "SetLock", fldb_dissect_setlock_rqst, fldb_dissect_setlock_resp},
  {18, "ReleaseLock", fldb_dissect_releaselock_rqst,
   fldb_dissect_releaselock_resp},
  {19, "ListEntry", fldb_dissect_listentry_rqst, fldb_dissect_listentry_resp},
  {20, "ListByAttributes", NULL, NULL},
  {21, "GetStats", NULL, NULL},
  {22, "AddAddress", NULL, NULL},
  {23, "RemoveAddress", NULL, NULL},
  {24, "ChangeAddress", NULL, NULL},
  {25, "GenerateSites", NULL, NULL},
  {26, "GetNewVolumeIds", fldb_dissect_getnewvolumeids_rqst, NULL},
  {27, "CreateServer", NULL, NULL},
  {28, "AlterServer", NULL, NULL},
  {0, NULL, NULL, NULL},
};


void
proto_register_fldb (void)
{
  static hf_register_info hf[] = {
    {&hf_fldb_releaselock_rqst_fsid_low,
     {"FSID releaselock Low", "fldb.releaselock_rqst_fsid_low", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_releaselock_rqst_voltype,
     {"voltype", "fldb.releaselock_rqst_voltype", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_releaselock_rqst_voloper,
     {"voloper", "fldb.releaselock_rqst_voloper", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_setlock_rqst_fsid_high,
     {"FSID setlock Hi", "fldb.setlock_rqst_fsid_high", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_setlock_rqst_fsid_low,
     {"FSID setlock Low", "fldb.setlock_rqst_fsid_low", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_setlock_rqst_voltype,
     {"voltype", "fldb.setlock_rqst_voltype", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_setlock_rqst_voloper,
     {"voloper", "fldb.setlock_rqst_voloper", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
#if 0
    {&hf_fldb_setlock_resp_st,
     {"Error", "fldb.setlock_resp_st", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_setlock_resp_st2,
     {"Error", "fldb.setlock_resp_st2", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
#endif
    {&hf_fldb_listentry_rqst_previous_index,
     {"Previous Index", "fldb.listentry_rqst_previous_index", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_listentry_rqst_var1,
     {"Var 1", "fldb.listentry_rqst_var1", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_listentry_resp_count,
     {"Count", "fldb.listentry_resp_count", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_listentry_resp_next_index,
     {"Next Index", "fldb.listentry_resp_next_index", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
#if 0
    {&hf_fldb_listentry_resp_key_size,
     {"Key Size", "fldb.listentry_resp_key_size", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_listentry_resp_key_t,
     {"Volume", "fldb.listentry_resp_key_t", FT_STRING, BASE_NONE, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_listentry_resp_voltype,
     {"VolType", "fldb.listentry_resp_voltype", FT_UINT32, BASE_HEX, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_listentry_resp_key_size2,
     {"key_size2", "fldb.listentry_resp_key_size2", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_listentry_resp_key_t2,
     {"Server", "fldb.listentry_resp_key_t2", FT_STRING, BASE_NONE, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_createentry_rqst_key_t,
     {"Volume", "fldb.createentry_rqst_key_t", FT_STRING, BASE_NONE, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_createentry_rqst_key_size,
     {"Volume Size", "fldb.createentry_rqst_key_size", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
#endif
    {&hf_fldb_deleteentry_rqst_fsid_high,
     {"FSID deleteentry Hi", "fldb.deleteentry_rqst_fsid_high", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_deleteentry_rqst_fsid_low,
     {"FSID deleteentry Low", "fldb.deleteentry_rqst_fsid_low", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_deleteentry_rqst_voltype,
     {"voltype", "fldb.deleteentry_rqst_voltype", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_deleteentry_rqst_voloper,
     {"voloper", "fldb.deleteentry_rqst_voloper", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
#if 0
    {&hf_fldb_getentrybyid_rqst_fsid_high,
     {"FSID deleteentry Hi", "fldb.getentrybyid_rqst_fsid_high", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyid_rqst_fsid_low,
     {"FSID getentrybyid Low", "fldb.getentrybyid_rqst_fsid_low",
      FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyid_rqst_voltype,
     {"voltype", "fldb.getentrybyid_rqst_voltype", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyid_rqst_voloper,
     {"voloper", "fldb.getentrybyid_rqst_voloper", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
#endif
    {&hf_fldb_replaceentry_rqst_fsid_high,
     {"FSID replaceentry Hi", "fldb.replaceentry_rqst_fsid_high",
      FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_replaceentry_rqst_fsid_low,
     {"FSID  replaceentry Low", "fldb.replaceentry_rqst_fsid_low",
      FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_replaceentry_rqst_voltype,
     {"voltype", "fldb.replaceentry_rqst_voltype", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
#if 0
    {&hf_fldb_replaceentry_rqst_key_t,
     {"Key", "fldb.replaceentry_rqst_key_t", FT_STRING, BASE_NONE, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_replaceentry_rqst_key_size,
     {"Key Size", "fldb.replaceentry_rqst_key_size", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_replaceentry_resp_st,
     {"Error", "fldb.replaceentry_resp_st", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_replaceentry_resp_st2,
     {"Error", "fldb.replaceentry_resp_st2", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_volumetype,
     {"fldb_getentrybyname_resp_volumetype",
      "fldb.getentrybyname_resp_volumetype", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_numservers,
     {"fldb_getentrybyname_resp_numservers",
      "fldb.getentrybyname_resp_numservers", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_sitecookies,
     {"fldb_getentrybyname_resp_sitecookies",
      "fldb.getentrybyname_resp_sitecookies", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_sitepartition,
     {"fldb_getentrybyname_resp_sitepartition",
      "fldb.getentrybyname_resp_sitepartition", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_siteflags,
     {"fldb_getentrybyname_resp_siteflags",
      "fldb.getentrybyname_resp_siteflags", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_sitemaxreplat,
     {"fldb_getentrybyname_resp_sitemaxreplat",
      "fldb.getentrybyname_resp_sitemaxreplat", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_volid_high,
     {"fldb_getentrybyname_resp_volid_high",
      "fldb.getentrybyname_resp_volid_high", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_volid_low,
     {"fldb_getentrybyname_resp_volid_low",
      "fldb.getentrybyname_resp_volid_low", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_voltype,
     {"fldb_getentrybyname_resp_voltype",
      "fldb.getentrybyname_resp_voltype", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_cloneid_high,
     {"fldb_getentrybyname_resp_cloneid_high",
      "fldb.getentrybyname_resp_cloneid_high", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_cloneid_low,
     {"fldb_getentrybyname_resp_cloneid_low",
      "fldb.getentrybyname_resp_cloneid_low", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_flags,
     {"fldb_getentrybyname_resp_flags",
      "fldb.getentrybyname_resp_flags", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_getentrybyname_resp_maxtotallat,
     {"fldb_getentrybyname_resp_maxtotallat",
      "fldb.getentrybyname_resp_maxtotallat", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_hardmaxtotlat,
     {"fldb_getentrybyname_resp_hardmaxtotlat",
      "fldb.getentrybyname_resp_hardmaxtotlat", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_minpouncedally,
     {"fldb_getentrybyname_resp_minpouncedally",
      "fldb.getentrybyname_resp_minpouncedally", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_defaultmaxreplat,
     {"fldb_getentrybyname_resp_defaultmaxreplat",
      "fldb.getentrybyname_resp_defaultmaxreplat", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_reclaimdally,
     {"fldb_getentrybyname_resp_reclaimdally",
      "fldb.getentrybyname_resp_reclaimdally", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_whenlocked,
     {"fldb_getentrybyname_resp_whenlocked",
      "fldb.getentrybyname_resp_whenlocked", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_spare1,
     {"fldb_getentrybyname_resp_spare1",
      "fldb.getentrybyname_resp_spare1", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_spare2,
     {"fldb_getentrybyname_resp_spare2",
      "fldb.getentrybyname_resp_spare2", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_spare3,
     {"fldb_getentrybyname_resp_spare3",
      "fldb.getentrybyname_resp_spare3", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_spare4,
     {"fldb_getentrybyname_resp_spare4",
      "fldb.getentrybyname_resp_spare4", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_key_t,
     {"fldb_getentrybyname_resp_key_t",
      "fldb.getentrybyname_resp_key_t", FT_STRING, BASE_NONE, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_key_size,
     {"fldb_getentrybyname_resp_key_size",
      "fldb.getentrybyname_resp_key_size", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_getentrybyname_resp_test,
     {"fldb_getentrybyname_resp_test", "fldb.getentrybyname_resp_test",
      FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}},
#endif
    {&hf_fldb_releaselock_rqst_fsid_high,
     {"FSID releaselock Hi", "fldb.releaselock_rqst_fsid_high", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_opnum,
     {"Operation", "fldb.opnum", FT_UINT16, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_volumename,
     {"VolumeName", "fldb.vldbentry.volumename", FT_STRING, BASE_NONE, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_vldbentry_volumetype,
     {"VolumeType", "fldb.vldbentry.volumetype", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_vldbentry_nservers,
     {"Number of Servers", "fldb.vldbentry.nservers", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_afsnetaddr_type,
     {"Type", "fldb.afsnetaddr.type", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_afsnetaddr_data,
     {"IP Data", "fldb.afsnetaddr.data", FT_UINT8, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_sitepartition,
     {"Site Partition", "fldb.vldbentry.sitepartition", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_siteflags,
     {"Site Flags", "fldb.vldbentry.siteflags", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_sitemaxreplicalatency,
     {"Site Max Replica Latench", "fldb.vldbentry.sitemaxreplatency", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vldbentry_siteprincipal,
     {"Principal Name", "fldb.vldbentry.siteprincipal", FT_STRING, BASE_NONE, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_vldbentry_siteowner,
     {"Site Owner", "fldb.vldbentry.siteowner", FT_GUID, BASE_NONE, NULL, 0x0,
      "UUID", HFILL}},
    {&hf_fldb_vldbentry_siteobjid,
     {"Site Object ID", "fldb.vldbentry.siteobjid", FT_GUID, BASE_NONE, NULL,
      0x0, "UUID", HFILL}},
    {&hf_fldb_vldbentry_volids_high,
     {"VolIDs high", "fldb.vldbentry.volidshigh", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_vldbentry_volids_low,
     {"VolIDs low", "fldb.vldbentry.volidslow", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_voltypes,
     {"VolTypes", "fldb.vldbentry.voltypes", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_cloneid_high,
     {"CloneID High", "fldb.vldbentry.cloneidhigh", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_vldbentry_cloneid_low,
     {"CloneID Low", "fldb.vldbentry.cloneidlow", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_afsflags_flags,
     {"AFS Flags", "fldb.vldbentry.afsflags", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_maxtotallatency,
     {"Max Total Latency", "fldb.vldbentry.maxtotallatency", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vldbentry_hardmaxtotallatency,
     {"Hard Max Total Latency", "fldb.vldbentry.hardmaxtotallatency", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vldbentry_minimumpouncedally,
     {"Minimum Pounce Dally", "fldb.vldbentry.minimumpouncedally", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vldbentry_defaultmaxreplicalatency,
     {"Default Max Replica Latency", "fldb.vldbentry.defaultmaxreplicalatency",
      FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vldbentry_reclaimdally,
     {"Reclaim Dally", "fldb.vldbentry.reclaimdally", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_vldbentry_whenlocked,
     {"When Locked", "fldb.vldbentry.whenlocked", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_vldbentry_spare1,
     {"Spare 1", "fldb.vldbentry.spare1", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_spare2,
     {"Spare 2", "fldb.vldbentry.spare2", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_spare3,
     {"Spare 3", "fldb.vldbentry.spare3", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_spare4,
     {"Spare 4", "fldb.vldbentry.spare4", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vldbentry_lockername,
     {"Locker Name", "fldb.vldbentry.lockername", FT_STRING, BASE_NONE, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_vldbentry_charspares,
     {"Char Spares", "fldb.vldbentry.charspares", FT_STRING, BASE_NONE, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_vlconf_cell_name,
     {"Name", "fldb.vlconf.name", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vlconf_cell_cellid_high,
     {"CellID High", "fldb.vlconf.cellidhigh", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vlconf_cell_cellid_low,
     {"CellID Low", "fldb.vlconf.cellidlow", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vlconf_cell_numservers,
     {"Number of Servers", "fldb.vlconf.numservers", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
    {&hf_fldb_vlconf_cell_hostname,
     {"hostName", "fldb.vlconf.hostname", FT_STRING, BASE_NONE, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_vlconf_cell_spare1,
     {"Spare1", "fldb.vlconf.spare1", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vlconf_cell_spare2,
     {"Spare2", "fldb.vlconf.spare2", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vlconf_cell_spare3,
     {"Spare3", "fldb.vlconf.spare3", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vlconf_cell_spare4,
     {"Spare4", "fldb.vlconf.spare4", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_vlconf_cell_spare5,
     {"Spare5", "fldb.vlconf.spare5", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_flagsp,
     {"flagsp", "fldb.flagsp", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_nextstartp,
     {"nextstartp", "fldb.nextstartp", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_afsNameString_t_principalName_size,
     {"Principal Name Size", "fldb.principalName_size", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
#if 0
    {&hf_fldb_afsNameString_t_principalName_size2,
     {"Principal Name Size2", "fldb.principalName_size2", FT_UINT32, BASE_DEC,
      NULL, 0x0, NULL, HFILL}},
#endif
    {&hf_fldb_afsNameString_t_principalName_string,
     {"Principal Name", "fldb.NameString_principal", FT_STRING, BASE_NONE,
      NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_namestring,
     {"Name string", "fldb.NameString_principal", FT_STRING, BASE_NONE, NULL,
      0x0, NULL, HFILL}},
#if 0
    {&hf_dcerpc_error_status,
     {"Error Status", "fldb.NameString_principal", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
#endif
    {&hf_error_st,
     {"Error Status 2", "fldb.error_st", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_creationquota,
     {"creation quota", "fldb.creationquota", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_creationuses,
     {"creation uses", "fldb.creationuses", FT_UINT32, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
    {&hf_fldb_deletedflag,
     {"deletedflag", "fldb.deletedflag", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
#if 0
    {&hf_fldb_getentrybyname_rqst_key_size,
     {"getentrybyname", "fldb.getentrybyname_rqst_key_size", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_getentrybyname_rqst_var1,
     {"getentrybyname var1", "fldb.getentrybyname_rqst_var1", FT_UINT32,
      BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_namestring_size,
     {"namestring size", "fldb.namestring_size", FT_UINT32, BASE_DEC, NULL,
      0x0, NULL, HFILL}},
#endif
    {&hf_fldb_numwanted,
     {"number wanted", "fldb.numwanted", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_spare2,
     {"spare2", "fldb.spare2", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_spare3,
     {"spare3", "fldb.spare3", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_spare4,
     {"spare4", "fldb.spare4", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_spare5,
     {"spare5", "fldb.spare5", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
    {&hf_fldb_uuid_objid,
     {"objid", "fldb.uuid_objid", FT_GUID, BASE_NONE, NULL, 0x0, "UUID",
      HFILL}},
    {&hf_fldb_uuid_owner,
     {"owner", "fldb.uuid_owner", FT_GUID, BASE_NONE, NULL, 0x0, "UUID",
      HFILL}},
    {&hf_fldb_volid_high,
     {"volid high", "fldb.volid_high", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_volid_low,
     {"volid low", "fldb.volid_low", FT_UINT32, BASE_DEC, NULL, 0x0, NULL,
      HFILL}},
    {&hf_fldb_voltype,
     {"voltype", "fldb.voltype", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL}},
  };

  static gint *ett[] = {
    &ett_fldb,
    &ett_fldb_vldbentry,
    &ett_fldb_afsnetaddr,
    &ett_fldb_siteflags,
    &ett_fldb_afsflags,
    &ett_fldb_vlconf_cell,
    &ett_fldb_afsNameString_t,
  };

  proto_fldb = proto_register_protocol ("DCE DFS Fileset Location Server", "FLDB", "fldb");
  proto_register_field_array (proto_fldb, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));
}


void
proto_reg_handoff_fldb (void)
{
  /* Register the protocol as dcerpc */
  dcerpc_init_uuid (proto_fldb, ett_fldb, &uuid_fldb, ver_fldb,
                    fldb_dissectors, hf_fldb_opnum);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
