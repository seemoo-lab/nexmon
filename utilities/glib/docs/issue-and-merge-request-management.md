Issue and merge request management policy
===

Aims
---

 * Finding duplicates and related issues and merge requests should be easy when
   triaging new ones, or when looking back at the history of a particular piece
   of code
 * GLib co-maintainers and other interested people should be able to subscribe
   to notifications for specific parts of GLib without receiving notifications
   for all activity on GLib
 * Issues and merge requests which are planned to be in a specific release
   should not be accidentally left out of that release
 * GLib co-maintainers should be able to easily see how much work is left to do
   before a release is ready
 * Users and developers should have some idea of whether an issue or merge
   request is being actively worked on, the timescale for its completion, and
   who’s working on it

Issue triage
---

 * Issues should be triaged shortly after they are filed. Triage should:
   - Add labels to categorise the issue, even if it’s about to be closed (this
     helps with finding related closed issues in future)
   - Close them immediately if they are a duplicate of an existing issue
     (`/duplicate #issue`)
   - Or if they are out of scope, such as a user support question (which should
     be on https://discourse.gnome.org)
   - Ask the user for necessary debug information if the issue is valid, a bug,
     and not enough information has been provided — extracting debug information
     from users is often time-critical because they can only reproduce an issue
     under certain conditions, or they lose interest and move on
   - Assign the issue to an upcoming milestone if it seems urgent
 * Note that triaging an issue does *not* commit the triager to working on the
   issue
 * If an issue is likely to affect a stable release (as well as the unstable
   `main` branch), assign it to the next micro release milestone for that stable
   release — the merge request for the fix will be assigned to the next `main`
   micro release, and its backport to the next stable micro release

Merge request triage
---

 * Merge requests should be triaged shortly after they are filed. Triage should
   proceed as for issues, including labels and milestones
 * The milestone for a merge request is the release it is intended to be
   included in, and it should match the target branch of the merge request
   - It’s important to add milestones to merge requests, as they then show up on
     the milestone page and highlight that the release is not yet ready until
     they’re all merged
   - This prevents releases accidentally being made without containing all the
     fixes they’re supposed to
 * The ‘assignee’ of the merge request is the person who is working on it,
   responding to review feedback
 * The ‘reviewer’ of the merge request is the GLib co-maintainer who is actively
   reviewing it
 * Don’t assign someone else as the reviewer or assignee of a merge request
   unless they have said they are willing to do it, otherwise it gives a false
   impression that their time is allocated for doing the work
   - You may assign someone else as a reviewer or assignee when closing or
     merging a merge request, though, if that helps document who has done the
     work so they can be appropriately attributed

Labelling issues and merge requests
---

GLib has a huge number of labels available, one per component of the library
plus several orthogonal labels. The use of labels allows for:
 * Easy searching for related issues and duplicates, by filtering on label
 * Co-maintainers of GLib to subscribe to issue and merge request notifications
   for a set of labels limited to their interests, meaning they don’t have to
   subscribe to the full fire hydrant of GLib notifications just to maintain one
   or two components
 * High-level prioritization of work, such as prioritizing crashes over new
   features
 * Tracking issues and merge requests through the release lifecycle, so that
   (for example) API additions can be done before the API freeze, and merge
   requests approved during a code freeze can all be landed when the freeze ends

To subscribe to a specific label, go to
[the labels page](https://gitlab.gnome.org/GNOME/glib/-/labels) and use the
subscription selector next to the labels you’re interested in.

Several labels are worth highlighting:
 * Security: Time-critical security issues, which should typically be marked
   as confidential.
 * Merge After Freeze: Merge requests which have been accepted, but which can’t
   be merged yet as GLib is in
   [code freeze](https://handbook.gnome.org/release-planning/freezes.html). All MRs tagged
   like this will be merged en-masse when the freeze ends.
 * Needs Information: Issues which are blocked due to needing more information
   from the reporter. These can be closed after 4 weeks if the reporter does not
   respond.
 * Not GNOME / Out of Scope: Issues which were closed due to not being within
   the scope of GLib.
 * Newcomers: Issues which are suitable for being taken on by newcomers to GLib.
   When labelling an issue as such, please make sure that the issue title is
   clear, and the description (or a comment) clearly explains what needs to be
   done to fix the issue, to give newcomers the best chance of successfully
   submitting a fix.
 * Translation / I18N: Issues which relate to translatable strings or other
   internationalization or localization problems. Adding this label may cause
   the translation team to be looped into an issue or merge request.
 * API deprecation: Issues or merge requests which deprecate GLib API, and hence
   can only land in an unstable release outside an
   [API freeze](https://handbook.gnome.org/release-planning/freezes.html#api-abi-freeze).
 * New API: Issues or merge requests which add new GLib API, and hence can only
   land in an unstable release outside an
   [API freeze](https://handbook.gnome.org/release-planning/freezes.html#api-abi-freeze).
 * Intermittent: Issues (such as test failures) which can only be reproduced
   intermittently.
 * Test failure: Issues which revolve around a unit test failing. Typically
   these are opened after a CI run has failed, and are useful for tracking how
   often a particular failure happens.
 * Test missing: Merge requests which need a unit test to be written before they
   can be merged.

Merge request review
---

 * Assign yourself as the ‘reviewer’ of a merge request when you start review;
   this helps with tracking notifications, and lets the assignee of the merge
   request know whether you are providing a comprehensive review or just some
   drive-by comments
 * Review the merge request at a high level first: is the change needed, does
   it make sense, is it structured correctly; then look at the detail of memory
   management, typos, etc.
  - If a merge request is large or contains multiple unrelated changes, it is
    best to ask the author to split it into multiple parallel merge requests.
    This prevents review comments on one part of the merge request from blocking
    merging the rest, and allows the reviewer’s time to be split into smaller
    and more manageable chunks.
 * Submit all your comments as a single review (rather than adding multiple
   single comments) to avoid spamming subscribers with multiple notifications.
 * CC in additional reviewers if their second opinion or domain expertise are
   needed.
 * Follow the review through multiple cycles of updates and re-review with an
   aim to getting the merge request landed — a merge request which gets one
   round of review and which is then forgotten about is as useful as a merge
   request which gets no review at all.
 * While it is useful to highlight related areas of the code needing work that
   you spot while doing a review, it is not the responsibility of the author of
   a merge request to fix things outside the scope of their merge request.
   Reviews which increase the scope of work for a merge request make it much
   less likely that the merge request will land quickly, which reduces the
   effective usefulness of the contribution. This wears down contributors and
   reviewers, as they don’t see closure on what they’ve put time into. It is
   better to file additional issues for follow-up or related work.
 * If you cannot follow through a merge request to completion, unassign yourself
   as the reviewer to make this clear to everybody.
 * Once a merge request lands, a backport might need to be created for the most
   recent stable GLib branch (see the [backport policy](./backports.md). It is
   the responsibility of the maintainers to do this, not the responsibility of
   the merge request author.

