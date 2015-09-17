module Scintilla
  class ScinTerm
    def ScinTerm::set_pallete(color_list)
      if color_list.length == 16
        for i in 0..15
          for j in 0..15
            n = color_pair(i, j)
            Curses::init_pair(n, color_list[i], color_list[j])
          end
        end
      end
    end
  end
end
