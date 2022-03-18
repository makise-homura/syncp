project('syncp', 'c', version: 'v0.1-dev')

# E-mail to insert to .pot file
email = 'akemi_homura@kurisa.ch'

# Determine version from git
ver = meson.project_version()
git = find_program('git', required: false)
if git.found()
    git_describe_cmd = run_command(git, 'describe', '--dirty')
    if git_describe_cmd.returncode() == 0
        ver = git_describe_cmd.stdout().strip()
    endif
endif
message('determined version ' + ver)

# Test for syncfs existence
if meson.get_compiler('c').has_function('syncfs', prefix : '#include <unistd.h>')
    add_global_arguments('-DHAVE_SYNCFS=1', language : 'c')
endif

subdir('po')
localedir = get_option('prefix') / get_option('localedir')

executable('syncp', 'syncp.c', install : true, c_args: ['-DPROG_NAME="' + meson.project_name() + '"', '-DPROG_VERSION="' + ver + '"', '-DLOCALEDIR="' + localedir + '"'])
