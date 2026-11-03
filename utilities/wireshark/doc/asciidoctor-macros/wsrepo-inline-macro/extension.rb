# SPDX-License-Identifier: MIT
require 'asciidoctor/extensions' unless RUBY_ENGINE == 'opal'

include ::Asciidoctor

# An inline macro that generates a link to a file in the 'master' branch of the
# Wireshark repository.
#
# Usage
#
#   wsrepofile:<path>[<descriptive text>]
#
# The default is to use <path> as the descriptive text.
#
class WSRepoFileInlineMacro < Extensions::InlineMacroProcessor
  include WsUtils
  use_dsl

  named :wsrepofile
  parse_content_as :text

  def process(parent, repopath, attrs)
    if repopath[0] == '/'
        repopath = repopath[1..]
    end
    target = %(https://gitlab.com/wireshark/wireshark/-/raw/master/#{repopath})
    repotext = !attrs['text'].nil_or_empty? ? attrs['text'] : repopath
    create_doc_links(parent, target, repotext)
  end
end

# An inline macro that generates a link to a view of a directory in the
# 'master' branch of the Wireshark repository.
#
# Usage
#
#   wsrepodir:<path>[<descriptive text>]
#
# The default is to use <path> as the descriptive text.
#
class WSRepoDirInlineMacro < Extensions::InlineMacroProcessor
  include WsUtils
  use_dsl

  named :wsrepodir
  parse_content_as :text

  def process(parent, repopath, attrs)
    if repopath[0] == '/'
        repopath = repopath[1..]
    end
    target = %(https://gitlab.com/wireshark/wireshark/tree/master/#{repopath})
    repotext = !attrs['text'].nil_or_empty? ? attrs['text'] : repopath
    create_doc_links(parent, target, repotext)
  end
end
