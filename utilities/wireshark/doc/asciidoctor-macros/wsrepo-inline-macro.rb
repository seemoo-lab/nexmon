# SPDX-License-Identifier: MIT
# Copied from https://github.com/asciidoctor/asciidoctor-extensions-lab/blob/master/lib/man-inline-macro.rb

RUBY_ENGINE == 'opal' ? (require 'wsrepo-inline-macro/extension') : (require_relative 'wsrepo-inline-macro/extension')

Extensions.register do
  inline_macro WSRepoFileInlineMacro
  inline_macro WSRepoDirInlineMacro
end
