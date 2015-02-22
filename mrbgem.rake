MRuby::Gem::Specification.new('mruby-scinterm') do |spec|
  spec.license = 'MIT'
  spec.authors = 'masahino'
  spec.add_dependency('mruby-curses', :github => 'masahino/mruby-curses')
  spec.add_dependency('mruby-scintilla-base')
end
