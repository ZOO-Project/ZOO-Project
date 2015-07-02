.. _contribute_doc:

.. include:: <xhtml1-lat1.txt>
.. include:: <xhtml1-symbol.txt>

Contribute documentation
========================

ZOO Documentation is a collaborative process managed by the ZOO developers. Anybody is welcome to contribute to the ZOO-Project documentation. Please consider the following instructions before doing so.

General information
------------------------

Heading syntaxe
...............

Tere are various title heading used in the documentation, when you
create a new document, you're invited to follow the following heading
underline syntaxe:

 * for Heading 1, use ``=``,
 *  for Heading 2, use ``-``,
 *  for Heading 3, use ``.``,
 *  for Heading 4, use ``*``,
 *  for Heading 5, use ``#``.
 

For new comers
..........................

New users are encouraged to contribute documentation using the following ways:

* Download the ZOO-Project svn, edit the documentation files
  located /docs directory and share the modifications through a new
  ticket set to 'Documentation' tracker
  
* Create a wiki page containg new or corrected documentation text, and create a new ticket to report its creation. 

The ZOO developers responsible for the documentation will then review the contributions to add them into the official docs.

For registered developers
........................................

The current structure of the ZOO Project documentation process is for
developers with :ref:`SVN <svn>` commit access to maintain their
documents in reStructuredText format, and therefore all documents live
in the /docs directory in SVN.  The `Sphinx
<http://sphinx.pocoo.org/>`__ documentation generator is used to
convert the reStructuredText files to html, and the live website is
then updated on an hourly basis.


Installing and using Sphinx
---------------------------

On Linux
.................

* Make sure you have the Python dev and setuptools packages installed. For example on Ubuntu:

   ::

       sudo apt-get install python-dev
       sudo apt-get install python-setuptools

* Install sphinx using easy_install:

   ::

       sudo easy_install Sphinx==1.3.1
       
   .. note::
    
      Make sure you install Sphinx 1.3.1 or more recent. 
      
* Checkout the /docs directory from SVN, such as:

   ::
  
       svn checkout http://svn.zoo-project.org/svn/trunk zoo-project      

* To process the docs, from the ZOO /docs directory, run:

   ::

       make html

   or

   ::

       make latex

   The HTML output will be written to the build/html sub-directory.

.. note::

    If there are more than one translation, the above commands will automatically
    build all translations.


On Mac OS X |trade|
..................................

* Install sphinx using easy_install:

   ::

       sudo easy_install-2.7 Sphinx==1.3.1
       
   .. note::
   
      Make sure you install Sphinx 1.3.1 or more recent. 

* Install `MacTex <http://www.tug.org/mactex/2009/>`__ if you want to build pdfs

* Checkout the /docs directory from SVN, such as:

   ::
  
       svn checkout http://svn.zoo-project.org/svn/trunk zoo-project

* To process the docs, from the ZOO /docs directory, run:

   ::

       make html

   or

   ::

       make latex

   The HTML output will be written to the build/html sub-directory.


On Windows |trade|
................................

* Install `Python 2.X <http://www.python.org/>`__
* Download `setuptools <http://pypi.python.org/pypi/setuptools#windows>`__
* Make sure that the ``C:/Python2X/Scripts`` directory is your path
* Execute the following at commandline:

   ::

       easy_install Sphinx==1.3.1

   ...you should see message: "Finished processing dependencies for Sphinx"

   .. note::
   
      Make sure you install Sphinx 1.3.1 or more recent.  See note above.
      
* Install `MiKTeX <http://miktex.org>`__ if you want to build pdfs
      
* Checkout the /docs directory from SVN, such as:

   ::
  
       svn checkout http://svn.zoo-project.org/svn/trunk zoo-project
      
* Inside the /docs directory, execute:

   ::

       make html

   or

   ::

       make latex

   The HTML output will be written to the _build/html sub-directory.


reStructuredText Reference Guides
***************************************

The following resources are considered as useful for editing and creating new ZOO-Project documentation files.

- Docutils `Quick reStructuredText <http://docutils.sourceforge.net/docs/user/rst/quickref.html>`__
- Docutils `reStructuredText Directives <http://docutils.sourceforge.net/docs/ref/rst/directives.html>`__
- Sphinx's `reStructuredText Primer <http://sphinx.pocoo.org/rest.html>`__
- search Sphinx's `mailing list <http://groups.google.com/group/sphinx-dev>`__
