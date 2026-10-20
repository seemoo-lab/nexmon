examples = [
    'gjobread',
    'io1',
    'io2',
    'parse1',
    'parse2',
    'parse3',
    'parse4',
    'reader1',
    'reader2',
    'reader3',
    'reader4',
    'testWriter',
    'tree1',
    'tree2',
    'xpath1',
    'xpath2',
]

exes = []
foreach example : examples
    exe = executable(
        example,
        files(example + '.c'),
        dependencies: xml_dep,
        include_directories: config_dir,
        build_by_default: false,
    )
    exes += exe
    if example == 'io1'
        io1 = exe
    endif
endforeach

# arrange to build examples alongside tests
test(
    'examples',
    io1,
    depends: exes,
)
