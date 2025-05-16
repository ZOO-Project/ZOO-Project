.. _contribute_hugo:

Contribute to the Website
===================================

This guide provides step-by-step instructions on how to run the ZOO-Project using the Hugo framework, including installation and implementation within the Hugo project structure.
`Hugo <https://gohugo.io>`_ is a fast and flexible static site generator written in Go, designed to build websites quickly and efficiently.

Clone the Repository
----------------------

Execute the following commands in the terminal:

.. code:: bash

   git clone https://github.com/ZOO-Project/website.git
   cd website

.. note::

   Changes made to the "Examples" page in the ZOO-Project/website repository  
   (`https://github.com/ZOO-Project/website <https://github.com/ZOO-Project/website>`_)  
   must also be reflected in the examples repository  
   (`https://github.com/ZOO-Project/examples <https://github.com/ZOO-Project/examples>`_)  
   to ensure consistency between both sources.


Installing Hugo
---------------

Install Hugo by following instructions here: https://gohugo.io/getting-started/installing/

To check if Hugo was installed successfully, run:

.. code:: bash

   hugo version


Running the Website Locally
---------------------------

.. code:: bash

   hugo server

Visit `http://localhost:8080/` to preview the site.


Creating a New Page
-------------------

1. Navigate to the `content` directory of the Hugo project.
2. Identify the appropriate section where the new page should be added.
3. Run the following command to generate a new Markdown file:

   .. code:: bash

      hugo new docs/new-page.md

4. Edit the file and include front matter like:

   .. code:: yaml

      ---
      title = "New Page Title"
      date = 2025-03-11
      description = "A brief description of the new page."
      draft = false
      ---

5. Preview with:

   .. code:: bash

      hugo server --buildDrafts


Adding Navigation Links
-----------------------

To include the new page in the navigation menu:

1. Edit the `config.toml` (or `hugo.toml`) file.
2. Add a reference under `[menu]`:

   .. code:: toml

      [[menu.main]]
      identifier = "new-page"
      name = "New Page"
      url = "/docs/new-page/"
      weight = 10


Steps to Use Custom Layouts
------------------------

1. Create a layout file (e.g., new-page.html) in themes/<your-theme>/layouts/
2. Edit the Markdown File:

   - In your .md file (e.g., new-page.md), specify the custom layout in the front matter

   .. code:: yaml

      ---
      title = "My Custom Page"
      layout = "new-page"
      ---

Add Custom CSS/JS
-----------------

1. Add files in themes/<your-theme>/static/
2. Reference them in:

   - `themes/<theme_name>/layouts/partials/head.html`
   - `themes/<theme_name>/layouts/_default/baseof.html`

3. Save the changes and test them with:

   .. code:: bash

      hugo --minify
      hugo server


