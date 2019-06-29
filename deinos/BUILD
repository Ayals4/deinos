cc_library(
	name = "deinos",
	srcs = ["chess.cc", "algorithm.cc", "dsai.cc"],
	hdrs = ["chess.h", "algorithm.h", "dsai.h"],
	linkopts = ["-pthread"],
	visibility = ["//deinoscli:__pkg__", "//deinoslichess:__pkg__"],
	deps = [
		"@gsl//:gsl",
	],
)

cc_test(
	name = "test_chess",
	srcs = ["test_chess.cc"],
	deps = [
		":deinos",
		"@gtest//:gtest",
		"@gtest//:gtest_main",
	],
)

cc_test(
	name = "test_algorithm",
	srcs = ["test_algorithm.cc"],
	deps = [
		":deinos",
		"@gtest//:gtest",
		"@gtest//:gtest_main",
	],
)
