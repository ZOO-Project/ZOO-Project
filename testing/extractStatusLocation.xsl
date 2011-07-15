<?xml version="1.0"  encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:ows="http://www.opengis.net/ows/1.1"
		xmlns:wps="http://www.opengis.net/wps/1.0.0"
		xmlns:xlink="http://www.w3.org/1999/xlink">

  <xsl:output method="text"/>

  <xsl:template match="wps:ExecuteResponse">
    <xsl:value-of select="@statusLocation" />
  </xsl:template> 

</xsl:stylesheet>
