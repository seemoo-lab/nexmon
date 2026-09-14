#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2025 Marco Trevisan (Treviño)

import argparse
import gitlab
import os
import time
import sys

from datetime import datetime


def has_self_mr_note(mr, current_user_id, contents):
    for note in mr.notes.list(get_all=True):
        if note.author["id"] == current_user_id:
            if contents in note.body:
                return True
    return False


def get_latest_job_for_mr_parent(project, mr, job_name):
    mr_commits = mr.commits(get_all=True)
    first_mr_commit = list(mr_commits)[-1]

    pipelines = []
    job = None

    for parent_sha in first_mr_commit.parent_ids:
        pipelines.extend(
            project.pipelines.list(
                sha=parent_sha,
                ref=mr.target_branch,
                get_all=True,
                order_by="id",
                sort="desc",
            )
        )

    for pipeline in pipelines:
        jobs = pipeline.jobs.list(all=True)

        for job in jobs:
            if job.status == "success" and job.name == job_name:
                return job


if __name__ == "__main__":
    server_uri = os.environ["CI_SERVER_URL"]
    project_path = os.environ["CI_PROJECT_PATH"]
    token_file = os.environ["GIR_CHECK_TOKEN_FILE"]

    parser = argparse.ArgumentParser()
    parser.add_argument("--mr", required=True)
    parser.add_argument("--job", required=True)
    parser.add_argument("--sha")
    parser.add_argument("--job-id-output")
    parser.add_argument("--last-target-job-id-output")
    parser.add_argument("--skip-if-mr-contains-our-comment-text")
    args = parser.parse_args()

    with open(token_file, mode="rt", encoding="utf-8") as tf:
        token = tf.read().strip()

    gl = gitlab.Gitlab(server_uri, private_token=token)
    project = gl.projects.get(project_path)

    [mr] = project.mergerequests.list(iids=[args.mr])

    if args.sha and not mr.sha.startswith(args.sha):
        print(f"MR !{mr.iid} not matching SHA {args.sha}", flush=True)
        sys.exit(77)

    if args.skip_if_mr_contains_our_comment_text:
        print(
            f"Checking comment on MR {server_uri}/{project_path}/-/"
            + f"merge_requests/{mr.iid}",
            flush=True,
        )

        if has_self_mr_note(
            mr,
            gl.http_get("/user")["id"],
            args.skip_if_mr_contains_our_comment_text,
        ):
            print(
                f"MR !{mr.iid} contains already a matching comment, skipping!",
                flush=True,
            )
            sys.exit(77)

    if args.last_target_job_id_output:
        job = get_latest_job_for_mr_parent(project, mr, args.job)

        if job is None:
            print(
                f"Impossible to find a valid job for branch {mr.target_branch}",
                file=sys.stderr,
                flush=True,
            )
            sys.exit(77)

        if job.artifacts_expire_at:
            artifacts_expire_date = datetime.fromisoformat(job.artifacts_expire_at)
            if datetime.now(artifacts_expire_date.tzinfo) >= artifacts_expire_date:
                print(
                    f"Artifacts for job {job.id} have exipred",
                    file=sys.stderr,
                    flush=True,
                )
                sys.exit(77)

        with open(args.last_target_job_id_output, "wt", encoding="utf-8") as f:
            f.write(f"{job.id}\n")

    # Find the project that is associated to the merge request's source branch
    if mr.source_project_id != project.id:
        source_project = gl.projects.get(mr.source_project_id)
    else:
        source_project = project

    try:
        pipelines = source_project.pipelines.list(sha=mr.sha, ref=mr.source_branch)
        ci_pipeline_id = os.environ.get("CI_PIPELINE_ID")
        [pipeline] = [
            p for p in pipelines if p.source != "trigger" and p.id != ci_pipeline_id
        ]
    except ValueError:
        print(
            f"No unique pipeline found for {server_uri}/{source_project.path_with_namespace}/-/"
            + f"commit/{mr.sha}",
            flush=True,
        )
        sys.exit(1)

    print(
        f"Found matching pipeline {server_uri}/{source_project.path_with_namespace}/-/"
        + f"pipelines/{pipeline.id}",
        flush=True,
    )

    try:
        [job] = [
            j
            for j in pipeline.jobs.list(all=True)
            if j.name == args.job and j.commit["id"] == mr.sha
        ]
    except ValueError:
        print(
            f"No unique '{args.job}' job found for {server_uri}/{source_project.path_with_namespace}"
            + f"/-/commit/{mr.sha}",
            flush=True,
        )
        sys.exit(1)

    print(
        f"Found matching job {server_uri}/{source_project.path_with_namespace}/-/jobs/{job.id}",
        flush=True,
    )

    if args.job_id_output:
        with open(args.job_id_output, "wt", encoding="utf-8") as f:
            f.write(f"{job.id}\n")

    waiting = False
    while job.status != "success":
        if job.status in (
            "pending",
            "running",
            "created",
            "preparing",
            "waiting_for_resource",
        ):
            if not waiting:
                print("Waiting for the job to complete...", flush=True)
                job = source_project.jobs.get(job.id)
                waiting = True

            time.sleep(5)
            job.refresh()
            continue

        if job.status == "canceled":
            print(f"Job {job.id} was cancelled", file=sys.stderr, flush=True)
            sys.exit(77)

        print(f"Job {job.id} did not succeed", file=sys.stderr, flush=True)
        sys.exit(1)

    print(f"Job {job.id} completed!", flush=True)
