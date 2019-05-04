<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:ows="http://www.opengis.net/ows/1.1"
                xmlns:ns14="http://www.opengis.net/wps/2.0"
                xmlns:wps="http://www.opengis.net/wps/2.0"
                xmlns:xlink="http://www.w3.org/1999/xlink">

  <!--
      This template is used by the ZOO-Kernel to generate a specific
      OGC WPS Execute request in case of rest service callback use.
      In such a case the ZOO-Kernel replace every input provided by
      value in the initial request and replace the data section by the
      reference to the published ressource.
      So, this template is basically made to replace any input provided
      by value to the corresponding local WCS/WFS OGC web service
      references.
      --> 
  <xsl:output method="xml"/>
  <xsl:param name="value" select="string('-1')"/>
  <xsl:param name="attr" select="string('-1')"/>
  <xsl:param name="cnt" select="string('-1')"/>
  <xsl:param name="index" select="0"/>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  
  <xsl:template match="/wps:Execute/wps:Input">
    <xsl:choose>
      <xsl:when test="@id=$attr">
        <xsl:copy>
          <xsl:attribute name="id">
            <xsl:value-of select="@id"/>
          </xsl:attribute>
          <ns14:Reference>
            <xsl:attribute name="xlink:href">
              <xsl:value-of select="$value"/>
            </xsl:attribute>
            <xsl:if test="./wps:Data[@mimeType!='']">
              <xsl:attribute name="mimeType">
                <xsl:value-of select="./wps:Data[@mimeType]"/>
              </xsl:attribute>
              <xsl:attribute name="encoding">
                <xsl:value-of select="./wps:Data[@encoding]"/>
              </xsl:attribute>
              <xsl:attribute name="schema">
                <xsl:value-of select="./wps:Data[@schema]"/>
              </xsl:attribute>
            </xsl:if>
          </ns14:Reference>
        </xsl:copy>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="."/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  
</xsl:stylesheet>
