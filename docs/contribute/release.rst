   .. _contribute_release:

Release procedure
================================

General rules
------------------

The ZOO-Project release procedure is commonly defined by the following rules:

   * Any ZOO-Project commiter can ask for a release by asking the ZOO-Project PSC and pointing a release manager. This last will then vote for accepting both the manager and the release procedure to happen.
   * It is first needed to create a wiki page summarizing the changes from the previous release (extracted from the revision log file called
   `HISTORY <http://zoo-project.org/trac/browser/trunk/zoo-project/HISTORY.txt>`__ ).
   * That wiki page should include new features, changed features, and
     deprecated features if any. Changes to the official documentation
     should be specifically noted along with other items that will
     cause breaking changes during upgrades.
   * Read the documentation and remove outdated parts.
   * Compress the release candidate source code in .zip and .tar.bz2
     formats, and then add them to the `Download
     <http://zoo-project.org/Download>`__ section of the ZOO-Project
     website.
   * Cut a release candidate once you think that everything is in
     order. Announce the release candidate for review for a duration
     of at least 1 week. During this time, it is also appropriate to
     deploy the release candidate in production since you are
     asserting that it is stable and (significant) bug free. Publish a
     specific revision with this.
   * If significant bugs are reported, fix and cut a new release
     candidate. If no major bugs, then announce that the release
     candidate has officially been promoted to the official release
     (This can optionally done with a motion and support of the
     PSC).
   * Ensure that release exactly matches something in SVN. Tag and branch appropriately.
   * Update documentation as needed.
   * Announce the release on the zoo-discuss list and various media

Creating an official release
---------------------

Release versions lead to an update in documentation and standard tarballs. This is to help future administrators repeatably create releases.

  * Double check that the pages from  the ZOO-Project.org web site match the current version.
  * Double check that the latest build file matches the current revisions number.
  * If this is a new major release create a branch and a tag, as shown
    bellow:
    
    ::
 
      cd zoo-project-svn/
      svn cp trunk branches/branch-1.6
      svn cp trunk tags/rel-1.6.0

  * If this is a major or minor relase, create a tag, as follow:

    ::
       
      svn cp branches/branch-1.6 tags/rel-1.6.1
      
  * Commit the tags or branches with the version numbers.
    
     ::
       
       svn commit -m 'Created branch/tags for the X.Y.Z release'

   * Create version archives
     
     ::
       
       export VERSION=2.6.0
       cd zoo-propject-svn
       cp -r trunk zoo-project-$VERSION
       cd zoo-project-$VERSION
       rm -rf $(find ./ -name ".svn") 
       cd zoo-project/zoo-kernel
       autoconf
       cd ../../..
       # Remove documentation from the archive
       rm -rf ./zoo-project-$VERSION/docs
       tar -cvjf ./zoo-project-$VERSION.tar.bz2 ./zoo-project-$VERSION
       zip -r ./zoo-project-$VERSION.zip ./zoo-project-$VERSION
       scp -P 1046 ./zoo-project-$VERSION.{zip,tar.bz2} zoo-project.org:/var/www/localhost/htdocs/dl/

   * Update the `Download <http://zoo-project.org/Download>`__ section to add and link to the latest release.
