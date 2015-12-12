
assert('Scintilla::ScinTerm.new') do
  Curses::initscr
  assert_equal(Scintilla::ScinTerm.new.class, Scintilla::ScinTerm)
  Curses::endwin
end

assert('ScinTerm.refresh') do
  Curses::initscr
  sci = Scintilla::ScinTerm.new
  sci.refresh
  Curses::endwin
end

assert('ScinTerm.send_message') do
  Curses::initscr
  sci = Scintilla::ScinTerm.new
  assert_raise ArgumentError do sci.send_message(0,0,0) end
  assert_raise ArgumentError do sci.send_message(Scintilla::SCI_START, 1.2, 0) end
  assert_raise ArgumentError do sci.send_message(Scintilla::SCI_START, 0, 3.4) end
  Curses::endwin
end

assert('Scintilla const') do
  assert_equal(2051, Scintilla::SCI_STYLESETFORE)
  assert_equal(2052, Scintilla::SCI_STYLESETBACK)
  assert_equal(32, Scintilla::STYLE_DEFAULT)
end

assert('SCI_SETAUTOMATICFOLD, SCI_GETAUTOMATICFOLD') do
  Curses::initscr
  sci = Scintilla::ScinTerm.new
  sci.send_message(Scintilla::SCI_SETAUTOMATICFOLD, Scintilla::SC_AUTOMATICFOLD_SHOW, 0)
  assert_equal(Scintilla::SC_AUTOMATICFOLD_SHOW, sci.send_message(Scintilla::SCI_GETAUTOMATICFOLD, 0, 0))
  Curses::endwin
end

assert('SCI_SETFOCUS, SCI_GETFOCUS') do
  Curses::initscr
  sci = Scintilla::ScinTerm.new
  sci.send_message(Scintilla::SCI_SETFOCUS, true, nil)
  assert_equal(1, sci.send_message(Scintilla::SCI_GETFOCUS, nil, nil))
  Curses::endwin
end

assert('get property') do
  Curses::initscr
  sci = Scintilla::ScinTerm.new
  sci.send_message(Scintilla::SCI_SETPROPERTY, "hogehoge", "huga")
  assert_equal("huga",   sci.get_property("hogehoge"))
end


assert('SCI_COLOR_PAIR') do
  Curses::initscr
  sci = Scintilla::ScinTerm.new
  assert_equal(1, sci.color_pair(0, 0))
  assert_equal(0xc0c0c1, sci.color_pair(Scintilla::COLOR_WHITE, 0))
  assert_equal(0xc0c0c0*16+1, sci.color_pair(Scintilla::COLOR_BLACK, Scintilla::COLOR_WHITE))
end

assert('Platform') do
  assert_equal(:CURSES, Scintilla::PLATFORM)
end
