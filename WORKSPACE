load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")
git_repository(
    name = "gtest",
    remote = "https://github.com/google/googletest",
    commit = "2fe3bd994b3189899d93f1d5a881e725e046fdc2",
    shallow_since = "1535728917 -0400",
)

new_git_repository(
    name = "gsl",
    remote = "https://github.com/microsoft/GSL",
    commit = "1995e86d1ad70519465374fb4876c6ef7c9f8c61",
    shallow_since = "1534723853 -0700",
    build_file = "gsl.BUILD",
)
