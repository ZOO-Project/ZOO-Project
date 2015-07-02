.. _service_translation:

Translation Support
===================

ZOO-Kernel support translating internal messages it emits but it can
also translate both the metadata informations stored in the ZCFG file
and the messages emitted by the ZOO-Service itself. This document show
how to create the files required to handle such a translation process
for the ZOO-Services.


ZCFG translation
--------------------------

First of all, use the following commands from your Services Provider
directory in order to extract all the messages to translate from the
ZCFG files :

  ::
  
      #!/bin/bash
      mkdir -p locale/{po,.cache}
      for j in cgi-env/*zcfg ; 
        do 
          for i in Title Abstract; 
           do
            grep $i $j | sed "s:$i = :_ss(\":g;s:$:\"):g" ;
           done;
       done > locale/.cache/my_service_string_to_translate.c
   

Then generate the ``messages.po`` file based on the Services Provider
source code (located in ``service.c`` in this example) using the
following command:

  ::
  
      #!/bin/bash
      xgettext service.c locale/.cache/my_service_string_to_translate.c -o message.po -p locale/po/ -k_ss

Once ``messages.po`` is created, use the following command to create
the ``.po`` file for the targeted language to translate into. We will
use the French language here as an example:

  ::
  
      #!/bin/bash
      cd locale/po/
      msginit -i messages.po -o zoo_fr_FR.po -l fr

Edit the ``zoo_fr_FR.po`` file with your favorite text editor or using
one of the following tools:

 * `poedit <http://www.poedit.net/>`__
 * `virtaal <http://translate.sourceforge.net/wiki/virtaal/index>`__
 * `transifex <https://www.transifex.net/>`__
 
Once the ``zoo_fr_FR.po`` file is completed, you can generate and
install the corresponding ``.mo`` file using the following command:

  ::
  
      #!/bin/bash
      msgfmt locale/po/zoo_fr_FR.po -o /usr/share/locale/fr/LC_MESSAGES/zoo-services.mo


In order to test the Services Provider ZCFG and internal messages
translation, please add the language argument to you request. As an
example, such a request:

http://youserver/cgi-bin/zoo_loader.cgi?request=GetCapabilities&service=WPS

would become the following:

http://youserver/cgi-bin/zoo_loader.cgi?request=GetCapabilities&service=WPS&language=fr-FR

The following command may also be useful in order to pull all the translations already available for a specific language.

  ::
  
      #!sh
      msgcat -o compilation.po $(find ../../ -name fr_FR.utf8.po)
      msgfmt compilation.po -o /usr/share/locale/fr/LC_MESSAGES/zoo-services.mo
