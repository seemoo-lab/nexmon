Making a release
===

When to make a release
---

Releases are made on a schedule determined in the [roadmap](./roadmap.md). Each
release corresponds to a [GitLab milestone](https://gitlab.gnome.org/GNOME/glib/-/milestones).

There is usually some scope to change a release date by plus or minus a week, to
allow specific merge requests to land if they are deemed as more important to
release sooner rather than waiting until the next scheduled release. However,
there is always another release, and releasing on time is more important than
releasing with everything landed. Releasing on time allows distributors to
schedule their packaging work efficiently.

Maintainers should take it in turns to make releases so that the load is spread
out evenly and every maintainer is practiced in the process.

How to make a release
---

Broadly, GLib follows the same process as [every other GNOME
module](https://handbook.gnome.org/maintainers/making-a-release.html).

These instructions use the following variables:
 - `new_version`: the version number of the release you are making, for example `2.73.1`
 - `previous_version`: the version number of the most-recently released version in the same release series, for example `2.73.0`
 - `branch`: the branch which the release is based on, for example `glib-2-72` or `main`

Make sure your repository is up to date and doesn’t contain local changes:
```sh
git pull
git status
```

Check the version in `meson.build` is correct for this release.

Download
[gitlab-changelog](https://gitlab.gnome.org/pwithnall/gitlab-changelog) and use
it to write a `NEWS` entry:
```sh
gitlab-changelog.py GNOME/glib ${previous_version}..
```

Copy this into `NEWS`, and manually write some highlights of the fixed bugs as
bullet points at the top. Most changes won’t need to be highlighted — only the
ones which add APIs, change dependencies or packaging requirements, or fix
impactful bugs which might affect distros’ decisions of how to prioritise the
GLib release or how urgent to mark it as.

You can get review of your `NEWS` changes from other co-maintainers if you wish.

Commit the release:
```sh
git add -p
git commit -s -m "${new_version}" --gpg-sign
```

Test build the release tarball:
```sh
meson dist -C build/
```

Tag, sign and push the release, using `${new_version}` as the tag message (see below for information about `git evtag`):
```sh
git evtag sign ${new_version}
git push --atomic origin ${branch} ${new_version}
```
You will need to **temporarily** allow maintainers to push to the `main` branch on https://gitlab.gnome.org/GNOME/glib/-/settings/repository. To use a specific key add an option `-u ${keyid|email}` after the `sign` argument.

The `dist-job` CI job will be triggered and will build and upload the final release archive.

Add the release notes to GitLab and close the milestone:
 - Go to https://gitlab.gnome.org/GNOME/glib/-/releases/${new_version}/edit
   and upload the release notes for the new release from the `NEWS` file
 - Then link the milestone to it, then list the new release tarball and
   `sha256sum` file in the ‘Release Assets’ section as the ‘Other’ types.
   Get the file links from https://download.gnome.org/sources/glib/ and
   name them ‘Release archive’ and ‘Release archive checksum’
 - Go to https://gitlab.gnome.org/GNOME/glib/-/milestones/
   choose the milestone and close it, as all issues and merge requests tagged
   for this release should now be complete

`git-evtag`
---

Releases should be done with `git evtag` rather than `git tag -s`, as it provides
stronger security guarantees. See
[its documentation](https://github.com/cgwalters/git-evtag) for more details.
In particular, it calculates its checksum over all blobs reachable from the tag,
including submodules; and uses a stronger checksum than SHA-1.

`git tag -a` must not be used. Releases must be cryptographically signed to
prove that they were made by a maintainer and have not been modified since
release.

You will need a GPG key for this, ideally which has been signed by others so
that it can be verified as being yours. However, even if your GPG key is
unsigned, using `git evtag` is still beneficial over using `git tag`.

It is possible to use an SSH key rather than a GPG key — git supports both,
although GitLab currently does not support verifying signatures by SSH keys.

See [the GNOME Handbook](https://handbook.gnome.org/maintainers/signing-releases.html)
for further information about release signing.
