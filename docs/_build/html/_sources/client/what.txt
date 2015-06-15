.. _client-what:

What is ZOO-Client ?
=========================

ZOO-Client is a client-side JavaScript API which provides simple methods
for interacting with `WPS <http://www.opengeospatial.org/standards/wps/>`__ server from web
applications. It is helpful for sending requests to any WPS compliant
server (such as :ref:`kernel_index`) and to parse the output responses
using simple JavaScript.


JavaScript
-------------------------------

ZOO-Client relies on modern JavaScript libraries and can be seamlessly
integrated in new or existing web platforms or applications. ZOO-Client works by expanding the tags available in WPS specific
templates using values provided by a JavaScript hash or object. It
allows to build valid WPS requests and to send them to a WPS server. It
also provides functions to easily parse and reuse the output XML
responses. Read the :ref:`next section <client-howto>` to get started.


Templates
-------------------------------

ZOO-Client uses logic-less `Mustache <http://mustache.github.io/>`__
templates for creating well-formed WPS requests. Templates are called
*logic-less* because they do not contain any *if* statements, *else*
clauses, or *for* loops, but only **tags**. Some tags are dynamically replaced by a
value or a series of values.


GetCapabilities
..........................................................

*GetCapabilities* requests are created using the following template:

::
   
   <wps:GetCapabilities xmlns:ows="http://www.opengis.net/ows/1.1" xmlns:wps="http://www.opengis.net/wps/1.0.0" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.opengis.net/wps/1.0.0 ../wpsGetCapabilities_request.xsd" language="{{language}}" service="WPS">
    <wps:AcceptVersions>
        <ows:Version>1.0.0</ows:Version>
    </wps:AcceptVersions>
   </wps:GetCapabilities>


DescribeProcess
..........................................................

*DescribeProcess* requests are created using the following template:

::

   <DescribeProcess xmlns="http://www.opengis.net/wps/1.0.0" xmlns:ows="http://www.opengis.net/ows/1.1" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.opengis.net/wps/1.0.0 ../wpsDescribeProcess_request.xsd" service="WPS" version="1.0.0" language="{{language}}">
   {{#identifiers}}
   <ows:Identifier>{{.}}</ows:Identifier>
   {{/identifiers}}
   </DescribeProcess>


Execute
..........................................................

*Execute* requests are created using a more complex template, as shown bellow:

::

   <wps:Execute service="WPS" version="1.0.0" xmlns:wps="http://www.opengis.net/wps/1.0.0" xmlns:ows="http://www.opengis.net/ows/1.1" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.opengis.net/wps/1.0.0../wpsExecute_request.xsd" language="{{language}}">
   <!-- template-version: 0.21 -->
	<ows:Identifier>{{Identifier}}</ows:Identifier>
	<wps:DataInputs>
	{{#DataInputs}}
	{{#is_literal}}
		<wps:Input>
			<ows:Identifier>{{identifier}}</ows:Identifier>
			<wps:Data>
				<wps:LiteralData{{#dataType}} dataType="{{dataType}}"{{/dataType}}>{{value}}</wps:LiteralData>
			</wps:Data>
		</wps:Input>
		{{/is_literal}}
		{{#is_bbox}}
		<wps:Input>
			<ows:Identifier>{{identifier}}</ows:Identifier>
			<wps:Data>
				<wps:BoundingBoxData ows:crs="{{crs}}" ows:dimensions="{{dimension}}">
            <ows:LowerCorner>{{lowerCorner}}</ows:LowerCorner>
            <ows:UpperCorner>{{upperCorner}}</ows:UpperCorner>
         </wps:BoundingBoxData>
			</wps:Data>
		</wps:Input>
		{{/is_bbox}}
		{{#is_complex}}
		{{#is_reference}}
		{{#is_get}}
		<wps:Input>
			<ows:Identifier>{{identifier}}</ows:Identifier>
			<wps:Reference xlink:href="{{href}}"{{#schema}} schema="{{shema}}"{{/schema}}{{#mimeType}} mimeType="{{mimeType}}"{{/mimeType}}{{#encoding}} encoding="{{encoding}}"{{/encoding}}/>
		</wps:Input>
		{{/is_get}}
		{{#is_post}}
		<wps:Input>
			<ows:Identifier>{{identifier}}</ows:Identifier>
			<wps:Reference xlink:href="{{href}}" method="{{method}}">
			{{#headers}}
			  <wps:Header key="{{key}}" value="{{value}}" />
			  {{/headers}}
			  <wps:Body>{{{value}}}</wps:Body>
			</wps:Reference>
		</wps:Input>
		{{/is_post}}
		{{/is_reference}}
		{{^is_reference}}
		<wps:Input>
      <ows:Identifier>{{identifier}}</ows:Identifier>
      <wps:Data>
        <wps:ComplexData{{#schema}} schema="{{shema}}"{{/schema}}{{#mimeType}} mimeType="{{mimeType}}"{{/mimeType}}{{#encoding}} encoding="{{encoding}}"{{/encoding}}>{{#is_XML}}
	 {{{value}}}{{/is_XML}}{{^is_XML}}<![CDATA[{{{value}}}]]>{{/is_XML}}
        </wps:ComplexData>
      </wps:Data>
    </wps:Input>
    {{/is_reference}}
    {{/is_complex}}
    {{/DataInputs}}
	</wps:DataInputs>	
	<wps:ResponseForm>
	{{#RawDataOutput}}
	{{#DataOutputs}}
    <wps:RawDataOutput mimeType="{{mimeType}}">
      <ows:Identifier>{{identifier}}</ows:Identifier>
    </wps:RawDataOutput>
    {{/DataOutputs}}
    {{/RawDataOutput}}
    {{^RawDataOutput}}
    <wps:ResponseDocument{{#storeExecuteResponse}} storeExecuteResponse="{{storeExecuteResponse}}"{{/storeExecuteResponse}}{{#lineage}} lineage="{{lineage}}"{{/lineage}}{{#status}} status="{{status}}"{{/status}}>
    {{#DataOutputs}}
    {{#is_literal}}
      <wps:Output{{#dataType}} dataType="{{dataType}}"{{/dataType}}{{#uom}} uom="{{uom}}"{{/uom}}>
        <ows:Identifier>{{identifier}}</ows:Identifier>
      </wps:Output>
      {{/is_literal}}
      {{^is_literal}}
      <wps:Output{{#asReference}} asReference="{{asReference}}"{{/asReference}}{{#schema}} schema="{{schema}}"{{/schema}}{{#mimeType}} mimeType="{{mimeType}}"{{/mimeType}}{{#encoding}} encoding="{{encoding}}"{{/encoding}}>
        <ows:Identifier>{{identifier}}</ows:Identifier>
      </wps:Output>
      {{/is_literal}}
      {{/DataOutputs}}
    </wps:ResponseDocument>
    {{/RawDataOutput}}
    </wps:ResponseForm>	
    </wps:Execute>
