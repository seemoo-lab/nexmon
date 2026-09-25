Backports policy
===

Aims
---

 * Known bugs should be fixed in stable versions of GLib
 * New bugs must not be introduced into stable versions of GLib
 * Users and distributors should be able to rely on micro stable releases
   working as drop-in replacements for the previous releases on that stable
   branch, requiring no packaging changes, or recompilation or build system
   changes in dependent projects
 * Effort is only spent on [supported versions](../SECURITY.md#user-content-supported-versions)

Policy
---

 * Bug fixes and documentation fixes should be backported to the current stable
   branch of GLib, from the current unstable branch
 * Backports should only be done to
   [supported versions](../SECURITY.md#user-content-supported-versions) of GLib
 * New features must not be backported
 * Any change which will require packaging changes in a distribution should not
   be backported unless unavoidable to fix a widely-occurring bug
 * If a backported change does affect packaging or use of GLib, it must be
   listed prominently in the release notes for that stable release
 * Any change which requires changes or additions to translatable strings should
   not be backported unless unavoidable to fix a widely-occurring bug
   - If possible, existing translatable strings should be reused
   - If unavoidable, liaise with the GNOME Translation Team and ensure string
     changes are landed with plenty of time to allow translators to provide new
     translations
   - See https://handbook.gnome.org/release-planning/freezes.html#string-freeze
 * API or ABI changes (including API additions) must not be backported
   - A commit which changes the documented behaviour of a function counts as an
     API break
 * It is discretionary whether fixes to test cases, or new test cases, are
   backported, based on a maintainer’s assessment of the effort required to
   backport vs the value in running those tests on a stable branch
   - The risk of backporting changes to tests is that they fail or become flaky,
     and require further work on the stable branch to fix them
 * All backports must be submitted as a merge request against the stable branch,
   must pass through continuous integration, and must be reviewed by a
   maintainer (other than the person submitting the merge request)
   - The reviewer should first assess whether the backport is necessary, and
     then review it as with any other merge request
   - The submitter must set a stable release milestone on the merge request, so
     that the next stable release can’t be accidentally made before it’s merged
 * Typically, backports are trivial cherry-picks of commits from the unstable
   branch — changes to the unstable branch which are intended to be backported
   should be structured so that backporting is easier
   - For example, by splitting changes to be backported into a separate commit
     from those which should not be backported, or splitting out changes which
     are more likely to cause conflicts when cherry-picked
 * Backports should be done as soon as a fix lands on the unstable branch,
   rather than waiting until when the next stable release is due and then
   backporting multiple changes from the unstable branch at once. This gives
   more opportunity for backported changes to be tested, and reduces the chance
   of backporting the wrong thing, or missing a backport.
 * These rules are not entirely prescriptive: there may be situations where
   maintainers agree that a backport is necessary even if it breaks some of
   these rules, due to the balance of fixing a critical bug vs keeping things
   easy for distribution maintainers
