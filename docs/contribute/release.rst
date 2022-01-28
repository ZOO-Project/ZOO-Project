.. _contribute_release:

Release Procedure
=================

The ZOO-Project release procedure is commonly defined by the following
rules:

* Any of the :ref:`zoo_developers` can ask for a release by asking the
  :ref:`zoo_psc` and pointing a release manager. This last will then
  vote for accepting both the manager and the release procedure to
  happen.
* If not already created, create  a wiki page (like this `one
  <https://github.com/ZOO-Project/ZOO-Project/wiki/Release:-1.8.0:-Notes>`_ using this
  scheme: Release:-M.m.r:-Notes), summarizing changes from the previous
  release (extracted from the `revision log
  <https://github.com/ZOO-Project/ZOO-Project/blob/main/zoo-project/HISTORY.txt>`_).
* That file should include new features, changed features, and
  deprecated features if any. Changes to the official documentation
  should be specifically noted along with other items that will cause
  breaking changes during upgrades. 
* Read the documentation and remove outdated parts.
* Create release candidate as .zip and .tar.bz2  then add them on this
  `page <http://zoo-project.org/new/Code/Download>`_ (by editing this
  `wiki page <http://zoo-project.org/trac/wiki/ZooWebSite/2015/Code/Download>`_)
* Cut a release candidate once you think that everything is in
  order. Announce the release candidate for review for at least 1
  week. In this period of time, it is also appropriate for you to
  deploy in production since you are asserting that it is stable and
  (significant) bug free. Publish a specific revision with this.
* If significant bugs are reported, fix and cut a new release
  candidate. If no major bugs, then announce that the release
  candidate has officially been promoted to the official release (if
  you want, you can do this with a motion and support of the PSC).
* Update documentation as needed.  
* Ensure that release exactly matches something in git history. Tag and branch
  appropriately.
* Once the tag is pushed back to GitHub, make sure to use the 
  "Create release from tag" button on the tag page (accessible from the 
  `Tags list <https://github.com/ZOO-Project/ZOO-Project/tags>`__).
  Set the release title to "ZOO-Project X.Y.Z" and you may include the 
  release note as the release description. Before publishing the
  release, make sure to upload a copy of the archives stored on 
  zoo-project.org/dl.
* Announce on various email list and other locations
  (news_item@osgeo.org, SlashGeo, etc)


Creating an Official Release
----------------------------

Release versions lead to an update in documentation and standard tarballs. This is to help future administrators repeatably create releases.

* Double check that the pages from `the ZOO-Project.org web site <http://zoo-project.org/>`_ match the current version.
* Double check that the latest build file matches the current revisions number.
* If this is a new major release create a branch and a tag.

.. code::

    git checkout main
    git pull origin main
    git checkout -b branch-1.9
    git push origin branch-1.9
    git checkout branch-1.9
    git pull origin branch-1.9
    git tag -a -m "Create tag rel-1.9.0-rc1 " rel-1.9.0-rc1

* If this is a major or minor relase, create a tag.

.. code::

    git checkout branch-1.9
    git pull origin branch-1.9
    git tag -a -m "Create tag rel-1.9.1" rel-1.9.1

* Commit the tags or branches with the version numbers.

.. code::

    git push origin rel-1.9.1

* Create version archives

.. code::

    export VERSION=2.6.0
    git clone -b branch-1.6 --single-branch git@github.com:ZOO-Project/ZOO-Project.git zoo-src
    cp -r zoo-src zoo-project-$VERSION
    cd zoo-project-$VERSION
    rm -rf $(find ./ -name ".git") 
    cd zoo-project/zoo-kernel
    autoconf
    # In case you did not build ZOO-Kernel
    cd ../../..
    # In case you built ZOO-Kernel, then remove the generated file from the archive
    make clean
    rm -f  {Makefile,ZOOMakefile.opts}
    cd ../../..
    # In case you built one or more ZOO-Services, then remove the generated file from the archive
    rm $(find ./zoo-project-$VERSION/zoo-project/zoo-services -name "*zo")
    # Remove documentation from the archive
    rm -rf ./zoo-project-$VERSION/docs
    tar -cvjf ./zoo-project-$VERSION.tar.bz2 ./zoo-project-$VERSION
    zip -r ./zoo-project-$VERSION.zip ./zoo-project-$VERSION
    scp -P 1046 ./zoo-project-$VERSION.{zip,tar.bz2} zoo-project.org:/var/www/localhost/htdocs/dl/

* Update the `Downloads page <http://zoo-project.org/new/Code/Download>`_ to add the latest release (by editing `this wiki page <http://zoo-project.org/trac/wiki/ZooWebSite/2015/Code/Download>`_).
