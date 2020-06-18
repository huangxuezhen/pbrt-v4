// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// It is licensed under the BSD license; see the file LICENSE.txt
// SPDX: BSD-3-Clause

#include <pbrt/pbrt.h>

#include <pbrt/options.h>
#include <pbrt/util/args.h>
#include <pbrt/util/error.h>
#include <pbrt/util/print.h>

#include <gtest/gtest.h>
#include <string>

using namespace pbrt;

void usage(const std::string &msg = "") {
    if (!msg.empty())
        fprintf(stderr, "pbrt_test: %s\n\n", msg.c_str());

    fprintf(stderr, R"(pbrt_test arguments:
  --log-level <level>         Log messages at or above this level, where <level>
                              is "verbose", "error", or "fatal". Default: "error".
  --nthreads <num>            Use specified number of threads for rendering.
  --test_filter <regexp>      Regular expression of test names to run.
  --vlog-level <n>            Set VLOG verbosity. (Default: 0, disabled.)
)");

    exit(msg.empty() ? 0 : 1);
}

int main(int argc, char **argv) {
    ExtendedOptions opt;
    opt.quiet = true;
    LogConfig logConfig;
    std::string logLevel = "error";
    std::string testFilter;

    char **origArgv = argv;
    // Process command-line arguments
    ++argv;
    while (*argv != nullptr) {
        auto onError = [](const std::string &err) {
            usage(err);
            exit(1);
        };

        if (ParseArg(&argv, "log-level", &logLevel, onError) ||
            ParseArg(&argv, "nthreads", &opt.nThreads, onError) ||
            ParseArg(&argv, "test-filter", &testFilter, onError) ||
            ParseArg(&argv, "vlog-level", &logConfig.vlogLevel, onError)) {
            // success
        } else if ((strcmp(*argv, "--help") == 0) || (strcmp(*argv, "-h") == 0)) {
            usage();
            return 0;
        } else {
            usage(StringPrintf("argument \"%s\" unknown", *argv));
            return 1;
        }
    }

    logConfig.level = LogLevelFromString(logLevel);

    InitPBRT(opt, logConfig);

    int googleArgc = 1;
    const char *googleArgv[4] = {};
    googleArgv[0] = argv[0];
    std::string filter;
    if (!testFilter.empty()) {
        filter = StringPrintf("--gtest_filter=%s", testFilter);
        googleArgc += 1;
        googleArgv[1] = filter.c_str();
    }
    testing::InitGoogleTest(&googleArgc, (char **)googleArgv);

    int ret = RUN_ALL_TESTS();

    CleanupPBRT();

    return ret;
}
