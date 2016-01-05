#==============================================================================
# ** Window_InputNumber
#------------------------------------------------------------------------------
#  This window is for inputting numbers, and is used within the
#  message window.
#==============================================================================

class Window_InputNumber < Window_Base
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     digits_max : digit count
  #--------------------------------------------------------------------------
  def initialize(digits_max)
    @digits_max = digits_max
    @number = 0
    # Calculate cursor width from number width (0-9 equal width and postulate)
    dummy_bitmap = Bitmap.new(32, 32)
    @cursor_width = dummy_bitmap.text_size("0").width + 8
    dummy_bitmap.dispose
    super(0, 0, @cursor_width * @digits_max + 32, 64)
    self.contents = Bitmap.new(width - 32, height - 32)
    self.z += 9999
    self.opacity = 0
    @index = 0
    refresh
    update_cursor_rect
  end
  #--------------------------------------------------------------------------
  # * Get Number
  #--------------------------------------------------------------------------
  def number
    return @number
  end
  #--------------------------------------------------------------------------
  # * Set Number
  #     number : new number
  #--------------------------------------------------------------------------
  def number=(number)
    @number = [[number, 0].max, 10 ** @digits_max - 1].min
    refresh
  end
  #--------------------------------------------------------------------------
  # * Cursor Rectangle Update
  #--------------------------------------------------------------------------
  def update_cursor_rect
    self.cursor_rect.set(@index * @cursor_width, 0, @cursor_width, 32)
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    super
    # If up or down directional button was pressed
    if Input.repeat?(Input::UP) or Input.repeat?(Input::DOWN)
      $game_system.se_play($data_system.cursor_se)
      # Get current place number and change it to 0
      place = 10 ** (@digits_max - 1 - @index)
      n = @number / place % 10
      @number -= n * place
      # If up add 1, if down substract 1
      n = (n + 1) % 10 if Input.repeat?(Input::UP)
      n = (n + 9) % 10 if Input.repeat?(Input::DOWN)
      # Reset current place number
      @number += n * place
      refresh
    end
    # Cursor right
    if Input.repeat?(Input::RIGHT)
      if @digits_max >= 2
        $game_system.se_play($data_system.cursor_se)
        @index = (@index + 1) % @digits_max
      end
    end
    # Cursor left
    if Input.repeat?(Input::LEFT)
      if @digits_max >= 2
        $game_system.se_play($data_system.cursor_se)
        @index = (@index + @digits_max - 1) % @digits_max
      end
    end
    update_cursor_rect
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    self.contents.clear
    self.contents.font.color = normal_color
    s = sprintf("%0*d", @digits_max, @number)
    for i in 0...@digits_max
      self.contents.draw_text(i * @cursor_width + 4, 0, 32, 32, s[i,1])
    end
  end
end
