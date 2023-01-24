.. _contribute_dev:

Committer guidelines
====================

This section gathers information to the registered ZOO-Project developers.

.. note::
    Since March 2021, ZOO-Project's source code is under GitHub control
    (https://github.com/ZOO-Project).  In the past, ZOO-Project has also used
    Subversion for version control.

Election to Git Commit Access
-----------------------------

Permission for git commit access shall be provided to new developers only if accepted by the :ref:`zoo_psc`. A proposal should be written to the PSC for new committers and voted.

Removal of git commit access should be handled by the same process.

The new committer should have demonstrated commitment to ZOO-Project and knowledge of the ZOO-Project source code and processes to the committee's satisfaction, usually by reporting bugs, submitting patches, and/or actively participating in the ZOO-Project mailing list(s).

The new committer should also be prepared to support any new feature or changes that he/she commits to the ZOO-Project source tree in future releases, or to find someone to which to delegate responsibility for them if he/she stops being available to support the portions of code that he/she is responsible for.

All committers should also be a member of the zoo-discuss mailing list so they can stay informed on policies, technical developments and release preparation.

New commiters are responsible for having read, and understood this document.

Committer Tracking
------------------

A list of all project committers will be kept in the main zoo-project directory (called `COMMITTERS <https://github.com/ZOO-Project/ZOO-Project/blob/main/zoo-project/COMMITTERS>`__) listing for each committer:

    * Userid: the id that will appear in the git logs for this person.
    * Full name: the users actual name.
    * Email address: A current email address at which the committer can be reached. It may be altered in normal ways to make it harder to auto-harvest.



Git Administrators
------------------

One member of the Project Steering Committee will be designed the Git Administrator. That person will be responsible for giving git commit access to folks, updating the COMMITTERS file, and other git related management. That person will need a user account to access GitHub of course.


Git Commit Practices
--------------------

The following are considered good git commit practices for the ZOO-Project project.

   * Use meaningful descriptions for SVN commit log entries.
   * Add an issue reference like "(#1234)" at the end of git commit log entries when committing changes related to an issue. The '#' character enables GitHub to create a hyperlink from the changeset to the mentionned ticket.
   * After commiting changes related to an issue, write the tree and revision in which it was fixed in the issue description. Such as "Fixed in master (ZOO-Project@o0o00o0o) and in branches/1.7 (ZOO-Project@o0o00o0o)". The 'ZOO-Project@' prefix enables GitHub to create a hyperlink from the issue to the corresponding commit (using the 8 first letters of the commit sha hash, 'o0o00o0o' in the example).
   * Changes should not be committed in stable branches without a corresponding bug id. Any change worth pushing into the stable version is worth a bug entry.
   * Never commit new features to a stable branch without permission of the PSC or release manager. Normally only fixes should go into stable branches.
   * New features go in the main development main branch.
   * Only bug fixes should be committed to the code during pre-release code freeze, without permission from the PSC or release manager. 
   * Significant changes to the main development version should be discussed on the zoo-discuss list before you make them, and larger changes will require to be discussed and approved on zoo-psc list by the PSC.
   * Do not create new branches without the approval of the PSC. Release managers are assumed to have permission to create a branch.
   * All source code in git should be in Unix text format as opposed to DOS text mode.
   * When committing new features or significant changes to existing source code, the committer should take reasonable measures to insure that the source code continues to build and work on the most commonly supported platforms (currently Linux and Windows), either by testing on those platforms directly, running Buildbot tests, or by getting help from other developers working on those platforms. If new files or library dependencies are added, then the configure.in, Makefile.in, Makefile.vc and related documentations should be kept up to date. 

Legal
-----

Committers are the front line gatekeepers to keep the code base clear of improperly contributed code. It is important to the ZOO-Project users, developers and the OSGeo foundation to avoid contributing any code to the project without it being clearly licensed under the project license.

Generally speaking the key issues are that those providing code to be included in the repository understand that the code will be released under the MIT/X license, and that the person providing the code has the right to contribute the code. For the commiter themselves understanding about the license is hopefully clear. For other contributors, the commiter should verify the understanding unless the commiter is very comfortable that the contributor understands the license (for instance frequent contributors).

If the contribution was developed on behalf of an employer (on work time, as part of a work project, etc) then it is important that an appropriate representative of the employer understand that the code will be contributed under the MIT/X license. The arrangement should be cleared with an authorized supervisor/manager, etc.

The code should be developed by the contributor, or the code should be from a source which can be rightfully contributed such as from the public domain, or from an open source project under a compatible license.

All unusual situations need to be discussed and/or documented.

Committers should adhere to the following guidelines, and may be personally legally liable for improperly contributing code to the source repository:

   * Make sure the contributor (and possibly employer) is aware of the contribution terms.
   * Code coming from a source other than the contributor (such as adapted from another project) should be clearly marked as to the original source, copyright holders, license terms and so forth. This information can be in the file headers, but should also be added to the project licensing file if not exactly matching normal project licensing (zoo-project/zoo-kernel/LICENSE).
   * Existing copyright headers and license text should never be stripped from a file. If a copyright holder wishes to give up copyright they must do so in writing to the foundation before copyright messages are removed. If license terms are changed it has to be by agreement (written in email is ok) of the copyright holders.
   * Code with licenses requiring credit, or disclosure to users should be added to /trunk/zoo-project/zoo-kernel/LICENSE.
   * When substantial contributions are added to a file (such as substantial patches) the author/contributor should be added to the list of copyright holders for the file.
   * If there is uncertainty about whether a change it proper to contribute to the code base, please seek more information from the project steering committee, or the foundation legal counsel. 


