MRuby::Gem::Specification.new('mruby-scinterm') do |spec|
  spec.license = 'MIT'
  spec.authors = 'masahino'
  spec.add_dependency('mruby-curses', :github => 'masahino/mruby-curses')
  spec.add_dependency('mruby-scintilla-base', :github => 'masahino/mruby-scintilla-base')

  def spec.download_scintilla
    require 'open-uri'
    scintilla_ver='366'
    scinterm_ver='1.8'
    scintilla_url = "http://www.scintilla.org/scintilla#{scintilla_ver}.tgz"
    scintilla_dir = "#{build_dir}/scintilla"
    scintilla_a = "#{scintilla_dir}/scintilla/bin/scintilla.a"
    scinterm_url = "http://foicica.com/scinterm/download/scinterm_#{scinterm_ver}.zip"
    scinterm_file = "#{scintilla_dir}/scintilla/scinterm_#{scinterm_ver}.zip"
    scinterm_dir = "#{scintilla_dir}/scintilla/scinterm_#{scinterm_ver}"

    unless File.exists?(scintilla_a)
      open(scintilla_url, "r") do |http|
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
      sh %Q{(cd #{scinterm_dir} && make CXX=#{build.cxx.command} AR=#{build.archiver.command} CURSES_FLAGS=-DNO_CXX_REGEX)}
    end

    self.linker.flags_before_libraries << scintilla_a
    [self.cc, self.cxx, self.objc, self.mruby.cc, self.mruby.cxx, self.mruby.objc].each do |cc|
      cc.include_paths << scintilla_dir+"/scintilla/include"
      cc.include_paths << scintilla_dir+"/scintilla/src"
      cc.include_paths << scinterm_dir
    end
  end
end
