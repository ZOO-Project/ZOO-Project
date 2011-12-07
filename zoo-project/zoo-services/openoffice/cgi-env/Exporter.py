#
# Author : Gérald FENOY
#
# Copyright 2008-2009 GeoLabs SARL. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import uno
import getopt, sys

from unohelper import Base, systemPathToFileUrl, absolutize

from com.sun.star.beans import PropertyValue
from com.sun.star.script import CannotConvertException
from com.sun.star.lang import IllegalArgumentException
from com.sun.star.task import ErrorCodeIOException
from com.sun.star.io import IOException, XOutputStream

class OutputStream( Base, XOutputStream ):
    def __init__( self ):
        self.closed = 0
    def closeOutput(self):
        self.closed = 1
    def writeBytes( self, seq ):
        sys.stdout.write( seq.value )
    def flush( self ):
        pass

def OdtConverter(conf,inputs,outputs):
	# get the uno component context from the PyUNO runtime  
	localContext = uno.getComponentContext()

	# create the UnoUrlResolver 
	# on a single line
	resolver = 	localContext.ServiceManager.createInstanceWithContext	("com.sun.star.bridge.UnoUrlResolver", localContext )

	# connect to the running office                                 
	ctx = resolver.resolve( conf["oo"]["server"].replace("::","=")+";urp;StarOffice.ComponentContext" )
	smgr = ctx.ServiceManager

	# get the central desktop object
	desktop = smgr.createInstanceWithContext( "com.sun.star.frame.Desktop",ctx)

	# get the file name
	adressDoc=systemPathToFileUrl(conf["main"]["dataPath"]+"/"+inputs["InputDoc"]["value"])

	propFich=PropertyValue("Hidden", 0, True, 0),

	myDocument=0
	try:
	    myDocument = desktop.loadComponentFromURL(adressDoc,"_blank",0,propFich)
	except CannotConvertException, e:
	    print >> sys.stderr,  'Impossible de convertir le fichier pour les raisons suivantes : \n'
	    print >> sys.stderr,  e
	    sys.exit(0)
	except IllegalArgumentException, e:
	    print >> sys.stderr,  'Impossible de convertir le fichier pour les 	raisons suivantes : \n'
	    print >> sys.stderr,  e
	    sys.exit(0)

	outputDoc=systemPathToFileUrl(conf["main"]["tmpPath"]+"/"+inputs["OutputDoc"]["value"])

	tmp=inputs["OutputDoc"]["value"].split('.');

	outputFormat={"pdf": "writer_pdf_Export", "html": "HTML (StarWriter)","odt": "writer8","doc": "MS Word 97","rtf": "Rich Text Format"}

	for i in range(len(outputFormat)) :
	    if tmp[1]==outputFormat.keys()[i] :
	        filterName=outputFormat[tmp[1]]
	        prop1Fich = (
	            PropertyValue( "FilterName" , 0, filterName , 0 ),
		        PropertyValue( "Overwrite" , 0, True , 0 )
	        )
	        break

	myDocument.storeToURL(outputDoc,prop1Fich)
	myDocument.close(True)
	ctx.ServiceManager
	outputs["OutputedDocument"]={"value": inputs["OutputDoc"]["value"],"dataType": "string"}
	return 3
