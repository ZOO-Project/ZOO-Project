# -*- coding: utf-8 -*-
#
# ZOO Project documentation build configuration file

import re
import sphinx

extensions = ['sphinx.ext.autodoc', 'sphinx.ext.doctest', 'sphinx.ext.todo','sphinx.ext.autosummary', 'sphinx.ext.extlinks','sphinx.ext.viewcode']
master_doc = 'index'
templates_path = ['_templates']
exclude_patterns = ['_build']

# General information

project = 'ZOO-Project'
copyright = '2009-2015, ZOO-Project team'
version = '1.0'
release = '1.5'
show_authors = True

# Options for HTML output

html_theme = 'sphinx_rtd_theme'
html_theme_path = ['_themes']
modindex_common_prefix = ['sphinx.']
html_static_path = ['_static']
html_sidebars = {'index': ['indexsidebar.html', 'searchbox.html']}
#html_additional_pages = {'index': 'index.html'}
html_use_opensearch = 'http://sphinx-doc.org'
htmlhelp_basename = 'Sphinxdoc'

# Options for epub output

epub_theme = 'epub'
epub_basename = 'ZOO-Project'
epub_author = 'ZOO-Project team'
epub_publisher = 'http://zoo-project.org/'
epub_scheme = 'url'
epub_identifier = 'http://zoo-project.org'
epub_pre_files = [('index.html', 'ZOO-Project documentation')]
#epub_post_files = [('install.html', 'Installing Sphinx'),('develop.html', 'Sphinx development')]
epub_exclude_files = ['_static/opensearch.xml', '_static/doctools.js','_static/jquery.js', '_static/searchtools.js','_static/underscore.js', '_static/basic.css','search.html', '_static/websupport.js']
epub_fix_images = False
epub_max_image_width = 0
epub_show_urls = 'inline'
epub_use_index = False
epub_guide = (('toc', 'index.html', u'Table of Contents'),)

# Options for LaTeX output

latex_documents = [('index', 'ZOO-Project.tex', 'ZOO-Project Documentation','ZOO-Project team', 'manual', 1)]
latex_logo = '_static/zoo-simple.png'
latex_elements = {'fontpkg': '\\usepackage{palatino}'}
latex_show_urls = 'footnote'
todo_include_todos = True
man_pages = [('index', 'zooproject', 'ZOO-Project Documentation','ZOO-Project team', 1)]

# -- Extension interface -------------------------------------------------------

from sphinx import addnodes  # noqa

event_sig_re = re.compile(r'([a-zA-Z-]+)\s*\((.*)\)')


def parse_event(env, sig, signode):
    m = event_sig_re.match(sig)
    if not m:
        signode += addnodes.desc_name(sig, sig)
        return sig
    name, args = m.groups()
    signode += addnodes.desc_name(name, name)
    plist = addnodes.desc_parameterlist()
    for arg in args.split(','):
        arg = arg.strip()
        plist += addnodes.desc_parameter(arg, arg)
    signode += plist
    return name


def setup(app):
    from sphinx.ext.autodoc import cut_lines
    from sphinx.util.docfields import GroupedField
    app.connect('autodoc-process-docstring', cut_lines(4, what=['module']))
    app.add_object_type('confval', 'confval',
                        objname='configuration value',
                        indextemplate='pair: %s; configuration value')
    fdesc = GroupedField('parameter', label='Parameters',
                         names=['param'], can_collapse=True)
    app.add_object_type('event', 'event', 'pair: %s; event', parse_event,
                        doc_field_types=[fdesc])
