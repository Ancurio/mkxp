#==============================================================================
# ** Window_PartyCommand
#------------------------------------------------------------------------------
#  This window is used to select whether to fight or escape on the battle
#  screen.
#==============================================================================

class Window_PartyCommand < Window_Selectable
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    super(0, 0, 640, 64)
    self.contents = Bitmap.new(width - 32, height - 32)
    self.back_opacity = 160
    @commands = ["Fight", "Escape"]
    @item_max = 2
    @column_max = 2
    draw_item(0, normal_color)
    draw_item(1, $game_temp.battle_can_escape ? normal_color : disabled_color)
    self.active = false
    self.visible = false
    self.index = 0
  end
  #--------------------------------------------------------------------------
  # * Draw Item
  #     index : item number
  #     color : text character color
  #--------------------------------------------------------------------------
  def draw_item(index, color)
    self.contents.font.color = color
    rect = Rect.new(160 + index * 160 + 4, 0, 128 - 10, 32)
    self.contents.fill_rect(rect, Color.new(0, 0, 0, 0))
    self.contents.draw_text(rect, @commands[index], 1)
  end
  #--------------------------------------------------------------------------
  # * Cursor Rectangle Update
  #--------------------------------------------------------------------------
  def update_cursor_rect
    self.cursor_rect.set(160 + index * 160, 0, 128, 32)
  end
end
