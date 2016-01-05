#==============================================================================
# ** Window_NameEdit
#------------------------------------------------------------------------------
#  This window is used to edit your name on the input name screen.
#==============================================================================

class Window_NameEdit < Window_Base
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_reader   :name                     # name
  attr_reader   :index                    # cursor position
  attr_reader   :max_char                 # max char
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     actor    : actor
  #     max_char : maximum number of characters
  #--------------------------------------------------------------------------
  def initialize
    super(0, 0, 640, 128)
    self.contents = Bitmap.new(width - 32, height - 32)
    @name = $game_oneshot.player_name
    @max_char = 16
    # Fit name within maximum number of characters
    name_array = @name.split(//)[0...@max_char]
    @name = ""
    for i in 0...name_array.size
      @name += name_array[i]
    end
    @default_name = @name
    @index = name_array.size
    refresh
    update_cursor_rect
  end
  #--------------------------------------------------------------------------
  # * Return to Default Name
  #--------------------------------------------------------------------------
  def restore_default
    @name = @default_name
    @index = @name.split(//).size
    refresh
    update_cursor_rect
  end
  #--------------------------------------------------------------------------
  # * Add Character
  #     character : text character to be added
  #--------------------------------------------------------------------------
  def add(character)
    if @index < @max_char and character != ""
      @name += character
      @index += 1
      refresh
      update_cursor_rect
    end
  end
  #--------------------------------------------------------------------------
  # * Delete Character
  #--------------------------------------------------------------------------
  def back
    if @index > 0
      # Delete 1 text character
      name_array = @name.split(//)
      @name = ""
      for i in 0...name_array.size-1
        @name += name_array[i]
      end
      @index -= 1
      refresh
      update_cursor_rect
    end
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    self.contents.clear
    # Draw name
    name_array = @name.split('')
    for i in 0...@max_char
      c = name_array[i]
      if c == nil
        c = "_"
      end
      x = (self.width - 32 - @max_char * 28) / 2 + i * 28
      self.contents.draw_text(x, 32, 28, 32, c, 1)
    end
    # Draw graphic
    #draw_actor_graphic(@actor, 320 - @max_char * 14 - 40, 80)
  end
  #--------------------------------------------------------------------------
  # * Cursor Rectangle Update
  #--------------------------------------------------------------------------
  def update_cursor_rect
    x = (self.width - 32 - @max_char * 28) / 2 + @index * 28
    self.cursor_rect.set(x, 32, 28, 32)
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    super
    update_cursor_rect
  end
end
