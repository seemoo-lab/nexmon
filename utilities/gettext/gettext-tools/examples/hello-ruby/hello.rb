#!@RUBY@
require 'gettext'
include GetText

bindtextdomain("hello-ruby", :path => "@localedir@")

puts _("Hello, world!")
puts _("This program is running as process number %{pid}.") % { :pid => Process.pid }
