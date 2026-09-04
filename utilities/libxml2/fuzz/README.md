libFuzzer instructions for libxml2
==================================

Set compiler and options. Make sure to enable at least basic optimizations
to avoid excessive stack usage. Also enable some debug output to get
meaningful stack traces.

    export CC=clang
    export CFLAGS=" \
        -O1 -gline-tables-only \
        -fsanitize=fuzzer-no-link,address,undefined \
        -fno-sanitize-recover=all \
        -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION"

Since llvm-symbolizer can use libxml2 itself, you may need the following
wrapper to make sure that it doesn't use the instrumented version of
libxml2:

    export ASAN_SYMBOLIZER_PATH="$(pwd)/.gitlab-ci/llvm-symbolizer"

Other options that can improve stack traces:

    -fno-omit-frame-pointer
    -fno-inline
    -fno-optimize-sibling-calls (disables tail call optimization)

Build libxml2 with instrumentation:

    ./configure --without-python
    make

Run fuzzers:

    make -C fuzz fuzz-xml

The environment variable XML_FUZZ_OPTIONS can be used to pass additional
flags to the fuzzer.

Malloc failure injection
------------------------

Most fuzzers inject malloc failures to cover code paths handling these
errors. This can lead to surprises when debugging crashes. You can set
the macro XML_FUZZ_MALLOC_ABORT in fuzz/fuzz.c to make the fuzz target
abort at the malloc invocation which would fail. This tells you if
and where a malloc failure was injected.

Some fuzzers also test whether malloc failures are reported. To debug
failures which aren't reported, it's helpful to enable
XML_FUZZ_MALLOC_ABORT to see which allocation failed. Debugging
failures which are erroneously reported can be harder. If the report
goes through xmlRaiseMemoryError, you can abort() there to get a
stack trace.

Bugs related to handling of malloc failures are not considered
security-critical by the libxml2 maintainers. Nevertheless, we'd like
to see such issues reported.
