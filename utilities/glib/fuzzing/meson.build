# Copyright 2018 pdknsk
# Copyright 2020, 2021, 2022 Endless OS Foundation, LLC
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

fuzz_targets = [
  'fuzz_bookmark',
  'fuzz_canonicalize_filename',
  'fuzz_data_input_stream_read_line',
  'fuzz_data_input_stream_read_line_utf8',
  'fuzz_data_input_stream_read_upto',
  'fuzz_date_parse',
  'fuzz_date_time_new_from_iso8601',
  'fuzz_dbus_message',
  'fuzz_dbus_node_info_new_for_xml',
  'fuzz_filename_from_uri',
  'fuzz_filename_to_uri',
  'fuzz_get_locale_variants',
  'fuzz_inet_address_mask_new_from_string',
  'fuzz_inet_address_new_from_string',
  'fuzz_inet_socket_address_new_from_string',
  'fuzz_key',
  'fuzz_markup_escape_text',
  'fuzz_network_address_parse',
  'fuzz_network_address_parse_uri',
  'fuzz_paths',
  'fuzz_resolver',
  'fuzz_special_dirs',
  'fuzz_string',
  'fuzz_uri_escape',
  'fuzz_uri_parse',
  'fuzz_uri_parse_params',
  'fuzz_utf8_normalize',
  'fuzz_utf8_validate',
  'fuzz_uuid_string_is_valid',
  'fuzz_variant_binary',
  'fuzz_variant_binary_byteswap',
  'fuzz_variant_text',
]

deps = [libgmodule_dep, libgio_dep, libglib_dep, libgobject_dep, test_profile_dep]

extra_sources = []
extra_c_args = cc.get_supported_arguments('-Werror=unused-function')

# Links in a static library provided by oss-fuzz, else a standalone driver.
# https://google.github.io/oss-fuzz/getting-started/new-project-guide/#buildsh-script-environment
have_fuzzing_engine = false
if have_cxx
  fuzzing_engine = cxx.find_library('FuzzingEngine', required : get_option('oss_fuzz'))
  have_fuzzing_engine = fuzzing_engine.found()
endif
if have_fuzzing_engine
  deps += fuzzing_engine
else
  extra_sources += 'driver.c'
endif

if have_fuzzing_engine or build_tests
  foreach target_name : fuzz_targets
    exe = executable(target_name, [extra_sources, target_name + '.c'],
      c_args : extra_c_args,
      dependencies : deps,
    )

    # If the FuzzingEngine isn’t available, build some unit tests to check
    # that the fuzzing files do basically work.  This doesn’t do any actual
    # fuzzing though.  Pass in the README as an arbitrary fuzzing input,
    # just so we have something.
    if not have_fuzzing_engine
      test(target_name, exe,
        args : files('README.md'),
        suite : 'fuzzing',
      )
    endif
  endforeach
endif
