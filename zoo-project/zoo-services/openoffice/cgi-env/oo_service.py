import uno
import getopt, sys, os

from unohelper import Base, systemPathToFileUrl, absolutize

from com.sun.star.beans import PropertyValue
from com.sun.star.script import CannotConvertException
from com.sun.star.lang import IllegalArgumentException
from com.sun.star.task import ErrorCodeIOException
from com.sun.star.io import IOException, XOutputStream
from com.sun.star.style.BreakType import PAGE_BEFORE, PAGE_AFTER
from com.sun.star.text.ControlCharacter import PARAGRAPH_BREAK

from xml.dom import minidom 
import sys 

keep_trace=''

def addToText(cursor,text,level,value):
    if level==1:
        cursor.NumberingStyleName = "NONE"
        cursor.ParaStyleName="Heading 1"
        text.insertString( cursor, value , 0 )
        text.insertControlCharacter( cursor, PARAGRAPH_BREAK , 0 )
        #print  >> sys.stderr,' * Main Title : ' + value
    else:
        i=0
        prefix=''
        while i < level-1:
            prefix+=' '
            i+=1
        cursor.NumberingStyleName="List "+str(level-1)
        text.insertString( cursor, prefix+value , 0 )
        text.insertControlCharacter( cursor, PARAGRAPH_BREAK , 0 )
        cursor.NumberingStyleName = "NONE"
	#print  >> sys.stderr,dir(sys.stderr)
        #print >> sys.stderr,prefix+' * NumberingStyleName '+str(level-1)+' '+value.encode('iso-8859-15')

def printChildren(cursor,text,node,level,keep_trace):
    if node.nodeType==3:
        level-=1

    if not(node.nodeValue!=None and len(node.nodeValue.replace(' ',''))!=1 and keep_trace!='' and keep_trace!=None):
        if keep_trace!='':
            addToText(cursor,text,level-1,keep_trace)
        keep_trace=node.nodeName

    if node.hasChildNodes():
        for i in node.childNodes:
            printChildren(cursor,text,i,level+1,keep_trace)
            keep_trace=''
    else:
        if node.nodeValue != None and len(node.nodeValue.replace(' ',''))>1:
            addToText(cursor,text,level-1,keep_trace+' : '+node.nodeValue)
            keep_trace=''
        else:
            if node.nodeValue != None and len(node.nodeValue.replace(' ',''))>1:
                addToText(cursor,text,level-1,keep_trace+' : '+node.nodeValue)
                keep_trace=''
            else:
                if keep_trace!='#text':
                    addToText(cursor,text,level-1,keep_trace)
                    keep_trace=''

    if node.nodeType==1 and node.hasAttributes():
        i=0
        while i<node.attributes.length:
            addToText(cursor,text,level,'(attr) '+node.attributes.keys()[i] + ' : ' + node.attributes[node.attributes.keys()[i]].value)
            i+=1

        

def Xml2Pdf(conf,input,output):
    localContext = uno.getComponentContext()
    resolver = localContext.ServiceManager.createInstanceWithContext("com.sun.star.bridge.UnoUrlResolver", localContext )
    ctx = resolver.resolve( "uno:socket,host=127.0.0.1,port=3662;urp;StarOffice.ComponentContext" )
    smgr = ctx.ServiceManager
    desktop = smgr.createInstanceWithContext( "com.sun.star.frame.Desktop",ctx)
    adressDoc=systemPathToFileUrl(input["doc"]["value"])
    propFich=PropertyValue("Hidden", 0, True, 0),
    try:
        myDocument = desktop.loadComponentFromURL(adressDoc,"_blank",0,propFich)
	#Prefer to create a new document without any style ?
        #myDocument = desktop.loadComponentFromURL("private:factory/writer","_blank",0,propFich)
    except:
        conf["lenv"]["message"]='Unable to load input document'
	return 4
    text = myDocument.Text
    cursor = text.createTextCursor()
    cursor.gotoStart(0)
    cursor.gotoEnd(1)
    xmldoc = minidom.parseString(input['xml']['value'])

    if xmldoc.hasChildNodes():
        for i in xmldoc.childNodes:
            if i.nodeType==1:
                cursor.ParaStyleName="Title"
                text.insertString( cursor, i.nodeName , 0 )
                text.insertControlCharacter( cursor, PARAGRAPH_BREAK , 0 )
                #print >> sys.stderr,' * 1st level' + i.nodeName
                if i.hasChildNodes():
                    for j in i.childNodes:
                        printChildren(cursor,text,j,2,'')

    tmp=myDocument.StyleFamilies.getByName("NumberingStyles")

    tmp1=tmp.getByName("Puce 1")

    prop1Fich = ( PropertyValue( "FilterName" , 0, "writer_pdf_Export", 0 ),PropertyValue( "Overwrite" , 0, True , 0 ) )
    outputDoc=systemPathToFileUrl("/tmp/output.pdf")
    myDocument.storeToURL(outputDoc,prop1Fich)

    myDocument.close(True)
    ctx.ServiceManager
    output["Document"]["value"]= open('/tmp/output.pdf', 'r').read()
    print >> sys.stderr,len(output["Document"]["value"])
    return 3

#To run test from command line uncomment the following line:
#xml2pdf({},{"file":{"value":"/tmp/demo.xml"},"doc":{"value":"/tmp/demo.odt"}},{})
