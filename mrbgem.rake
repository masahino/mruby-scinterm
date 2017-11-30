MRuby::Gem::Specification.new('mruby-scinterm') do |spec|
  spec.license = 'MIT'
  spec.authors = 'masahino'
  spec.add_dependency('mruby-curses', :github => 'masahino/mruby-curses')
  spec.add_dependency('mruby-scintilla-base', :github => 'masahino/mruby-scintilla-base')

  def spec.download_scintilla
    require 'open-uri'
    scintilla_ver='376'
    scinterm_ver='1.9'
    scintilla_url = "http://www.scintilla.org/scintilla#{scintilla_ver}.tgz"
    scintilla_dir = "#{build_dir}/scintilla"
    scintilla_a = "#{scintilla_dir}/scintilla/bin/scintilla.a"
    scinterm_url = "https://foicica.com/scinterm/download/scinterm_#{scinterm_ver}.zip"
    scinterm_file = "#{scintilla_dir}/scintilla/scinterm_#{scinterm_ver}.zip"
    scinterm_dir = "#{scintilla_dir}/scintilla/scinterm_#{scinterm_ver}"

    unless File.exists?(scintilla_a)
      open(scintilla_url, 'Accept-Encoding' => '') do |http|
        scintilla_tar = http.read
        FileUtils.mkdir_p scintilla_dir
        IO.popen("tar xfz - -C #{filename scintilla_dir}", "w") do |f|
          f.write scintilla_tar
        end
      end
      open(scinterm_file, 'wb') do |output|
        open(scinterm_url, "r") do |http|
          output.write(http.read)
        end
      end
      sh %Q{(cd #{scintilla_dir}/scintilla && unzip -o #{filename scinterm_file})}
      sh %Q{(cd #{scintilla_dir}/scintilla && patch -p0 < #{dir}/misc/scinterm_1.9.patch)}
      curses_flag = "-DNO_CXX11_REGEX"
      if RUBY_PLATFORM =~ /cygwin/
        curses_flag += "-D_WIN32"
      end
      if build.kind_of?(MRuby::CrossBuild) && %w(x86_64-w64-mingw32 i686-w64-mingw32).include?(build.host_target)
        curses_flag += " -I/usr/#{build.host_target}/include/ncurses"
      end
      if build.kind_of?(MRuby::CrossBuild)
        curses_flag += " #{build.cxx.all_flags.gsub('\\','\\\\').gsub('"', '\\"')}"
      end
      sh %Q{(cd #{scinterm_dir} && make CXX=#{build.cxx.command} AR=#{build.archiver.command} CURSES_FLAGS="#{curses_flag}")}
    end

    self.linker.flags_before_libraries << scintilla_a
    [self.cc, self.cxx, self.objc, self.mruby.cc, self.mruby.cxx, self.mruby.objc].each do |cc|
      cc.include_paths << scintilla_dir+"/scintilla/include"
      cc.include_paths << scintilla_dir+"/scintilla/src"
      cc.include_paths << scinterm_dir
    end
  end
end
