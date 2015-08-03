<xsl:stylesheet version="1.0" 
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:ows="http://www.opengis.net/ows/1.1"
                xmlns:wps="http://www.opengis.net/wps/1.0.0"
                xmlns:xlink="http://www.w3.org/1999/xlink">
  <!--
      Author : Gérald FENOY
      
      Copyright 2015 GeoLabs SARL. All rights reserved.
      
      Permission is hereby granted, free of charge, to any person obtaining a copy
      of this software and associated documentation files (the "Software"), to deal
      in the Software without restriction, including without limitation the rights
      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
      copies of the Software, and to permit persons to whom the Software is
      furnished to do so, subject to the following conditions:
      
      The above copyright notice and this permission notice shall be included in
      all copies or substantial portions of the Software.
      
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
      THE SOFTWARE.
      
  -->
  <xsl:output method="text"/>

  <xsl:template match="/*">
    <xsl:for-each select="./ProcessDescription">
      <xsl:call-template name="extractBasicMetaData">
	<xsl:with-param name="context" select="." /> 
	<xsl:with-param name="total" select="1" /> 
      </xsl:call-template>
      <xsl:text>storeSupported=true</xsl:text>
      <xsl:text>&#xa;</xsl:text>
      <xsl:text>statusSupported=true</xsl:text>
      <xsl:text>&#xa;</xsl:text>
      <xsl:text>serviceType=Python</xsl:text>
      <xsl:text>&#xa;</xsl:text>
      <xsl:text>serviceProvider=</xsl:text>
      <xsl:value-of select="translate(./ows:Identifier,'.','_')" />      
      <xsl:text>&#xa;</xsl:text>
      <xsl:text>&lt;DataInputs&gt;</xsl:text>
      <xsl:text>&#xa;</xsl:text>
      <xsl:for-each select="./DataInputs/Input">
	<xsl:call-template name="extractBasicMetaData">
	  <xsl:with-param name="context" select="." /> 
	  <xsl:with-param name="total" select="3" /> 
	</xsl:call-template>
	<xsl:call-template name="printSpace">
	  <xsl:with-param name="index" select="1" />
	  <xsl:with-param name="total" select="3" />
	</xsl:call-template>
	<xsl:text>minOccurs = </xsl:text>
	<xsl:value-of select="./@minOccurs" />
	<xsl:text>&#xa;</xsl:text>
	<xsl:call-template name="printSpace">
	  <xsl:with-param name="index" select="1" />
	  <xsl:with-param name="total" select="3" />
	</xsl:call-template>
	<xsl:text>maxOccurs = </xsl:text>
	<xsl:value-of select="./@maxOccurs" />
	<xsl:text>&#xa;</xsl:text>
	<xsl:call-template name="extractLiteralData">
	  <xsl:with-param name="context" select=". /LiteralData" /> 
	</xsl:call-template>
	<xsl:call-template name="extractComplexData">
	  <xsl:with-param name="context" select=". /ComplexData" /> 
	</xsl:call-template>
      </xsl:for-each>
      <xsl:text>&lt;/DataInputs&gt;</xsl:text>
      <xsl:text>&#xa;</xsl:text>
      <xsl:text>&lt;DataOutputs&gt;</xsl:text>
      <xsl:text>&#xa;</xsl:text>
      <xsl:for-each select="./ProcessOutputs/Output">
	<xsl:call-template name="extractBasicMetaData">
	  <xsl:with-param name="context" select="." /> 
	  <xsl:with-param name="total" select="3" /> 
	</xsl:call-template>
	<xsl:call-template name="extractComplexData">
	  <xsl:with-param name="context" select=". /ComplexOutput" /> 
	  <xsl:with-param name="total" select="3" /> 
	</xsl:call-template>
      </xsl:for-each>
      <xsl:text>&lt;/DataOutputs&gt;</xsl:text>
      <xsl:text>&#xa;</xsl:text>
    </xsl:for-each>

  </xsl:template>

  <xsl:template name="extractBasicMetaData">
    <xsl:param name="context" />
    <xsl:param name="index" select="1" />
    <xsl:param name="total" select="1" />
    <xsl:call-template name="printSpace">
      <xsl:with-param name="index" select="1" />
      <xsl:with-param name="total" select="$total" />
    </xsl:call-template>
    <xsl:text>[</xsl:text>
    <xsl:copy-of select="translate($context/ows:Identifier,'.','_')" />
    <xsl:text>]</xsl:text>
    <xsl:text>&#xa;</xsl:text>
    <xsl:call-template name="printSpace">
      <xsl:with-param name="index" select="1" />
      <xsl:with-param name="total" select="$total" />
    </xsl:call-template>
    <xsl:text>Title = </xsl:text>
    <xsl:copy-of select="$context/ows:Title" />
    <xsl:text>&#xa;</xsl:text>
    <xsl:call-template name="printSpace">
      <xsl:with-param name="index" select="1" />
      <xsl:with-param name="total" select="$total" />
    </xsl:call-template>
    <xsl:text>Abstract = </xsl:text>
    <xsl:copy-of select="$context/ows:Abstract" />
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <xsl:template name="printSpace">
    <xsl:param name="index" select="1"/>
    <xsl:param name="total" />
    <xsl:if test="not($index = $total)">
      <xsl:text> </xsl:text>
      <xsl:call-template name="printSpace">
	<xsl:with-param name="index" select="$index + 1" />
	<xsl:with-param name="total" select="$total" />
      </xsl:call-template>	
    </xsl:if>
  </xsl:template>
  
  <xsl:template name="extractLiteralData">
    <xsl:param name="context" />
    <xsl:for-each select="$context">
      <xsl:call-template name="printSpace">
	<xsl:with-param name="index" select="1" />
	<xsl:with-param name="total" select="3" />
      </xsl:call-template>
      <xsl:text>&lt;LiteralData&gt;</xsl:text>
      <xsl:text>&#xa;</xsl:text>
      <xsl:call-template name="printSpace">
	<xsl:with-param name="index" select="1" />
	<xsl:with-param name="total" select="4" />
      </xsl:call-template>
      <xsl:text>dataType=</xsl:text>
      <xsl:copy-of select="./ows:DataType" />
      <xsl:text>&#xa;</xsl:text>
      <xsl:if test="./ows:AllowedValues">
	<xsl:call-template name="printSpace">
	  <xsl:with-param name="index" select="1" />
	  <xsl:with-param name="total" select="4" />
	</xsl:call-template>
	<xsl:text>AllowedValues=</xsl:text>
	<xsl:for-each select="./ows:AllowedValues/ows:Value">
	  <xsl:copy-of select="." /><xsl:if test="position() != last()" ><xsl:text>,</xsl:text></xsl:if>
	</xsl:for-each>
	<xsl:text>&#xa;</xsl:text>
      </xsl:if>
      <xsl:choose>
	<xsl:when test="./DefaultValue">
	  <xsl:call-template name="printSpace">
	    <xsl:with-param name="index" select="1" />
	    <xsl:with-param name="total" select="4" />
	  </xsl:call-template>
	  <xsl:text>&lt;Default&gt;</xsl:text>
	  <xsl:text>&#xa;</xsl:text>
	  <xsl:call-template name="printSpace">
	    <xsl:with-param name="index" select="1" />
	    <xsl:with-param name="total" select="5" />
	  </xsl:call-template>
	  <xsl:text>value=</xsl:text>
	  <xsl:copy-of select="./DefaultValue" />
	  <xsl:text>&#xa;</xsl:text>
	  <xsl:call-template name="printSpace">
	    <xsl:with-param name="index" select="1" />
	    <xsl:with-param name="total" select="4" />
	  </xsl:call-template>
	  <xsl:text>&lt;/Default&gt;</xsl:text>
	  <xsl:text>&#xa;</xsl:text>  
	</xsl:when>
	<xsl:otherwise>
	  <xsl:call-template name="printSpace">
	    <xsl:with-param name="index" select="1" />
	    <xsl:with-param name="total" select="4" />
	  </xsl:call-template>
	  <xsl:text>&lt;Default /&gt;</xsl:text>
	  <xsl:text>&#xa;</xsl:text>
	</xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="printSpace">
	<xsl:with-param name="index" select="1" />
	<xsl:with-param name="total" select="3" />
      </xsl:call-template>
      <xsl:text>&lt;/LiteralData&gt;</xsl:text>
      <xsl:text>&#xa;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="extractComplexFormat">
    <xsl:param name="context" />
    <xsl:for-each select="$context">
	<xsl:text>    mimeType=</xsl:text>
	<xsl:copy-of select="./MimeType" />
	<xsl:text>&#xa;</xsl:text>
	<xsl:if test="./Encoding">
	  <xsl:text>    encoding=</xsl:text>
	  <xsl:copy-of select="./Encoding" />
	  <xsl:text>&#xa;</xsl:text>
	</xsl:if>
	<xsl:if test="./Schema">
	  <xsl:text>    schema=</xsl:text>
	  <xsl:copy-of select="./Schema" />
	  <xsl:text>&#xa;</xsl:text>
	</xsl:if>
      </xsl:for-each>
  </xsl:template>
  
  <xsl:template name="extractComplexData">
    <xsl:param name="context" />
    <xsl:for-each select="$context">
      <xsl:text>  &lt;ComplexData&gt;</xsl:text>
      <xsl:text>&#xa;</xsl:text>
      <xsl:text>   &lt;Default&gt;</xsl:text>
      <xsl:text>&#xa;</xsl:text>
      <xsl:call-template name="extractComplexFormat">
	<xsl:with-param name="context" select=". /Default/Format" /> 
      </xsl:call-template>
      <xsl:text>   &lt;/Default&gt;</xsl:text>
      <xsl:for-each select="./Supported/Format">
	<xsl:text>&#xa;</xsl:text>
	<xsl:text>   &lt;Supported&gt;</xsl:text>
	<xsl:text>&#xa;</xsl:text>
	<xsl:call-template name="extractComplexFormat">
	  <xsl:with-param name="context" select=". " /> 
	</xsl:call-template>
	<xsl:text>   &lt;/Supported&gt;</xsl:text>	
      </xsl:for-each>
      <xsl:text>&#xa;</xsl:text>
      <xsl:text>  &lt;/ComplexData&gt;</xsl:text>	
      <xsl:text>&#xa;</xsl:text>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>
