#==============================================================================
# ** Window_NameInput
#------------------------------------------------------------------------------
#  This window is used to select text characters on the input name screen.
#==============================================================================

class Window_NameInput < Window_Base
  GROUP_WIDTH = 5

  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    super(0, 128, 640, 352)
    self.contents = Bitmap.new(width - 32, height - 32)
    @index = 0

    # Create dimension information
    @character_table = $language.character_table
    @group_height = $language.character_table_height
    @group_size = @group_height * GROUP_WIDTH
    @num_groups = @character_table.length / @group_size
    @start_x = ((width - 32) - @num_groups * 152 + 12) / 2
    @ok_text = tr("OK")
    @ok_text_size = self.contents.text_size(@ok_text).width

    refresh
    update_cursor_rect
  end
  #--------------------------------------------------------------------------
  # * Text Character Acquisition
  #--------------------------------------------------------------------------
  def character
    return @character_table[@index]
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    self.contents.clear
    for i in 0...@character_table.length
      x = @start_x + i / GROUP_WIDTH / @group_height * 152 + i % GROUP_WIDTH * 28
      y = i / GROUP_WIDTH % @group_height * 32
      self.contents.draw_text(x, y, 28, 32, @character_table[i], 1)
    end
    self.contents.draw_text(@start_x + @num_groups * 152 - 12 - @ok_text_size - 16,
                            9 * 32, @ok_text_size, 32, @ok_text, 1)
  end
  #--------------------------------------------------------------------------
  # * Cursor Rectangle Update
  #--------------------------------------------------------------------------
  def update_cursor_rect
    # If cursor is positioned on [OK]
    if @index >= @character_table.length
      self.cursor_rect.set(@start_x + @num_groups * 152 - 12 - @ok_text_size - 32,
                           9 * 32, @ok_text_size + 32, 32)
    # If cursor is positioned on anything other than [OK]
    else
      x = @start_x + @index / GROUP_WIDTH / @group_height * 152 + @index % GROUP_WIDTH * 28
      y = @index / GROUP_WIDTH % @group_height * 32
      self.cursor_rect.set(x, y, 28, 32)
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    super
    # If cursor is positioned on [OK]
    if @index >= @character_table.length
      # Cursor down
      if Input.trigger?(Input::DOWN)
        $game_system.se_play($data_system.cursor_se)
        @index -= @character_table.length
      end
      # Cursor up
      if Input.repeat?(Input::UP)
        $game_system.se_play($data_system.cursor_se)
        @index -= @character_table.length - (@group_size - GROUP_WIDTH)
      end
    # If cursor is positioned on anything other than [OK]
    else
      # If right directional button is pushed
      if Input.repeat?(Input::RIGHT)
        # If directional button pressed down is not a repeat, or
        # cursor is not positioned on the right edge
        if Input.trigger?(Input::RIGHT) or
           @index / @group_size < 3 or @index % GROUP_WIDTH < 4
          # Move cursor to right
          $game_system.se_play($data_system.cursor_se)
          if @index % GROUP_WIDTH < 4
            @index += 1
          else
            @index += @group_size - 4
          end
          if @index >= @character_table.length
            @index -= @character_table.length
          end
        end
      end
      # If left directional button is pushed
      if Input.repeat?(Input::LEFT)
        # If directional button pressed down is not a repeat, or
        # cursor is not positioned on the left edge
        if Input.trigger?(Input::LEFT) or
           @index / @group_size > 0 or @index % GROUP_WIDTH > 0
          # Move cursor to left
          $game_system.se_play($data_system.cursor_se)
          if @index % GROUP_WIDTH > 0
            @index -= 1
          else
            @index -= @group_size - 4
          end
          if @index < 0
            @index += @character_table.length
          end
        end
      end
      # If down directional button is pushed
      if Input.repeat?(Input::DOWN)
        # Move cursor down
        $game_system.se_play($data_system.cursor_se)
        if @index % @group_size < @group_size - GROUP_WIDTH
          @index += GROUP_WIDTH
        else
          @index += @character_table.length - (@group_size - GROUP_WIDTH)
        end
      end
      # If up directional button is pushed
      if Input.repeat?(Input::UP)
        # If directional button pressed down is not a repeat, or
        # cursor is not positioned on the upper edge
        if Input.trigger?(Input::UP) or @index % @group_size >= GROUP_WIDTH
          # Move cursor up
          $game_system.se_play($data_system.cursor_se)
          if @index % @group_size >= GROUP_WIDTH
            @index -= GROUP_WIDTH
          else
            @index += @character_table.length
          end
        end
      end
    end
    update_cursor_rect
  end
end
