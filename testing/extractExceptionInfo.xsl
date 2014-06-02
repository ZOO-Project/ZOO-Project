<?xml version="1.0"  encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:ows="http://www.opengis.net/ows/1.1"
		xmlns:xlink="http://www.w3.org/1999/xlink">

  <xsl:output method="text"/>

  <xsl:template match="*/*">
      <xsl:text>Code: </xsl:text><xsl:value-of select="@exceptionCode" /><xsl:text>, Locator: </xsl:text><xsl:value-of select="@locator" />
  </xsl:template> 

</xsl:stylesheet>
