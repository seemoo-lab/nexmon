﻿<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<!-- import the main stylesheet -->
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl"/>

<!-- create pdf bookmarks -->
<!-- Disable this since FOP 0.93 doesn't handle them, yet -->
<xsl:param name="fop1.extensions" select="1"/>

<!-- don't use the draft mode (no loading of image from the web) -->
<xsl:param name="draft.mode" select="no"/>

<!-- use graphics for admons (note, tip, ...) -->
<xsl:param name="admon.graphics" select="1"/>
<xsl:param name="admon.graphics.path">common_graphics/</xsl:param>
<xsl:param name="admon.graphics.extension" select="'.svg'"/>

<!-- use numbering for sections (not only for chapters) -->
<xsl:param name="section.autolabel" select="1"/>
<xsl:param name="section.label.includes.component.label" select="1"/>

<!-- include a single TOC (use book style TOC, removes the list of figures etc.) -->
<xsl:param name="generate.toc" select="'book toc'"/>

<!-- include page numbers in cross references -->
<!-- <xsl:param name="insert.xref.page.number" select="1"/> -->

<!-- don't show URL's, but only the text of it -->
<xsl:param name="ulink.show" select="0"/>

<!-- hyphenate URL's after the slash -->
<!-- (http://docbook.sourceforge.net/release/xsl/current/doc/fo/ulink.hyphenate.html) -->
<xsl:param name="ulink.hyphenate.chars" select="'/:'"></xsl:param>
<xsl:param name="ulink.hyphenate" select="'&#x200B;'"></xsl:param>

<!-- don't allow section titles to be hyphenated -->
<xsl:attribute-set name="section.title.properties">
  <xsl:attribute name="hyphenate">false</xsl:attribute>
</xsl:attribute-set>

<!-- put a page break after each section
<xsl:attribute-set name="section.level1.properties">
  <xsl:attribute name="break-after">page</xsl:attribute>
</xsl:attribute-set>
-->

<!-- set link style to blue and underlined -->
<xsl:attribute-set name="xref.properties">
  <xsl:attribute name="color">blue</xsl:attribute>
  <xsl:attribute name="text-decoration">underline</xsl:attribute>
</xsl:attribute-set>

<!-- reduce the size of programlisting to make them fit the page -->
<xsl:attribute-set name="monospace.verbatim.properties">
  <xsl:attribute name="font-size">80%</xsl:attribute>
</xsl:attribute-set>

</xsl:stylesheet>
