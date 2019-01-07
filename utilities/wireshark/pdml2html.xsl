<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- This XSLT will convert a PDML file, saved by Wireshark, into
     HTML. The HTML page should look like Wireshark. For questions contact
     Dirk Jagdmann (doj@cubic.org).
     Version: 2010-06-09

     Wireshark - Network traffic analyzer
     By Gerald Combs <gerald@wireshark.org>
     Copyright 1998 Gerald Combs

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
     -->

<!-- set parameters of the HTML output -->
<xsl:output method="html" encoding="UTF-8" omit-xml-declaration="no" standalone="yes" indent="yes"/>

<!-- this matches the "field" tag -->
<xsl:template match="field">
  &#160;&#160;&#160; <!-- indent with 3 non-breaking spaces -->

  <!-- output either the "showname" or "show" attribute -->
  <xsl:choose>
    <xsl:when test="string-length(@showname)>0">
      <xsl:value-of select="@showname"/><br/>
    </xsl:when>
    <xsl:otherwise>
      <!--<xsl:value-of select="@name"/>:--> <xsl:value-of select="@show"/><br/>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:apply-templates/> <!-- we expect to match "field" tags -->
</xsl:template>

<!-- this matches the "packet" tag -->
<xsl:template match="packet">

  <!-- declare some variables for later use -->
  <xsl:variable name="frame_num" select="proto[@name='frame']/field[@name='frame.number']/@show"/>
  <xsl:variable name="frame_id"  select="concat('f',$frame_num)"/>
  <xsl:variable name="frame_c"   select="concat($frame_id,'c')"/>

  <!-- the "title" bar of the frame -->
  <div width="100%" id="{$frame_id}">
    <a href="javascript:toggle_node('{$frame_c}')">&#8658;</a> <!-- #8658 is a "rArr" (double right arrow) character -->
    Frame <xsl:value-of select="$frame_num"/>:
    <xsl:for-each select="proto[@name!='geninfo']">
      <xsl:value-of select="@name"/>,
    </xsl:for-each>
    <small><a href="javascript:hide_node('{$frame_id}')">[X]</a></small>
  </div>

  <!-- the frame contents are stored in a div, so we can toggle it -->
  <div width="100%" id="{$frame_c}" style="display:none">
    <!-- loop through all proto tags, but skip the "geninfo" one -->
    <xsl:for-each select="proto[@name!='geninfo']">

      <xsl:variable name="proto" select="concat($frame_id,@name,count(preceding-sibling::proto)+1)"/>

      <!-- the "title" bar of the proto -->
      <div width="100%" style="background-color:#e5e5e5; margin-bottom: 2px">
        &#160;<a href="javascript:toggle_node('{$proto}')">&#8658;</a>&#160;<xsl:value-of select="@showname"/>

        <!-- print "proto" details inside another div -->
        <div width="100%" id="{$proto}" style="display:none">
         <xsl:apply-templates/> <!-- we expect to match "field" tags -->
        </div>
      </div>
    </xsl:for-each>
  </div>

  <!-- use the javascript function set_node_color() to set the color
       of the frame title bar. Defer colorization until the full page has
       been loaded. If the browser would support the XPath function
       replace() we could simply set the class attribute of the title bar div,
       but for now we're stuck with class names from Wireshark's colorfilters
       that contain spaces and we can't handle them in CSS. -->
  <script type="text/javascript">
    dojo.addOnLoad(function(){
      set_node_color(
        '<xsl:value-of select="$frame_id"/>',
        '<xsl:value-of select="proto[@name='frame']/field[@name='frame.coloring_rule.name']/@show"/>'
      );
    });
  </script>
</xsl:template>

<xsl:template match="pdml">
  Capture Filename: <b><xsl:value-of select="@capture_file"/></b>
  PDML created: <b><xsl:value-of select="@time"/></b>
  <tt>
    <xsl:apply-templates/> <!-- we expect to match the "packet" nodes -->
  </tt>
</xsl:template>

<!-- this block matches the start of the PDML file -->
<xsl:template match="/">
  <html>
  <head>
    <title>poor man's Wireshark</title>
    <script src="http://ajax.googleapis.com/ajax/libs/dojo/1.4/dojo/dojo.xd.js" type="text/javascript"></script>
    <script type="text/javascript">
function set_node(node, str)
{
  if(dojo.isString(node))
    node = dojo.byId(node);
  if(!node) return;
  node.style.display = str;
}
function toggle_node(node)
{
  if(dojo.isString(node))
    node = dojo.byId(node);
  if(!node) return;
  set_node(node, (node.style.display != 'none') ? 'none' : 'block');
}
function hide_node(node)
{
  set_node(node, 'none');
}
// this function was generated by colorfilters2js.pl
function set_node_color(node,colorname)
{
  if(dojo.isString(node))
    node = dojo.byId(node);
  if(!node) return;
  var fg;
  var bg;
  if(colorname == 'Bad TCP') {
    bg='#000000';
    fg='#ff5f5f';
  }
  if(colorname == 'HSRP State Change') {
    bg='#000000';
    fg='#fff600';
  }
  if(colorname == 'Spanning Tree Topology  Change') {
    bg='#000000';
    fg='#fff600';
  }
  if(colorname == 'OSPF State Change') {
    bg='#000000';
    fg='#fff600';
  }
  if(colorname == 'ICMP errors') {
    bg='#000000';
    fg='#00ff0e';
  }
  if(colorname == 'ARP') {
    bg='#d6e8ff';
    fg='#000000';
  }
  if(colorname == 'ICMP') {
    bg='#c2c2ff';
    fg='#000000';
  }
  if(colorname == 'TCP RST') {
    bg='#900000';
    fg='#fff680';
  }
  if(colorname == 'TTL low or unexpected') {
    bg='#900000';
    fg='#ffffff';
  }
  if(colorname == 'Checksum Errors') {
    bg='#000000';
    fg='#ff5f5f';
  }
  if(colorname == 'SMB') {
    bg='#fffa99';
    fg='#000000';
  }
  if(colorname == 'HTTP') {
    bg='#8dff7f';
    fg='#000000';
  }
  if(colorname == 'IPX') {
    bg='#ffe3e5';
    fg='#000000';
  }
  if(colorname == 'DCERPC') {
    bg='#c797ff';
    fg='#000000';
  }
  if(colorname == 'Routing') {
    bg='#fff3d6';
    fg='#000000';
  }
  if(colorname == 'TCP SYN/FIN') {
    bg='#a0a0a0';
    fg='#000000';
  }
  if(colorname == 'TCP') {
    bg='#e7e6ff';
    fg='#000000';
  }
  if(colorname == 'UDP') {
    bg='#70e0ff';
    fg='#000000';
  }
  if(colorname == 'Broadcast') {
    bg='#ffffff';
    fg='#808080';
  }
  if(fg.length > 0)
    node.style.color = fg;
  if(bg.length > 0)
    node.style.background = bg;
}
    </script>
  </head>
    <body>
      <xsl:apply-templates/> <!-- we expect to match the "pdml" node -->
    </body>
  </html>
</xsl:template>

</xsl:stylesheet>
