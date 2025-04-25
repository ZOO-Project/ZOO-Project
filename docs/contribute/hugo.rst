.. _contribute_hugo:

Contribute to the Website
===================================

This guide provides step-by-step instructions for contributing to the ZOO-Project website using the Hugo static site generator.

Repository Location
-------------------

The source code for the website is hosted on GitHub:

`https://github.com/ZOO-Project/website <https://github.com/ZOO-Project/website>`_

If you're new to the project, this is where all the website content is managed. You will need to clone this repository and set it up locally before making any changes.

Cloning the Repository
----------------------

To get started, clone the official repository and navigate into the project folder:

.. code:: bash

   git clone https://github.com/ZOO-Project/website.git
   cd website

Installing Hugo
---------------

The website uses Hugo, a fast static site generator written in Go. You must install Hugo on your system.

Install Hugo by following instructions here: https://gohugo.io/getting-started/installing/

To check if Hugo was installed successfully, run:

.. code:: bash

   hugo version

Running the Website Locally
---------------------------

To preview the website locally before committing changes:

.. code:: bash

   hugo server

Then open your browser at `http://localhost:8080/` to see the live site.


Creating a New Page
-------------------

To create a new page in the documentation:

1. Navigate to the `content` directory of the Hugo project.
2. Identify the appropriate section where the new page should be added.
3. Run the following command to generate a new Markdown file:

   .. code:: bash

      hugo new docs/new-page.md

4. Open the newly created file in a text editor and add relevant content following Hugo's Markdown syntax.
5. Ensure the front matter (YAML, TOML, or JSON format) contains necessary metadata, e.g.:

   .. code:: yaml

      ---
      title: "New Page Title"
      date: 2025-03-11
      description: "A brief description of the new page."
      draft: false
      ---

6. Save the file and preview changes using:

   .. code:: bash

      hugo server --buildDrafts

Updating an Existing Page
-------------------------

To modify an existing page:

1. Locate the page inside the `content` directory.
2. Open the file and make necessary changes.
3. Save the file and verify the updates using:

   .. code:: bash

      hugo server

4. When you're satisfied, commit and push your changes:

   .. code:: bash

      git add content/docs/updated-page.md
      git commit -m "Updated documentation"
      git push origin main

Adding Navigation Links
-----------------------

To include the new page in the navigation menu:

1. Edit the `config.toml` (or `config.yaml`/`config.json`) file.
2. Add a reference under `[menu]`:

   .. code:: toml

      [[menu.main]]
      identifier = "new-page"
      name = "New Page"
      url = "/docs/new-page/"
      weight = 10

3. Save the file and test the navigation link locally using:

   .. code:: bash

      hugo server

