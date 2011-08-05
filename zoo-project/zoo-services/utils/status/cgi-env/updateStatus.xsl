<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:ows="http://www.opengis.net/ows/1.1"
                xmlns:wps="http://www.opengis.net/wps/1.0.0"
                xmlns:xlink="http://www.w3.org/1999/xlink">

  <xsl:output method="xml"/>
  <xsl:param name="value" select="string('-1')"/>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="/wps:ExecuteResponse/wps:Status/wps:ProcessStarted/@percentCompleted">
    <xsl:attribute name="percentCompleted">
      <xsl:value-of select="$value"/>
    </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
