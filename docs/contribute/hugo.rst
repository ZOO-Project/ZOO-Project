.. _contribute_hugo:

Creating and Updating Pages in Hugo
===================================

This guide provides instructions for contributors on how to create and update pages using the Hugo framework.

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

4. Once satisfied with the updates, commit the changes:

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

3. Save the changes and test the menu update.
