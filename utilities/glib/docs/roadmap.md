Roadmap
===

The roadmap for development of GLib in upcoming releases is tracked in GitLab,
using its [milestones feature](https://gitlab.gnome.org/GNOME/glib/-/milestones).
Look on the upcoming milestones to see what features and fixes are planned for
each release.

An issue being assigned to a milestone is no guarantee that it will actually be
fixed in time for that milestone. Milestones are a rough prioritisation system
for work, but GLib is a volunteer project with no fixed resources, so no
guarantees can be given.

All releases are time-based rather than feature-based, as the development and
stable branches of GLib should always be in a releasable state. Sometimes, at
the discretion of the maintainers, a release may be held for a week or so in
order to allow a particular merge request to land so that it can be made
available to distributions or testers more rapidly.

When [making a release](./releasing.md), all remaining issues and merge requests
allocated to the milestone for that release should be fixed (potentially
delaying the release), or rescheduled to a different release, based on the
maintainers’ assessment.

Unstable release planning
---

At the start of a development cycle, milestones are created for each release in
the cycle according to the [GNOME release
schedule](https://release.gnome.org/calendar/). GLib roughly follows the GNOME
release schedule, but makes its releases one or two weeks ahead of each
corresponding GNOME release. This allows other GNOME modules to depend on the
correct GLib version for new APIs. GLib does not follow the GNOME module
versioning scheme.

As the milestones are created, maintainers will assign issues to them, based on
what they think is possible to achieve for each milestone given the amount of
developer time available before the release.

Issues affecting a lot of users (such as common crashes), and new features which
maintainers think will have a wide benefit are prioritised.

As a development cycle progresses, some of the releases are timed to coincide
with [GNOME’s API/feature, string and hard code
freezes](https://handbook.gnome.org/release-planning/freezes.html). Issues which add API
and features are scheduled for the earlier micro releases in a development
cycle, followed by issues which add or change translatable strings, followed by
smaller bug fixes, documentation and unit test updates.

Stable release planning
---

Stable micro releases are scheduled at a cadence picked by maintainers,
depending on the rate at which bugs are being found in that stable branch. More
bugs leads to a more frequent release cadence.

Historically, the rate of releases on each stable branch has decreased inversely
proportionally to the time since the initial release of that branch.

There is no limit on the number of micro releases in a stable release series.
Typically there will be around 6. Micro releases stop once there are no more
bugs found in a stable series, or once a new stable series supersedes it.

The milestone for the next micro release in a stable series is created when the
previous micro release is made, such that only one stable micro release is
scheduled at any time.
