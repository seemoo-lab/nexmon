/******************************************************************************
** Copyright (C) 2006-2009 ascolab GmbH. All Rights Reserved.
** Web: http://www.ascolab.com
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Project: OpcUa Wireshark Plugin
**
** Description: Service table and service dispatcher.
**
******************************************************************************/

void dispatchService(proto_tree *tree, tvbuff_t *tvb, packet_info *pinfo, gint *pOffset, int ServiceId);

